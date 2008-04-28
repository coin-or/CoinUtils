// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinUtilsConfig.h"

#include <cassert>
#include <cstdio>

#include "CoinFactorization.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include <stdio.h>
#include <iostream>
#if DENSE_CODE==1 
// using simple lapack interface
extern "C" 
{
  /** LAPACK Fortran subroutine DGETRS. */
  void F77_FUNC(dgetrs,DGETRS)(char *trans, cipfint *n,
                               cipfint *nrhs, const double *A, cipfint *ldA,
                               cipfint * ipiv, double *B, cipfint *ldB, ipfint *info,
			       int trans_len);
}
#endif
// For semi-sparse
#define BITS_PER_CHECK 8
#define CHECK_SHIFT 3
typedef unsigned char CoinCheckZero;

//:class CoinFactorization.  Deals with Factorization and Updates

// Updates part of column (FTRANL) when densish
void 
CoinFactorization::updateColumnLDensish ( CoinIndexedVector * regionSparse ,
					  int * regionIndex)
  const
{
  double *region = regionSparse->denseVector (  );
  int number = regionSparse->getNumElements (  );
  int numberNonZero;
  double tolerance = zeroTolerance_;
  
  numberNonZero = 0;
  int k;
  int i , iPivot;
  
  const CoinBigIndex *startColumn = startColumnL_.array();
  const int *indexRow = indexRowL_.array();
  const double *element = elementL_.array();
  int last = numberRows_;
  assert ( last == baseL_ + numberL_);
#if DENSE_CODE==1
  //can take out last bit of sparse L as empty
  last -= numberDense_;
#endif
  int smallestIndex = numberRowsExtra_;
  // do easy ones
  for (k=0;k<number;k++) {
    iPivot=regionIndex[k];
    if (iPivot<baseL_) 
      regionIndex[numberNonZero++]=iPivot;
    else
      smallestIndex = CoinMin(iPivot,smallestIndex);
  }
  // now others
  for ( i = smallestIndex; i < last; i++ ) {
    double pivotValue = region[i];
    CoinBigIndex start = startColumn[i];
    CoinBigIndex end = startColumn[i + 1];
    
    if ( fabs(pivotValue) > tolerance ) {
      CoinBigIndex j;
      for ( j = start; j < end; j ++ ) {
	int iRow = indexRow[j];
	double result = region[iRow];
	double value = element[j];

	region[iRow] = result - value * pivotValue;
      }     
      regionIndex[numberNonZero++] = i;
    } else {
      region[i] = 0.0;
    }       
  }     
  // and dense
  for ( ; i < numberRows_; i++ ) {
    double pivotValue = region[i];
    if ( fabs(pivotValue) > tolerance ) {
      regionIndex[numberNonZero++] = i;
    } else {
      region[i] = 0.0;
    }       
  }     
  regionSparse->setNumElements ( numberNonZero );
} 
// Updates part of column (FTRANL) when sparsish
void 
CoinFactorization::updateColumnLSparsish ( CoinIndexedVector * regionSparse,
					   int * regionIndex)
  const
{
  double *region = regionSparse->denseVector (  );
  int number = regionSparse->getNumElements (  );
  int numberNonZero;
  double tolerance = zeroTolerance_;
  
  numberNonZero = 0;
  int k;
  int i , iPivot;
  
  const CoinBigIndex *startColumn = startColumnL_.array();
  const int *indexRow = indexRowL_.array();
  const double *element = elementL_.array();
  int last = numberRows_;
  assert ( last == baseL_ + numberL_);
#if DENSE_CODE==1
  //can take out last bit of sparse L as empty
  last -= numberDense_;
#endif
  // mark known to be zero
  int nInBig = sizeof(CoinBigIndex)/sizeof(int);
  CoinCheckZero * mark = (CoinCheckZero *) (sparse_.array()+(2+nInBig)*maximumRowsExtra_);
  int smallestIndex = numberRowsExtra_;
  // do easy ones
  for (k=0;k<number;k++) {
    iPivot=regionIndex[k];
    if (iPivot<baseL_) { 
      regionIndex[numberNonZero++]=iPivot;
    } else {
      smallestIndex = CoinMin(iPivot,smallestIndex);
      int iWord = iPivot>>CHECK_SHIFT;
      int iBit = iPivot-(iWord<<CHECK_SHIFT);
      if (mark[iWord]) {
	mark[iWord] |= 1<<iBit;
      } else {
	mark[iWord] = 1<<iBit;
      }
    }
  }
  // now others
  // First do up to convenient power of 2
  int jLast = (smallestIndex+BITS_PER_CHECK-1)>>CHECK_SHIFT;
  jLast = CoinMin((jLast<<CHECK_SHIFT),last);
  for ( i = smallestIndex; i < jLast; i++ ) {
    double pivotValue = region[i];
    CoinBigIndex start = startColumn[i];
    CoinBigIndex end = startColumn[i + 1];
    
    if ( fabs(pivotValue) > tolerance ) {
      CoinBigIndex j;
      for ( j = start; j < end; j ++ ) {
	int iRow = indexRow[j];
	double result = region[iRow];
	double value = element[j];
	region[iRow] = result - value * pivotValue;
	int iWord = iRow>>CHECK_SHIFT;
	int iBit = iRow-(iWord<<CHECK_SHIFT);
	if (mark[iWord]) {
	  mark[iWord] |= 1<<iBit;
	} else {
	  mark[iWord] = 1<<iBit;
	}
      }     
      regionIndex[numberNonZero++] = i;
    } else {
      region[i] = 0.0;
    }       
  }
  
  int kLast = last>>CHECK_SHIFT;
  if (jLast<last) {
    // now do in chunks
    for (k=(jLast>>CHECK_SHIFT);k<kLast;k++) {
      unsigned int iMark = mark[k];
      if (iMark) {
	// something in chunk - do all (as imark may change)
	i = k<<CHECK_SHIFT;
	int iLast = i+BITS_PER_CHECK;
	for ( ; i < iLast; i++ ) {
	  double pivotValue = region[i];
	  CoinBigIndex start = startColumn[i];
	  CoinBigIndex end = startColumn[i + 1];
	  
	  if ( fabs(pivotValue) > tolerance ) {
	    CoinBigIndex j;
	    for ( j = start; j < end; j ++ ) {
	      int iRow = indexRow[j];
	      double result = region[iRow];
	      double value = element[j];
	      region[iRow] = result - value * pivotValue;
	      int iWord = iRow>>CHECK_SHIFT;
	      int iBit = iRow-(iWord<<CHECK_SHIFT);
	      if (mark[iWord]) {
		mark[iWord] |= 1<<iBit;
	      } else {
		mark[iWord] = 1<<iBit;
	      }
	    }     
	    regionIndex[numberNonZero++] = i;
	  } else {
	    region[i] = 0.0;
	  }       
	}
	mark[k]=0; // zero out marked
      }
    }
    i = kLast<<CHECK_SHIFT;
  }
  for ( ; i < last; i++ ) {
    double pivotValue = region[i];
    CoinBigIndex start = startColumn[i];
    CoinBigIndex end = startColumn[i + 1];
    
    if ( fabs(pivotValue) > tolerance ) {
      CoinBigIndex j;
      for ( j = start; j < end; j ++ ) {
	int iRow = indexRow[j];
	double result = region[iRow];
	double value = element[j];
	region[iRow] = result - value * pivotValue;
      }     
      regionIndex[numberNonZero++] = i;
    } else {
      region[i] = 0.0;
    }       
  }
  // Now dense part
  for ( ; i < numberRows_; i++ ) {
    double pivotValue = region[i];
    if ( fabs(pivotValue) > tolerance ) {
      regionIndex[numberNonZero++] = i;
    } else {
      region[i] = 0.0;
    }       
  }
  // zero out ones that might have been skipped
  mark[smallestIndex>>CHECK_SHIFT]=0;
  int kkLast = (numberRows_+BITS_PER_CHECK-1)>>CHECK_SHIFT;
  CoinZeroN(mark+kLast,kkLast-kLast);
  regionSparse->setNumElements ( numberNonZero );
} 
// Updates part of column (FTRANL) when sparse
void 
CoinFactorization::updateColumnLSparse ( CoinIndexedVector * regionSparse ,
					   int * regionIndex)
  const
{
  double *region = regionSparse->denseVector (  );
  int number = regionSparse->getNumElements (  );
  int numberNonZero;
  double tolerance = zeroTolerance_;
  
  numberNonZero = 0;
  int j, k;
  int i;
  
  const CoinBigIndex *startColumn = startColumnL_.array();
  const int *indexRow = indexRowL_.array();
  const double *element = elementL_.array();
  // use sparse_ as temporary area
  // mark known to be zero
  int * stack = sparse_.array();  /* pivot */
  int * list = stack + maximumRowsExtra_;  /* final list */
  CoinBigIndex * next = (CoinBigIndex *) (list + maximumRowsExtra_);  /* jnext */
  char * mark = (char *) (next + maximumRowsExtra_);
  int nList;
#ifdef COIN_DEBUG
  for (i=0;i<maximumRowsExtra_;i++) {
    assert (!mark[i]);
  }
#endif
  int nStack;
  nList=0;
  for (k=0;k<number;k++) {
    int kPivot=regionIndex[k];
    if (kPivot>=baseL_) {
      if(!mark[kPivot]) {
	stack[0]=kPivot;
	CoinBigIndex j=startColumn[kPivot+1]-1;
        nStack=0;
	while (nStack>=0) {
	  /* take off stack */
	  if (j>=startColumn[kPivot]) {
	    int jPivot=indexRow[j--];
	    assert (jPivot>=baseL_);
	    /* put back on stack */
	    next[nStack] =j;
	    if (!mark[jPivot]) {
	      /* and new one */
	      kPivot=jPivot;
	      j = startColumn[kPivot+1]-1;
	      stack[++nStack]=kPivot;
	      mark[kPivot]=1;
	      next[nStack]=j;
	    }
	  } else {
	    /* finished so mark */
	    list[nList++]=kPivot;
	    mark[kPivot]=1;
	    --nStack;
	    if (nStack>=0) {
	      kPivot=stack[nStack];
	      j=next[nStack];
	    }
	  }
	}
      }
    } else {
      // just put on list
      regionIndex[numberNonZero++]=kPivot;
    }
  }
  for (i=nList-1;i>=0;i--) {
    int iPivot = list[i];
    mark[iPivot]=0;
    double pivotValue = region[iPivot];
    if ( fabs ( pivotValue ) > tolerance ) {
      regionIndex[numberNonZero++]=iPivot;
      for ( j = startColumn[iPivot]; j < startColumn[iPivot+1]; j ++ ) {
	int iRow = indexRow[j];
	double value = element[j];
	region[iRow] -= value * pivotValue;
      }
    } else {
      region[iPivot]=0.0;
    }
  }
  regionSparse->setNumElements ( numberNonZero );
}
//  updateColumnL.  Updates part of column (FTRANL)
void
CoinFactorization::updateColumnL ( CoinIndexedVector * regionSparse,
					   int * regionIndex) const
{
  if (numberL_) {
    int number = regionSparse->getNumElements (  );
    int goSparse;
    // Guess at number at end
    if (sparseThreshold_>0) {
      if (ftranAverageAfterL_) {
	int newNumber = (int) (number*ftranAverageAfterL_);
	if (newNumber< sparseThreshold_&&(numberL_<<2)>newNumber)
	  goSparse = 2;
	else if (newNumber< sparseThreshold2_&&(numberL_<<1)>newNumber)
	  goSparse = 1;
	else
	  goSparse = 0;
      } else {
	if (number<sparseThreshold_&&(numberL_<<2)>number) 
	  goSparse = 2;
	else
	  goSparse = 0;
      }
    } else {
      goSparse=0;
    }
    switch (goSparse) {
    case 0: // densish
      updateColumnLDensish(regionSparse,regionIndex);
      break;
    case 1: // middling
      updateColumnLSparsish(regionSparse,regionIndex);
      break;
    case 2: // sparse
      updateColumnLSparse(regionSparse,regionIndex);
      break;
    }
  }
#ifdef DENSE_CODE
  if (numberDense_) {
    //take off list
    int lastSparse = numberRows_-numberDense_;
    int number = regionSparse->getNumElements();
    double *region = regionSparse->denseVector (  );
    int i=0;
    bool doDense=false;
    while (i<number) {
      int iRow = regionIndex[i];
      if (iRow>=lastSparse) {
	doDense=true;
	regionIndex[i] = regionIndex[--number];
      } else {
	i++;
      }
    }
    if (doDense) {
      //int iopt=0;
      //dges(denseArea_,&numberDense_,&numberDense_,densePermute_,
      //   &region[lastSparse],&iopt);
      char trans = 'N';
      int ione=1;
      int info;
      F77_FUNC(dgetrs,DGETRS)(&trans,&numberDense_,&ione,denseArea_,&numberDense_,
			      densePermute_,region+lastSparse,&numberDense_,&info,1);
      for (i=lastSparse;i<numberRows_;i++) {
	double value = region[i];
	if (value) {
	  if (fabs(value)>=1.0e-15) 
	    regionIndex[number++] = i;
	  else
	    region[i]=0.0;
	}
      }
      regionSparse->setNumElements(number);
    }
  }
#endif
}

int CoinFactorization::checkPivot(double saveFromU,
				 double oldPivot) const
{
  int status;
  if ( fabs ( saveFromU ) > 1.0e-8 ) {
    double checkTolerance;
    
    if ( numberRowsExtra_ < numberRows_ + 2 ) {
      checkTolerance = 1.0e-5;
    } else if ( numberRowsExtra_ < numberRows_ + 10 ) {
      checkTolerance = 1.0e-6;
    } else if ( numberRowsExtra_ < numberRows_ + 50 ) {
      checkTolerance = 1.0e-8;
    } else {
      checkTolerance = 1.0e-10;
    }       
    checkTolerance *= relaxCheck_;
    if ( fabs ( 1.0 - fabs ( saveFromU / oldPivot ) ) < checkTolerance ) {
      status = 0;
    } else {
#if COIN_DEBUG
      std::cout <<"inaccurate pivot "<< oldPivot << " " 
		<< saveFromU << std::endl;
#endif
      if ( fabs ( fabs ( oldPivot ) - fabs ( saveFromU ) ) < 1.0e-12 ||
        fabs ( 1.0 - fabs ( saveFromU / oldPivot ) ) < 1.0e-8 ) {
        status = 1;
      } else {
        status = 2;
      }       
    }       
  } else {
    //error
    status = 2;
#if COIN_DEBUG
    std::cout <<"inaccurate pivot "<< saveFromU / oldPivot 
	      << " " << saveFromU << std::endl;
#endif
  } 
  return status;
}
//  replaceColumn.  Replaces one Column to basis
//      returns 0=OK, 1=Probably OK, 2=singular, 3=no room
//partial update already in U
int
CoinFactorization::replaceColumn ( CoinIndexedVector * regionSparse,
                                 int pivotRow,
				  double pivotCheck ,
				  bool checkBeforeModifying)
{
  CoinBigIndex * startColumnU = startColumnU_.array();
  CoinBigIndex *startColumn;
  int *indexRow;
  double *element;
  
  //return at once if too many iterations
  if ( numberColumnsExtra_ >= maximumColumnsExtra_ ) {
    return 5;
  }       
  if ( lengthAreaU_ < startColumnU[maximumColumnsExtra_] ) {
    return 3;
  }   
  
  int * numberInRow = numberInRow_.array();
  int * numberInColumn = numberInColumn_.array();
  int * numberInColumnPlus = numberInColumnPlus_.array();
  int realPivotRow;
  realPivotRow = pivotColumn_.array()[pivotRow];
  //zeroed out region
  double *region = regionSparse->denseVector (  );
  
  element = elementU_.array();
  //take out old pivot column

  // If we have done no pivots then always check before modification
  if (!numberPivots_)
    checkBeforeModifying=true;
  
  totalElements_ -= numberInColumn[realPivotRow];
  double * pivotRegion = pivotRegion_.array();
  double oldPivot = pivotRegion[realPivotRow];
  // for accuracy check
  pivotCheck = pivotCheck / oldPivot;
#if COIN_DEBUG>1
  int checkNumber=1000000;
  //if (numberL_) checkNumber=-1;
  if (numberR_>=checkNumber) {
    printf("pivot row %d, check %g - alpha region:\n",
      realPivotRow,pivotCheck);
      /*int i;
      for (i=0;i<numberRows_;i++) {
      if (pivotRegion[i])
      printf("%d %g\n",i,pivotRegion[i]);
  }*/
  }   
#endif
  pivotRegion[realPivotRow] = 0.0;
  CoinBigIndex i;

  CoinBigIndex saveEnd = startColumnU[realPivotRow]
                         + numberInColumn[realPivotRow];
  // not necessary at present - but take no chances for future
  numberInColumn[realPivotRow] = 0;
  //get entries in row (pivot not stored)
  CoinBigIndex *startRow = startRowU_.array();
  CoinBigIndex start;
  CoinBigIndex end;

  start = startRow[realPivotRow];
  end = start + numberInRow[realPivotRow];
  int numberNonZero = 0;
  int *indexColumn = indexColumnU_.array();
  CoinBigIndex *convertRowToColumn = convertRowToColumnU_.array();
  int *regionIndex = regionSparse->getIndices (  );
  
#if COIN_DEBUG>1
  if (numberR_>=checkNumber) 
    printf("Before btranu\n");
#endif
  int smallestIndex=numberRowsExtra_;
  if (!checkBeforeModifying) {
    for ( i = start; i < end ; i ++ ) {
      int iColumn = indexColumn[i];
      smallestIndex = CoinMin(smallestIndex,iColumn);
      CoinBigIndex j = convertRowToColumn[i];
      
      region[iColumn] = element[j];
#if COIN_DEBUG>1
      if (numberR_>=checkNumber) 
	printf("%d %g\n",iColumn,region[iColumn]);
#endif
      element[j] = 0.0;
      regionIndex[numberNonZero++] = iColumn;
    }
  } else {
    for ( i = start; i < end ; i ++ ) {
      int iColumn = indexColumn[i];
      smallestIndex = CoinMin(smallestIndex,iColumn);
      CoinBigIndex j = convertRowToColumn[i];
      
      region[iColumn] = element[j];
#if COIN_DEBUG>1
      if (numberR_>=checkNumber) 
	printf("%d %g\n",iColumn,region[iColumn]);
#endif
      regionIndex[numberNonZero++] = iColumn;
    }
  }       
  //do BTRAN - finding first one to use
  regionSparse->setNumElements ( numberNonZero );
  updateColumnTransposeU ( regionSparse, smallestIndex );
  numberNonZero = regionSparse->getNumElements (  );

  double saveFromU = 0.0;

  CoinBigIndex startU = startColumnU[numberColumnsExtra_];
  int *indexU = &indexRowU_.array()[startU];
  double *elementU = &elementU_.array()[startU];
  

  // Do accuracy test here if caller is paranoid
  if (checkBeforeModifying) {
    double tolerance = zeroTolerance_;
    int number = numberInColumn[numberColumnsExtra_];
  
    for ( i = 0; i < number; i++ ) {
      int iRow = indexU[i];
      //if (numberCompressions_==99&&lengthU_==278)
      //printf("row %d saveFromU %g element %g region %g\n",
      //       iRow,saveFromU,elementU[i],region[iRow]);
      if ( fabs ( elementU[i] ) > tolerance ) {
	if ( iRow != realPivotRow ) {
	  saveFromU -= elementU[i] * region[iRow];
	} else {
	  saveFromU += elementU[i];
	}       
      }       
    }       
    //check accuracy
    int status = checkPivot(saveFromU,pivotCheck);
    if (status) {
      // restore some things
      pivotRegion[realPivotRow] = oldPivot;
      number = saveEnd-startColumnU[realPivotRow];
      totalElements_ += number;
      numberInColumn[realPivotRow]=number;
      regionSparse->clear();
      return status;
    } else {
      // do what we would have done by now
      for ( i = start; i < end ; i ++ ) {
	CoinBigIndex j = convertRowToColumn[i];
	element[j] = 0.0;
      }
    }
  }
  // Now zero out column of U
  //take out old pivot column
  for ( i = startColumnU[realPivotRow]; i < saveEnd ; i ++ ) {
    element[i] = 0.0;
  }       
  //zero out pivot Row (before or after?)
  //add to R
  startColumn = startColumnR_.array();
  indexRow = indexRowR_;
  element = elementR_;
  CoinBigIndex l = lengthR_;
  int number = numberR_;
  
  startColumn[number] = l;  //for luck and first time
  number++;
  startColumn[number] = l + numberNonZero;
  numberR_ = number;
  lengthR_ = l + numberNonZero;
  totalElements_ += numberNonZero;
  if ( lengthR_ >= lengthAreaR_ ) {
    //not enough room
    regionSparse->clear();
    return 3;
  }       
#if COIN_DEBUG>1
  if (numberR_>=checkNumber) 
    printf("After btranu\n");
#endif
  for ( i = 0; i < numberNonZero; i++ ) {
    int iRow = regionIndex[i];
#if COIN_DEBUG>1
    if (numberR_>=checkNumber) 
      printf("%d %g\n",iRow,region[iRow]);
#endif
    
    indexRow[l] = iRow;
    element[l] = region[iRow];
    l++;
  }       
  //take out row
  int * nextRow = nextRow_.array();
  int * lastRow = lastRow_.array();
  int next = nextRow[realPivotRow];
  int last = lastRow[realPivotRow];
  
  nextRow[last] = next;
  lastRow[next] = last;
  numberInRow[realPivotRow]=0;
#if COIN_DEBUG
  nextRow[realPivotRow] = 777777;
  lastRow[realPivotRow] = 777777;
#endif
  //do permute
  permute_.array()[numberRowsExtra_] = realPivotRow;
  // and other way
  permuteBack_.array()[realPivotRow] = numberRowsExtra_;
  permuteBack_.array()[numberRowsExtra_] = -1;;
  //and for safety
  permute_.array()[numberRowsExtra_ + 1] = 0;

  pivotColumn_.array()[pivotRow] = numberRowsExtra_;
  pivotColumnBack_.array()[numberRowsExtra_] = pivotRow;
  startColumn = startColumnU;
  indexRow = indexRowU_.array();
  element = elementU_.array();

  numberU_++;
  number = numberInColumn[numberColumnsExtra_];

  totalElements_ += number;
  lengthU_ += number;
  if ( lengthU_ >= lengthAreaU_ ) {
    //not enough room
    regionSparse->clear();
    return 3;
  }
       
  saveFromU = 0.0;
  
  //put in pivot
  //add row counts

  double tolerance = zeroTolerance_;
  
#if COIN_DEBUG>1
  if (numberR_>=checkNumber) 
    printf("On U\n");
#endif
  for ( i = 0; i < number; i++ ) {
    int iRow = indexU[i];
#if COIN_DEBUG>1
    if (numberR_>=checkNumber) 
      printf("%d %g\n",iRow,elementU[i]);
#endif
    
    if ( fabs ( elementU[i] ) > tolerance ) {
      if ( iRow != realPivotRow ) {
        int next = nextRow[iRow];
        int iNumberInRow = numberInRow[iRow];
        CoinBigIndex space;
        CoinBigIndex put = startRow[iRow] + iNumberInRow;
        
        space = startRow[next] - put;
        if ( space <= 0 ) {
          getRowSpaceIterate ( iRow, iNumberInRow + 4 );
          put = startRow[iRow] + iNumberInRow;
        }     
        indexColumn[put] = numberColumnsExtra_;
        convertRowToColumn[put] = i + startU;
        numberInRow[iRow] = iNumberInRow + 1;
        saveFromU = saveFromU - elementU[i] * region[iRow];
      } else {
        //zero out and save
        saveFromU += elementU[i];
        elementU[i] = 0.0;
      }       
    } else {
      elementU[i] = 0.0;
    }       
  }       
  //in at end
  last = lastRow[maximumRowsExtra_];
  nextRow[last] = numberRowsExtra_;
  lastRow[maximumRowsExtra_] = numberRowsExtra_;
  lastRow[numberRowsExtra_] = last;
  nextRow[numberRowsExtra_] = maximumRowsExtra_;
  startRow[numberRowsExtra_] = startRow[maximumRowsExtra_];
  numberInRow[numberRowsExtra_] = 0;
  //column in at beginning (as empty)
  int * nextColumn = nextColumn_.array();
  int * lastColumn = lastColumn_.array();
  next = nextColumn[maximumColumnsExtra_];
  lastColumn[next] = numberColumnsExtra_;
  nextColumn[maximumColumnsExtra_] = numberColumnsExtra_;
  nextColumn[numberColumnsExtra_] = next;
  lastColumn[numberColumnsExtra_] = maximumColumnsExtra_;
  //check accuracy - but not if already checked (optimization problem)
  int status =  (checkBeforeModifying) ? 0 : checkPivot(saveFromU,pivotCheck);

  if (status!=2) {
  
    double pivotValue = 1.0 / saveFromU;
    
    pivotRegion[numberRowsExtra_] = pivotValue;
    //modify by pivot
    for ( i = 0; i < number; i++ ) {
      elementU[i] *= pivotValue;
    }       
    maximumU_ = CoinMax(maximumU_,startU+number);
    numberRowsExtra_++;
    numberColumnsExtra_++;
    numberGoodU_++;
    numberPivots_++;
  }       
  if ( numberRowsExtra_ > numberRows_ + 50 ) {
    CoinBigIndex extra = factorElements_ >> 1;
    
    if ( numberRowsExtra_ > numberRows_ + 100 + numberRows_ / 500 ) {
      if ( extra < 2 * numberRows_ ) {
        extra = 2 * numberRows_;
      }       
    } else {
      if ( extra < 5 * numberRows_ ) {
        extra = 5 * numberRows_;
      }       
    }       
    CoinBigIndex added = totalElements_ - factorElements_;
    
    if ( added > extra && added > ( factorElements_ ) << 1 && !status 
	 && 3*totalElements_ > 2*(lengthAreaU_+lengthAreaL_)) {
      status = 3;
      if ( messageLevel_ & 4 ) {
        std::cout << "Factorization has "<< totalElements_
          << ", basis had " << factorElements_ <<std::endl;
      }
    }       
  }
  if (numberInColumnPlus&&status<2) {
    // we are going to put another copy of R in R
    double * elementR = elementR_ + lengthAreaR_;
    int * indexRowR = indexRowR_ + lengthAreaR_;
    CoinBigIndex * startR = startColumnR_.array()+maximumPivots_+1;
    int pivotRow = numberRowsExtra_-1;
    for ( i = 0; i < numberNonZero; i++ ) {
      int iRow = regionIndex[i];
      assert (pivotRow>iRow);
      next = nextColumn[iRow];
      CoinBigIndex space;
      if (next!=maximumColumnsExtra_)
	space = startR[next]-startR[iRow];
      else
	space = lengthAreaR_-startR[iRow];
      int numberInR = numberInColumnPlus[iRow];
      if (space>numberInR) {
	// there is space
	CoinBigIndex  put=startR[iRow]+numberInR;
	numberInColumnPlus[iRow]=numberInR+1;
	indexRowR[put]=pivotRow;
	elementR[put]=region[iRow];
	//add 4 for luck
	if (next==maximumColumnsExtra_)
	  startR[maximumColumnsExtra_] = CoinMin((CoinBigIndex) (put + 4) ,lengthAreaR_);
      } else {
	// no space - do we shuffle?
	if (!getColumnSpaceIterateR(iRow,region[iRow],pivotRow)) {
	  // printf("Need more space for R\n");
	  numberInColumnPlus_.conditionalDelete();
	  regionSparse->clear();
	  break;
	}
      }
      region[iRow]=0.0;
    }       
    regionSparse->setNumElements(0);
  } else {
    regionSparse->clear();
  }
  return status;
}

//  updateColumnTranspose.  Updates one column transpose (BTRAN)
int
CoinFactorization::updateColumnTranspose ( CoinIndexedVector * regionSparse,
                                          CoinIndexedVector * regionSparse2 ) 
  const
{
  //zero region
  regionSparse->clear (  );
  double *region = regionSparse->denseVector (  );
  double * vector = regionSparse2->denseVector();
  int * index = regionSparse2->getIndices();
  int numberNonZero = regionSparse2->getNumElements();
  int i;
  const int * pivotColumn = pivotColumn_.array();
  
  //move indices into index array
  int *regionIndex = regionSparse->getIndices (  );
  int iRow;
  bool packed = regionSparse2->packedMode();
  if (packed) {
    for ( i = 0; i < numberNonZero; i ++ ) {
      iRow = index[i];
      double value = vector[i];
      iRow=pivotColumn[iRow];
      vector[i]=0.0;
      region[iRow] = value;
      regionIndex[i] = iRow;
    }
  } else {
    for ( i = 0; i < numberNonZero; i ++ ) {
      iRow = index[i];
      double value = vector[iRow];
      vector[iRow]=0.0;
      iRow=pivotColumn[iRow];
      region[iRow] = value;
      regionIndex[i] = iRow;
    }
  }
  regionSparse->setNumElements ( numberNonZero );
  if (collectStatistics_) {
    numberBtranCounts_++;
    btranCountInput_ += (double) numberNonZero;
  }
  if (!doForrestTomlin_) {
    // Do PFI before everything else
    updateColumnTransposePFI(regionSparse);
    numberNonZero = regionSparse->getNumElements();
  }
  //  ******* U
  // Apply pivot region - could be combined for speed
  int j;
  double *pivotRegion = pivotRegion_.array();
  
  int smallestIndex=numberRowsExtra_;
  for ( j = 0; j < numberNonZero; j++ ) {
    int iRow = regionIndex[j];
    smallestIndex = CoinMin(smallestIndex,iRow);
    region[iRow] *= pivotRegion[iRow];
  }
  updateColumnTransposeU ( regionSparse,smallestIndex );
  if (collectStatistics_) 
    btranCountAfterU_ += (double) regionSparse->getNumElements();
  //permute extra
  //row bits here
  updateColumnTransposeR ( regionSparse );
  //  ******* L
  updateColumnTransposeL ( regionSparse );
  numberNonZero = regionSparse->getNumElements (  );
  if (collectStatistics_) 
    btranCountAfterL_ += (double) numberNonZero;
  const int * permuteBack = pivotColumnBack_.array();
  int number=0;
  if (packed) {
    for (i=0;i<numberNonZero;i++) {
      int iRow=regionIndex[i];
      double value = region[iRow];
      region[iRow]=0.0;
      if (fabs(value)>zeroTolerance_) {
	iRow=permuteBack[iRow];
	vector[number]=value;
	index[number++]=iRow;
      }
    }
  } else {
    for (i=0;i<numberNonZero;i++) {
      int iRow=regionIndex[i];
      double value = region[iRow];
      region[iRow]=0.0;
      if (fabs(value)>zeroTolerance_) {
	iRow=permuteBack[iRow];
	vector[iRow]=value;
	index[number++]=iRow;
      }
    }
  }
  regionSparse->setNumElements(0);
  regionSparse2->setNumElements(number);
#ifdef COIN_DEBUG
  for (i=0;i<numberRowsExtra_;i++) {
    assert (!region[i]);
  }
#endif
  return number;
}

/* Updates part of column transpose (BTRANU) when densish,
   assumes index is sorted i.e. region is correct */
void 
CoinFactorization::updateColumnTransposeUDensish 
                        ( CoinIndexedVector * regionSparse,
			  int smallestIndex) const
{
  double *region = regionSparse->denseVector (  );
  int numberNonZero = regionSparse->getNumElements (  );
  double tolerance = zeroTolerance_;
  
  int *regionIndex = regionSparse->getIndices (  );
  
  int i,j;
  
  const CoinBigIndex *startRow = startRowU_.array();
  
  const CoinBigIndex *convertRowToColumn = convertRowToColumnU_.array();
  const int *indexColumn = indexColumnU_.array();
  
  const double * element = elementU_.array();
  int last = numberU_;
  
  double pivotValue;
  
  const int *numberInRow = numberInRow_.array();
  numberNonZero = 0;
  for (i=smallestIndex ; i < last; i++ ) {
    pivotValue = region[i];
    if ( fabs ( pivotValue ) > tolerance ) {
      CoinBigIndex start = startRow[i];
      int numberIn = numberInRow[i];
      CoinBigIndex end = start + numberIn;
      for (j = start ; j < end; j ++ ) {
	int iRow = indexColumn[j];
	CoinBigIndex getElement = convertRowToColumn[j];
	double value = element[getElement];
	region[iRow] -=  value * pivotValue;
      }     
      regionIndex[numberNonZero++] = i;
    } else {
      region[i] = 0.0;
    }       
  }
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}
/* Updates part of column transpose (BTRANU) when sparsish,
      assumes index is sorted i.e. region is correct */
void 
CoinFactorization::updateColumnTransposeUSparsish 
                        ( CoinIndexedVector * regionSparse,
			  int smallestIndex) const
{
  double *region = regionSparse->denseVector (  );
  int numberNonZero = regionSparse->getNumElements (  );
  double tolerance = zeroTolerance_;
  
  int *regionIndex = regionSparse->getIndices (  );
  
  int i,j;
  
  const CoinBigIndex *startRow = startRowU_.array();
  
  const CoinBigIndex *convertRowToColumn = convertRowToColumnU_.array();
  const int *indexColumn = indexColumnU_.array();
  
  const double * element = elementU_.array();
  int last = numberU_;
  
  double pivotValue;
  
  const int *numberInRow = numberInRow_.array();
  
  // mark known to be zero
  int nInBig = sizeof(CoinBigIndex)/sizeof(int);
  CoinCheckZero * mark = (CoinCheckZero *) (sparse_.array()+(2+nInBig)*maximumRowsExtra_);

  for (i=0;i<numberNonZero;i++) {
    int iPivot=regionIndex[i];
    int iWord = iPivot>>CHECK_SHIFT;
    int iBit = iPivot-(iWord<<CHECK_SHIFT);
    if (mark[iWord]) {
      mark[iWord] |= 1<<iBit;
    } else {
      mark[iWord] = 1<<iBit;
    }
  }

  numberNonZero = 0;
  // Find convenient power of 2
  smallestIndex = smallestIndex >> CHECK_SHIFT;
  int kLast = last>>CHECK_SHIFT;
  // do in chunks
  int k;

  for (k=smallestIndex;k<kLast;k++) {
    unsigned int iMark = mark[k];
    if (iMark) {
      // something in chunk - do all (as imark may change)
      i = k<<CHECK_SHIFT;
      int iLast = i+BITS_PER_CHECK;
      for ( ; i < iLast; i++ ) {
	pivotValue = region[i];
	if ( fabs ( pivotValue ) > tolerance ) {
	  CoinBigIndex start = startRow[i];
	  int numberIn = numberInRow[i];
	  CoinBigIndex end = start + numberIn;
	  for (j = start ; j < end; j ++ ) {
	    int iRow = indexColumn[j];
	    CoinBigIndex getElement = convertRowToColumn[j];
	    double value = element[getElement];
	    int iWord = iRow>>CHECK_SHIFT;
	    int iBit = iRow-(iWord<<CHECK_SHIFT);
	    if (mark[iWord]) {
	      mark[iWord] |= 1<<iBit;
	    } else {
	      mark[iWord] = 1<<iBit;
	    }
	    region[iRow] -=  value * pivotValue;
	  }     
	  regionIndex[numberNonZero++] = i;
	} else {
	  region[i] = 0.0;
	}       
      }
      mark[k]=0;
    }
  }
  i = kLast<<CHECK_SHIFT;
  mark[kLast]=0;
  for (; i < last; i++ ) {
    pivotValue = region[i];
    if ( fabs ( pivotValue ) > tolerance ) {
      CoinBigIndex start = startRow[i];
      int numberIn = numberInRow[i];
      CoinBigIndex end = start + numberIn;
      for (j = start ; j < end; j ++ ) {
	int iRow = indexColumn[j];
	CoinBigIndex getElement = convertRowToColumn[j];
	double value = element[getElement];

	region[iRow] -=  value * pivotValue;
      }     
      regionIndex[numberNonZero++] = i;
    } else {
      region[i] = 0.0;
    }       
  }
#ifdef COIN_DEBUG
  for (i=0;i<maximumRowsExtra_;i++) {
    assert (!mark[i]);
  }
#endif
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}
/* Updates part of column transpose (BTRANU) when sparse,
   assumes index is sorted i.e. region is correct */
void 
CoinFactorization::updateColumnTransposeUSparse ( 
		   CoinIndexedVector * regionSparse) const
{
  double *region = regionSparse->denseVector (  );
  int numberNonZero = regionSparse->getNumElements (  );
  double tolerance = zeroTolerance_;
  
  int *regionIndex = regionSparse->getIndices (  );
  
  int i;
  
  const CoinBigIndex *startRow = startRowU_.array();
  
  const CoinBigIndex *convertRowToColumn = convertRowToColumnU_.array();
  const int *indexColumn = indexColumnU_.array();
  
  const double * element = elementU_.array();
  
  double pivotValue;
  
  int *numberInRow = numberInRow_.array();
  
  // use sparse_ as temporary area
  // mark known to be zero
  int * stack = sparse_.array();  /* pivot */
  int * list = stack + maximumRowsExtra_;  /* final list */
  CoinBigIndex * next = (CoinBigIndex *) (list + maximumRowsExtra_);  /* jnext */
  char * mark = (char *) (next + maximumRowsExtra_);
  int nList;
  int iPivot;
#ifdef COIN_DEBUG
  for (i=0;i<maximumRowsExtra_;i++) {
    assert (!mark[i]);
  }
#endif
#if 0
  {
    int i;
    for (i=0;i<numberRowsExtra_;i++) {
      CoinBigIndex krs = startRow[i];
      CoinBigIndex kre = krs + numberInRow[i];
      CoinBigIndex k;
      for (k=krs;k<kre;k++)
	assert (indexColumn[k]>i);
    }
  }
#endif
  int k,nStack;
  nList=0;
  for (k=0;k<numberNonZero;k++) {
    int kPivot=regionIndex[k];
    stack[0]=kPivot;
    CoinBigIndex j=startRow[kPivot]+numberInRow[kPivot]-1;
    next[0]=j;
    nStack=1;
    while (nStack) {
      /* take off stack */
      kPivot=stack[--nStack];
      if (mark[kPivot]!=1) {
	j=next[nStack];
	if (j>=startRow[kPivot]) {
	  kPivot=indexColumn[j--];
	  /* put back on stack */
	  next[nStack++] =j;
	  if (!mark[kPivot]) {
	    /* and new one */
	    j=startRow[kPivot]+numberInRow[kPivot]-1;
	    stack[nStack]=kPivot;
	    mark[kPivot]=2;
	    next[nStack++]=j;
	  }
	} else {
	  // finished
	  list[nList++]=kPivot;
	  mark[kPivot]=1;
	}
      }
    }
  }
  numberNonZero=0;
  for (i=nList-1;i>=0;i--) {
    iPivot = list[i];
    mark[iPivot]=0;
    pivotValue = region[iPivot];
    if ( fabs ( pivotValue ) > tolerance ) {
      CoinBigIndex start = startRow[iPivot];
      int numberIn = numberInRow[iPivot];
      CoinBigIndex end = start + numberIn;
      CoinBigIndex j;
      for (j=start ; j < end; j ++ ) {
	int iRow = indexColumn[j];
	CoinBigIndex getElement = convertRowToColumn[j];
	double value = element[getElement];
	region[iRow] -= value * pivotValue;
      }     
      regionIndex[numberNonZero++] = iPivot;
    } else {
      region[iPivot] = 0.0;
    }       
  }       
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}
//  updateColumnTransposeU.  Updates part of column transpose (BTRANU)
//assumes index is sorted i.e. region is correct
//does not sort by sign
void
CoinFactorization::updateColumnTransposeU ( CoinIndexedVector * regionSparse,
					    int smallestIndex) const
{
  int number = regionSparse->getNumElements (  );
  int goSparse;
  // Guess at number at end
  if (sparseThreshold_>0) {
    if (btranAverageAfterU_) {
      int newNumber = (int) (number*btranAverageAfterU_);
      if (newNumber< sparseThreshold_)
	goSparse = 2;
      else if (newNumber< sparseThreshold2_)
	goSparse = 1;
      else
	goSparse = 0;
    } else {
      if (number<sparseThreshold_) 
	goSparse = 2;
      else
	goSparse = 0;
    }
  } else {
    goSparse=0;
  }
  switch (goSparse) {
  case 0: // densish
    updateColumnTransposeUDensish(regionSparse,smallestIndex);
    break;
  case 1: // middling
    updateColumnTransposeUSparsish(regionSparse,smallestIndex);
    break;
  case 2: // sparse
    updateColumnTransposeUSparse(regionSparse);
    break;
  }
}

/*  updateColumnTransposeLDensish.  
    Updates part of column transpose (BTRANL) dense by column */
void
CoinFactorization::updateColumnTransposeLDensish 
     ( CoinIndexedVector * regionSparse ) const
{
  double *region = regionSparse->denseVector (  );
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero;
  double tolerance = zeroTolerance_;
  int base;
  int first = -1;
  
  numberNonZero=0;
  //scan
  for (first=numberRows_-1;first>=0;first--) {
    if (region[first]) 
      break;
  }
  if ( first >= 0 ) {
    base = baseL_;
    const CoinBigIndex *startColumn = startColumnL_.array();
    const int *indexRow = indexRowL_.array();
    const double *element = elementL_.array();
    int last = baseL_ + numberL_;
    
    if ( first >= last ) {
      first = last - 1;
    }       
    int i;
    double pivotValue;
    for (i = first ; i >= base; i-- ) {
      CoinBigIndex j;
      pivotValue = region[i];
      for ( j= startColumn[i] ; j < startColumn[i+1]; j++ ) {
	int iRow = indexRow[j];
	double value = element[j];
	pivotValue -= value * region[iRow];
      }       
      if ( fabs ( pivotValue ) > tolerance ) {
	region[i] = pivotValue;
	regionIndex[numberNonZero++] = i;
      } else {
	region[i] = 0.0;
      }       
    }       
    //may have stopped early
    if ( first < base ) {
      base = first + 1;
    }
    if (base > 5) {
      i=base-1;
      pivotValue=region[i];
      bool store = fabs(pivotValue) > tolerance;
      for (; i > 0; i-- ) {
	bool oldStore = store;
	double oldValue = pivotValue;
	pivotValue = region[i-1];
	store = fabs ( pivotValue ) > tolerance ;
	if (oldStore) {
	  region[i] = oldValue;
	  regionIndex[numberNonZero++] = i;
	} else {
	  region[i] = 0.0;
	}       
      }     
      if (store) {
	region[0] = pivotValue;
	regionIndex[numberNonZero++] = 0;
      } else {
	region[0] = 0.0;
      }       
    } else {
      for (i = base -1 ; i >= 0; i-- ) {
	pivotValue = region[i];
	if ( fabs ( pivotValue ) > tolerance ) {
	  region[i] = pivotValue;
	  regionIndex[numberNonZero++] = i;
	} else {
	  region[i] = 0.0;
	}       
      }     
    }
  } 
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}
/*  updateColumnTransposeLByRow. 
    Updates part of column transpose (BTRANL) densish but by row */
void
CoinFactorization::updateColumnTransposeLByRow 
    ( CoinIndexedVector * regionSparse ) const
{
  double *region = regionSparse->denseVector (  );
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero;
  double tolerance = zeroTolerance_;
  int first = -1;
  
  // use row copy of L
  const double * element = elementByRowL_.array();
  const CoinBigIndex * startRow = startRowL_.array();
  const int * column = indexColumnL_.array();
  int i;
  CoinBigIndex j;
  for (first=numberRows_-1;first>=0;first--) {
    if (region[first]) 
      break;
  }
  numberNonZero=0;
  for (i=first;i>=0;i--) {
    double pivotValue = region[i];
    if ( fabs ( pivotValue ) > tolerance ) {
      regionIndex[numberNonZero++] = i;
      for (j = startRow[i + 1]-1;j >= startRow[i]; j--) {
	int iRow = column[j];
	double value = element[j];
	region[iRow] -= pivotValue*value;
      }
    } else {
      region[i] = 0.0;
    }     
  }
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}
// Updates part of column transpose (BTRANL) when sparsish by row
void
CoinFactorization::updateColumnTransposeLSparsish 
    ( CoinIndexedVector * regionSparse ) const
{
  double *region = regionSparse->denseVector (  );
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero = regionSparse->getNumElements();
  double tolerance = zeroTolerance_;
  
  // use row copy of L
  const double * element = elementByRowL_.array();
  const CoinBigIndex * startRow = startRowL_.array();
  const int * column = indexColumnL_.array();
  int i;
  CoinBigIndex j;
  // mark known to be zero
  int nInBig = sizeof(CoinBigIndex)/sizeof(int);
  CoinCheckZero * mark = (CoinCheckZero *) (sparse_.array()+(2+nInBig)*maximumRowsExtra_);
  for (i=0;i<numberNonZero;i++) {
    int iPivot=regionIndex[i];
    int iWord = iPivot>>CHECK_SHIFT;
    int iBit = iPivot-(iWord<<CHECK_SHIFT);
    if (mark[iWord]) {
      mark[iWord] |= 1<<iBit;
    } else {
      mark[iWord] = 1<<iBit;
    }
  }
  numberNonZero = 0;
  // First do down to convenient power of 2
  int jLast = (numberRows_-1)>>CHECK_SHIFT;
  jLast = (jLast<<CHECK_SHIFT);
  for (i=numberRows_-1;i>=jLast;i--) {
    double pivotValue = region[i];
    if ( fabs ( pivotValue ) > tolerance ) {
      regionIndex[numberNonZero++] = i;
      for (j = startRow[i + 1]-1;j >= startRow[i]; j--) {
	int iRow = column[j];
	double value = element[j];
	int iWord = iRow>>CHECK_SHIFT;
	int iBit = iRow-(iWord<<CHECK_SHIFT);
	if (mark[iWord]) {
	  mark[iWord] |= 1<<iBit;
	} else {
	  mark[iWord] = 1<<iBit;
	}
	region[iRow] -= pivotValue*value;
      }
    } else {
      region[i] = 0.0;
    }     
  }
  // and in chunks
  jLast = jLast>>CHECK_SHIFT;
  int k ;
  for (k=jLast-1;k>=0;k--) {
    unsigned int iMark = mark[k];
    if (iMark) {
      // something in chunk - do all (as imark may change)
      int iLast = k<<CHECK_SHIFT;
      i = iLast+BITS_PER_CHECK-1;
      for ( ; i >= iLast; i-- ) {
	double pivotValue = region[i];
	if ( fabs ( pivotValue ) > tolerance ) {
	  regionIndex[numberNonZero++] = i;
	  for (j = startRow[i + 1]-1;j >= startRow[i]; j--) {
	    int iRow = column[j];
	    double value = element[j];
	    int iWord = iRow>>CHECK_SHIFT;
	    int iBit = iRow-(iWord<<CHECK_SHIFT);
	    if (mark[iWord]) {
	      mark[iWord] |= 1<<iBit;
	    } else {
	      mark[iWord] = 1<<iBit;
	    }
	    region[iRow] -= pivotValue*value;
	  }
	} else {
	  region[i] = 0.0;
	}     
      }
      mark[k]=0;
    }
  }
  mark[jLast]=0;
#ifdef COIN_DEBUG
  for (i=0;i<maximumRowsExtra_;i++) {
    assert (!mark[i]);
  }
#endif
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}
/*  updateColumnTransposeLSparse. 
    Updates part of column transpose (BTRANL) sparse */
void
CoinFactorization::updateColumnTransposeLSparse 
    ( CoinIndexedVector * regionSparse ) const
{
  double *region = regionSparse->denseVector (  );
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero = regionSparse->getNumElements (  );
  double tolerance = zeroTolerance_;
  
  // use row copy of L
  const double * element = elementByRowL_.array();
  const CoinBigIndex * startRow = startRowL_.array();
  const int * column = indexColumnL_.array();
  int i;
  CoinBigIndex j;
  // use sparse_ as temporary area
  // mark known to be zero
  int * stack = sparse_.array();  /* pivot */
  int * list = stack + maximumRowsExtra_;  /* final list */
  CoinBigIndex * next = (CoinBigIndex *) (list + maximumRowsExtra_);  /* jnext */
  char * mark = (char *) (next + maximumRowsExtra_);
  int nList;
  int number = numberNonZero;
  int k, iPivot;
#ifdef COIN_DEBUG
  for (i=0;i<maximumRowsExtra_;i++) {
    assert (!mark[i]);
  }
#endif
  int nStack;
  nList=0;
  for (k=0;k<number;k++) {
    int kPivot=regionIndex[k];
    if(!mark[kPivot]&&region[kPivot]) {
      stack[0]=kPivot;
      CoinBigIndex j=startRow[kPivot+1]-1;
      nStack=0;
      while (nStack>=0) {
	/* take off stack */
	if (j>=startRow[kPivot]) {
	  int jPivot=column[j--];
	  /* put back on stack */
	  next[nStack] =j;
	  if (!mark[jPivot]) {
	    /* and new one */
	    kPivot=jPivot;
	    j = startRow[kPivot+1]-1;
	    stack[++nStack]=kPivot;
	    mark[kPivot]=1;
	    next[nStack]=j;
	  }
	} else {
	  /* finished so mark */
	  list[nList++]=kPivot;
	  mark[kPivot]=1;
	  --nStack;
	  if (nStack>=0) {
	    kPivot=stack[nStack];
	    j=next[nStack];
	  }
	}
      }
    }
  }
  numberNonZero=0;
  for (i=nList-1;i>=0;i--) {
    iPivot = list[i];
    mark[iPivot]=0;
    double pivotValue = region[iPivot];
    if ( fabs ( pivotValue ) > tolerance ) {
      regionIndex[numberNonZero++] = iPivot;
      for ( j = startRow[iPivot]; j < startRow[iPivot+1]; j ++ ) {
	int iRow = column[j];
	double value = element[j];
	region[iRow] -= value * pivotValue;
      }
    } else {
      region[iPivot]=0.0;
    }
  }
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}
//  updateColumnTransposeL.  Updates part of column transpose (BTRANL)
void
CoinFactorization::updateColumnTransposeL ( CoinIndexedVector * regionSparse ) const
{
  int number = regionSparse->getNumElements (  );
  if (!numberL_&&!numberDense_) {
    if (sparse_.array()||number<numberRows_)
      return;
  }
  int goSparse;
  // Guess at number at end
  // we may need to rethink on dense
  if (sparseThreshold_>0) {
    if (btranAverageAfterL_) {
      int newNumber = (int) (number*btranAverageAfterL_);
      if (newNumber< sparseThreshold_)
	goSparse = 2;
      else if (newNumber< sparseThreshold2_)
	goSparse = 1;
      else
	goSparse = 0;
    } else {
      if (number<sparseThreshold_) 
	goSparse = 2;
      else
	goSparse = 0;
    }
  } else {
    goSparse=-1;
  }
#ifdef DENSE_CODE
  if (numberDense_) {
    //take off list
    int lastSparse = numberRows_-numberDense_;
    int number = regionSparse->getNumElements();
    double *region = regionSparse->denseVector (  );
    int *regionIndex = regionSparse->getIndices (  );
    int i=0;
    bool doDense=false;
    if (number<=numberRows_) {
      while (i<number) {
	int iRow = regionIndex[i];
	if (iRow>=lastSparse) {
	  doDense=true;
	  regionIndex[i] = regionIndex[--number];
	} else {
	  i++;
	}
      }
    } else {
      for (i=numberRows_-1;i>=lastSparse;i--) {
	if (region[i]) {
	  doDense=true;
          // numbers are all wrong - do a scan
          regionSparse->setNumElements(0);
          regionSparse->scan(0,lastSparse,zeroTolerance_);
          number=regionSparse->getNumElements();
	  break;
	}
      }
      if (sparseThreshold_)
	goSparse=0;
      else
	goSparse=-1;
    }
    if (doDense) {
      regionSparse->setNumElements(number);
      char trans = 'T';
      int ione=1;
      int info;
      F77_FUNC(dgetrs,DGETRS)(&trans,&numberDense_,&ione,denseArea_,&numberDense_,
			      densePermute_,region+lastSparse,&numberDense_,&info,1);
      //and scan again
      if (goSparse>0||!numberL_)
	regionSparse->scan(lastSparse,numberRows_,zeroTolerance_);
    } 
    if (!numberL_)
      return;
  } 
#endif
  switch (goSparse) {
  case -1: // No row copy
    updateColumnTransposeLDensish(regionSparse);
    break;
  case 0: // densish but by row
    updateColumnTransposeLByRow(regionSparse);
    break;
  case 1: // middling(and by row)
    updateColumnTransposeLSparsish(regionSparse);
    break;
  case 2: // sparse
    updateColumnTransposeLSparse(regionSparse);
    break;
  }
}

//  getRowSpaceIterate.  Gets space for one Row with given length
//may have to do compression  (returns true)
//also moves existing vector
bool
CoinFactorization::getRowSpaceIterate ( int iRow,
                                      int extraNeeded )
{
  const int * numberInRow = numberInRow_.array();
  int number = numberInRow[iRow];
  CoinBigIndex *startRow = startRowU_.array();
  int *indexColumn = indexColumnU_.array();
  CoinBigIndex *convertRowToColumn = convertRowToColumnU_.array();
  CoinBigIndex space = lengthAreaU_ - startRow[maximumRowsExtra_];
  int * nextRow = nextRow_.array();
  int * lastRow = lastRow_.array();
  if ( space < extraNeeded + number + 2 ) {
    //compression
    int iRow = nextRow[maximumRowsExtra_];
    CoinBigIndex put = 0;
    while ( iRow != maximumRowsExtra_ ) {
      //move
      CoinBigIndex get = startRow[iRow];
      CoinBigIndex getEnd = startRow[iRow] + numberInRow[iRow];
      
      startRow[iRow] = put;
      CoinBigIndex i;
      for ( i = get; i < getEnd; i++ ) {
	indexColumn[put] = indexColumn[i];
	convertRowToColumn[put] = convertRowToColumn[i];
	put++;
      }       
      iRow = nextRow[iRow];
    }       /* endwhile */
    numberCompressions_++;
    startRow[maximumRowsExtra_] = put;
    space = lengthAreaU_ - put;
    if ( space < extraNeeded + number + 2 ) {
      //need more space
      //if we can allocate bigger then do so and copy
      //if not then return so code can start again
      status_ = -99;
      return false;    }       
  }       
  CoinBigIndex put = startRow[maximumRowsExtra_];
  int next = nextRow[iRow];
  int last = lastRow[iRow];
  
  //out
  nextRow[last] = next;
  lastRow[next] = last;
  //in at end
  last = lastRow[maximumRowsExtra_];
  nextRow[last] = iRow;
  lastRow[maximumRowsExtra_] = iRow;
  lastRow[iRow] = last;
  nextRow[iRow] = maximumRowsExtra_;
  //move
  CoinBigIndex get = startRow[iRow];
  
  int * indexColumnU = indexColumnU_.array();
  startRow[iRow] = put;
  while ( number ) {
    number--;
    indexColumnU[put] = indexColumnU[get];
    convertRowToColumn[put] = convertRowToColumn[get];
    put++;
    get++;
  }       /* endwhile */
  //add four for luck
  startRow[maximumRowsExtra_] = put + extraNeeded + 4;
  return true;
}

//  getColumnSpaceIterateR.  Gets space for one extra R element in Column
//may have to do compression  (returns true)
//also moves existing vector
bool
CoinFactorization::getColumnSpaceIterateR ( int iColumn, double value,
					   int iRow)
{
  double * elementR = elementR_ + lengthAreaR_;
  int * indexRowR = indexRowR_ + lengthAreaR_;
  CoinBigIndex * startR = startColumnR_.array()+maximumPivots_+1;
  int * numberInColumnPlus = numberInColumnPlus_.array();
  int number = numberInColumnPlus[iColumn];
  //*** modify so sees if can go in
  //see if it can go in at end
  int * nextColumn = nextColumn_.array();
  int * lastColumn = lastColumn_.array();
  if (lengthAreaR_-startR[maximumColumnsExtra_]<number+1) {
    //compression
    int jColumn = nextColumn[maximumColumnsExtra_];
    CoinBigIndex put = 0;
    while ( jColumn != maximumColumnsExtra_ ) {
      //move
      CoinBigIndex get;
      CoinBigIndex getEnd;

      get = startR[jColumn];
      getEnd = get + numberInColumnPlus[jColumn];
      startR[jColumn] = put;
      CoinBigIndex i;
      for ( i = get; i < getEnd; i++ ) {
	indexRowR[put] = indexRowR[i];
	elementR[put] = elementR[i];
	put++;
      }
      jColumn = nextColumn[jColumn];
    }
    numberCompressions_++;
    startR[maximumColumnsExtra_]=put;
  }
  // Still may not be room (as iColumn was still in)
  if (lengthAreaR_-startR[maximumColumnsExtra_]<number+1) 
    return false;

  int next = nextColumn[iColumn];
  int last = lastColumn[iColumn];

  //out
  nextColumn[last] = next;
  lastColumn[next] = last;

  CoinBigIndex put = startR[maximumColumnsExtra_];
  //in at end
  last = lastColumn[maximumColumnsExtra_];
  nextColumn[last] = iColumn;
  lastColumn[maximumColumnsExtra_] = iColumn;
  lastColumn[iColumn] = last;
  nextColumn[iColumn] = maximumColumnsExtra_;
  
  //move
  CoinBigIndex get = startR[iColumn];
  startR[iColumn] = put;
  int i = 0;
  for (i=0 ; i < number; i ++ ) {
    elementR[put]= elementR[get];
    indexRowR[put++] = indexRowR[get++];
  }
  //insert
  elementR[put]=value;
  indexRowR[put++]=iRow;
  numberInColumnPlus[iColumn]++;
  //add 4 for luck
  startR[maximumColumnsExtra_] = CoinMin((CoinBigIndex) (put + 4) ,lengthAreaR_);
  return true;
}
/*  getColumnSpaceIterate.  Gets space for one extra U element in Column
    may have to do compression  (returns true)
    also moves existing vector.
    Returns -1 if no memory or where element was put
    Used by replaceRow (turns off R version) */
CoinBigIndex 
CoinFactorization::getColumnSpaceIterate ( int iColumn, double value,
					   int iRow)
{
  if (numberInColumnPlus_.array()) {
    numberInColumnPlus_.conditionalDelete();
  }
  int * numberInRow = numberInRow_.array();
  int * numberInColumn = numberInColumn_.array();
  int * nextColumn = nextColumn_.array();
  int * lastColumn = lastColumn_.array();
  int number = numberInColumn[iColumn];
  int iNext=nextColumn[iColumn];
  CoinBigIndex * startColumnU = startColumnU_.array();
  CoinBigIndex * startRowU = startRowU_.array();
  CoinBigIndex space = startColumnU[iNext]-startColumnU[iColumn];
  CoinBigIndex put;
  CoinBigIndex *convertRowToColumnU = convertRowToColumnU_.array();
  int * indexColumnU = indexColumnU_.array();
  double *elementU = elementU_.array();
  int * indexRowU = indexRowU_.array();
  if ( space < number + 1 ) {
    //see if it can go in at end
    if (lengthAreaU_-startColumnU[maximumColumnsExtra_]<number+1) {
      //compression
      int jColumn = nextColumn[maximumColumnsExtra_];
      CoinBigIndex put = 0;
      while ( jColumn != maximumColumnsExtra_ ) {
	//move
	CoinBigIndex get;
	CoinBigIndex getEnd;

	get = startColumnU[jColumn];
	getEnd = get + numberInColumn[jColumn];
	startColumnU[jColumn] = put;
	CoinBigIndex i;
	for ( i = get; i < getEnd; i++ ) {
	  double value = elementU[i];
	  if (value) {
	    indexRowU[put] = indexRowU[i];
	    elementU[put] = value;
	    put++;
	  } else {
	    numberInColumn[jColumn]--;
	  }
	}
	jColumn = nextColumn[jColumn];
      }
      numberCompressions_++;
      startColumnU[maximumColumnsExtra_]=put;
      //space for cross reference
      CoinBigIndex *convertRowToColumn = convertRowToColumnU_.array();
      CoinBigIndex j = 0;
      CoinBigIndex *startRow = startRowU_.array();
      
      int iRow;
      for ( iRow = 0; iRow < numberRowsExtra_; iRow++ ) {
	startRow[iRow] = j;
	j += numberInRow[iRow];
      }
      factorElements_=j;
      
      CoinZeroN ( numberInRow, numberRowsExtra_ );
      int i;
      for ( i = 0; i < numberRowsExtra_; i++ ) {
	CoinBigIndex start = startColumnU[i];
	CoinBigIndex end = start + numberInColumn[i];

	CoinBigIndex j;
	for ( j = start; j < end; j++ ) {
	  int iRow = indexRowU[j];
	  int iLook = numberInRow[iRow];
	  
	  numberInRow[iRow] = iLook + 1;
	  CoinBigIndex k = startRow[iRow] + iLook;
	  
	  indexColumnU[k] = i;
	  convertRowToColumn[k] = j;
	}
      }
    }
    // Still may not be room (as iColumn was still in)
    if (lengthAreaU_-startColumnU[maximumColumnsExtra_]>=number+1) {
      int next = nextColumn[iColumn];
      int last = lastColumn[iColumn];
      
      //out
      nextColumn[last] = next;
      lastColumn[next] = last;
      
      put = startColumnU[maximumColumnsExtra_];
      //in at end
      last = lastColumn[maximumColumnsExtra_];
      nextColumn[last] = iColumn;
      lastColumn[maximumColumnsExtra_] = iColumn;
      lastColumn[iColumn] = last;
      nextColumn[iColumn] = maximumColumnsExtra_;
      
      //move
      CoinBigIndex get = startColumnU[iColumn];
      startColumnU[iColumn] = put;
      int i = 0;
      for (i=0 ; i < number; i ++ ) {
	double value = elementU[get];
	int iRow=indexRowU[get++];
	if (value) {
	  elementU[put]= value;
	  int n=numberInRow[iRow];
	  CoinBigIndex start = startRowU[iRow];
	  CoinBigIndex j;
	  for (j=start;j<start+n;j++) {
	    if (indexColumnU[j]==iColumn) {
	      convertRowToColumnU[j]=put;
	      break;
	    }
	  }
	  assert (j<start+n);
	  indexRowU[put++] = iRow;
	} else {
	  assert (!numberInRow[iRow]);
	  numberInColumn[iColumn]--;
	}
      }
      //insert
      int n=numberInRow[iRow];
      CoinBigIndex start = startRowU[iRow];
      CoinBigIndex j;
      for (j=start;j<start+n;j++) {
	if (indexColumnU[j]==iColumn) {
	  convertRowToColumnU[j]=put;
	  break;
	}
      }
      assert (j<start+n);
      elementU[put]=value;
      indexRowU[put]=iRow;
      numberInColumn[iColumn]++;
      //add 4 for luck
      startColumnU[maximumColumnsExtra_] = CoinMin((CoinBigIndex) (put + 4) ,lengthAreaU_);
    } else {
      // no room
      put=-1;
    }
  } else {
    // just slot in
    put=startColumnU[iColumn]+numberInColumn[iColumn];
    int n=numberInRow[iRow];
    CoinBigIndex start = startRowU[iRow];
    CoinBigIndex j;
    for (j=start;j<start+n;j++) {
      if (indexColumnU[j]==iColumn) {
	convertRowToColumnU[j]=put;
	break;
      }
    }
    assert (j<start+n);
    elementU[put]=value;
    indexRowU[put]=iRow;
    numberInColumn[iColumn]++;
  }
  return put;
}
// Condition number - product of pivots after factorization
double 
CoinFactorization::conditionNumber() const
{
  double condition = 1.0;
  const double * pivotRegion = pivotRegion_.array();
  for (int i=0;i<numberRows_;i++) {
    condition *= pivotRegion[i];
  }
  condition = CoinMax(fabs(condition),1.0e-50);
  return 1.0/condition;
}
/* Updates one column (FTRAN) from region2
   Tries to do FT update
   number returned is negative if no room.
   Also updates region3
   region1 starts as zero and is zero at end */
int 
CoinFactorization::updateTwoColumnsFT ( CoinIndexedVector * regionSparse1,
					CoinIndexedVector * regionSparse2,
					CoinIndexedVector * regionSparse3,
					bool noPermuteRegion3)
{
#if 1
  //#ifdef NDEBUG
  //#undef NDEBUG
  //#endif
  //#define COIN_DEBUG
#ifdef COIN_DEBUG
  regionSparse1->checkClean();
  CoinIndexedVector save2(*regionSparse2);
  CoinIndexedVector save3(*regionSparse3);
#endif
  CoinIndexedVector * regionFT ;
  CoinIndexedVector * regionUpdate ;
  int *regionIndex ;
  int numberNonZero ;
  const int *permute = permute_.array();
  int * index ;
  double * region ;
  int j;
  if (!noPermuteRegion3) {
    regionFT = regionSparse3;
    regionUpdate = regionSparse1;
    //permute and move indices into index array
    regionIndex = regionUpdate->getIndices (  );
    //int numberNonZero;
    region = regionUpdate->denseVector();
    
    numberNonZero = regionSparse3->getNumElements();
    int * index = regionSparse3->getIndices();
    double * array = regionSparse3->denseVector();
    assert (!regionSparse3->packedMode());
    for ( j = 0; j < numberNonZero; j ++ ) {
      int iRow = index[j];
      double value = array[iRow];
      array[iRow]=0.0;
      iRow = permute[iRow];
      region[iRow] = value;
      regionIndex[j] = iRow;
    }
    regionUpdate->setNumElements ( numberNonZero );
  } else {
    regionFT = regionSparse1;
    regionUpdate = regionSparse3;
  }
  //permute and move indices into index array (in U)
  regionIndex = regionFT->getIndices (  );
  numberNonZero = regionSparse2->getNumElements();
  index = regionSparse2->getIndices();
  region = regionFT->denseVector();
  double * array = regionSparse2->denseVector();
  CoinBigIndex * startColumnU = startColumnU_.array();
  CoinBigIndex start = startColumnU[maximumColumnsExtra_];
  startColumnU[numberColumnsExtra_] = start;
  regionIndex = indexRowU_.array() + start;

  assert(regionSparse2->packedMode());
  for ( j = 0; j < numberNonZero; j ++ ) {
    int iRow = index[j];
    double value = array[j];
    array[j]=0.0;
    iRow = permute[iRow];
    region[iRow] = value;
    regionIndex[j] = iRow;
  }
  regionFT->setNumElements ( numberNonZero );
  if (collectStatistics_) {
    numberFtranCounts_+=2;
    ftranCountInput_ += (double) regionFT->getNumElements()+
    (double) regionUpdate->getNumElements();
  }
    
  //  ******* L
  updateColumnL ( regionFT, regionIndex );
  updateColumnL ( regionUpdate, regionUpdate->getIndices() );
  if (collectStatistics_) 
    ftranCountAfterL_ += (double) regionFT->getNumElements()+
      (double) regionUpdate->getNumElements();
  //permute extra
  //row bits here
  updateColumnRFT ( regionFT, regionIndex );
  updateColumnR ( regionUpdate );
  if (collectStatistics_) 
    ftranCountAfterR_ += (double) regionFT->getNumElements()+
    (double) regionUpdate->getNumElements();
  //  ******* U - see if densish
  // Guess at number at end
  int goSparse=0;
  if (sparseThreshold_>0) {
    int numberNonZero = (regionUpdate->getNumElements (  )+
			 regionFT->getNumElements())>>1;
    if (ftranAverageAfterR_) {
      int newNumber = (int) (numberNonZero*ftranAverageAfterU_);
      if (newNumber< sparseThreshold_)
	goSparse = 2;
      else if (newNumber< sparseThreshold2_)
	goSparse = 1;
    } else {
      if (numberNonZero<sparseThreshold_) 
	goSparse = 2;
    }
  }
#ifndef COIN_FAST_CODE
  assert (slackValue_==-1.0);
#endif
  if (!goSparse&&numberRows_<1000) {
    double *arrayFT = regionFT->denseVector (  );
    int * indexFT = regionFT->getIndices();
    int numberNonZeroFT;
    double *arrayUpdate = regionUpdate->denseVector (  );
    int * indexUpdate = regionUpdate->getIndices();
    int numberNonZeroUpdate;
    updateTwoColumnsUDensish(numberNonZeroFT,arrayFT,indexFT,
			     numberNonZeroUpdate,arrayUpdate,indexUpdate);
    regionFT->setNumElements ( numberNonZeroFT );
    regionUpdate->setNumElements ( numberNonZeroUpdate );
  } else {
    // sparse 
    updateColumnU ( regionFT, regionIndex);
    updateColumnU ( regionUpdate, regionUpdate->getIndices());
  }
  permuteBack(regionFT,regionSparse2);
  if (!noPermuteRegion3) {
    permuteBack(regionUpdate,regionSparse3);
  }
#ifdef COIN_DEBUG
  int n2=regionSparse2->getNumElements();
  regionSparse1->checkClean();
  int n2a= updateColumnFT(regionSparse1,&save2);
  assert(n2==n2a);
  {
    int j;
    double * regionA = save2.denseVector();
    int * indexA = save2.getIndices();
    double * regionB = regionSparse2->denseVector();
    int * indexB = regionSparse2->getIndices();
    for (j=0;j<n2;j++) {
      int k = indexA[j];
      assert (k==indexB[j]);
      double value = regionA[j];
      assert (value==regionB[j]);
    }
  }
  updateColumn(&save3,
	       &save3,
	       noPermuteRegion3);
  int n3=regionSparse3->getNumElements();
  assert (n3==save3.getNumElements());
  {
    int j;
    double * regionA = save3.denseVector();
    int * indexA = save3.getIndices();
    double * regionB = regionSparse3->denseVector();
    int * indexB = regionSparse3->getIndices();
    for (j=0;j<n3;j++) {
      int k = indexA[j];
      assert (k==indexB[j]);
      double value = regionA[k];
      assert (value==regionB[k]);
    }
  }
  //*regionSparse2=save2;
  //*regionSparse3=save3;
  printf("REGION2 %d els\n",regionSparse2->getNumElements());
  regionSparse2->print();
  printf("REGION3 %d els\n",regionSparse3->getNumElements());
  regionSparse3->print();
#endif
  return regionSparse2->getNumElements();
#else
  int returnCode= updateColumnFT(regionSparse1,
				 regionSparse2);
  assert (noPermuteRegion3);
  updateColumn(regionSparse3,
	       regionSparse3,
	       noPermuteRegion3);
  //printf("REGION2 %d els\n",regionSparse2->getNumElements());
  //regionSparse2->print();
  //printf("REGION3 %d els\n",regionSparse3->getNumElements());
  //regionSparse3->print();
  return returnCode;
#endif
}
// Updates part of 2 columns (FTRANU) real work
void
CoinFactorization::updateTwoColumnsUDensish (
					     int & numberNonZero1,
					     double * COIN_RESTRICT region1, 
					     int * COIN_RESTRICT index1,
					     int & numberNonZero2,
					     double * COIN_RESTRICT region2, 
					     int * COIN_RESTRICT index2) const
{
  double tolerance = zeroTolerance_;
  const CoinBigIndex *startColumn = startColumnU_.array();
  const int *indexRow = indexRowU_.array();
  const double *element = elementU_.array();
  int numberNonZeroA = 0;
  int numberNonZeroB = 0;
  const int *numberInColumn = numberInColumn_.array();
  int i;
  const double *pivotRegion = pivotRegion_.array();
  
  for (i = numberU_-1 ; i >= numberSlacks_; i-- ) {
    double pivotValue1 = region1[i];
    double pivotValue2 = region2[i];
    int iSwitch=0;
    if (pivotValue1) {
      region1[i] = 0.0;
      if ( fabs ( pivotValue1 ) > tolerance )
	iSwitch = 1;
    }
    if (pivotValue2) {
      region2[i] = 0.0;
      if ( fabs ( pivotValue2 ) > tolerance )
	iSwitch |= 2;;
    }
    if (iSwitch) {
      CoinBigIndex start = startColumn[i];
      const double * thisElement = element+start;
      const int * thisIndex = indexRow+start;
      CoinBigIndex j;
      //#define NO_LOAD
      switch(iSwitch) {
	// just region 1
      case 1:
	for (j=numberInColumn[i]-1 ; j >=0; j-- ) {
	  int iRow = thisIndex[j];
	  double value = thisElement[j];
#ifdef NO_LOAD
	  region1[iRow] -= value * pivotValue1;
#else
	  double regionValue1 = region1[iRow];
	  region1[iRow] = regionValue1 - value * pivotValue1;
#endif
	}
	pivotValue1 *= pivotRegion[i];
	region1[i]=pivotValue1;
	index1[numberNonZeroA++]=i;
	break;
	// just region 2
      case 2:
	for (j=numberInColumn[i]-1 ; j >=0; j-- ) {
	  int iRow = thisIndex[j];
	  double value = thisElement[j];
#ifdef NO_LOAD
	  region2[iRow] -= value * pivotValue2;
#else
	  double regionValue2 = region2[iRow];
	  region2[iRow] = regionValue2 - value * pivotValue2;
#endif
	}
	pivotValue2 *= pivotRegion[i];
	region2[i]=pivotValue2;
	index2[numberNonZeroB++]=i;
	break;
	// both
      case 3:
	for (j=numberInColumn[i]-1 ; j >=0; j-- ) {
	  int iRow = thisIndex[j];
	  double value = thisElement[j];
#ifdef NO_LOAD
	  region1[iRow] -= value * pivotValue1;
	  region2[iRow] -= value * pivotValue2;
#else
	  double regionValue1 = region1[iRow];
	  double regionValue2 = region2[iRow];
	  region1[iRow] = regionValue1 - value * pivotValue1;
	  region2[iRow] = regionValue2 - value * pivotValue2;
#endif
	}
	pivotValue1 *= pivotRegion[i];
	pivotValue2 *= pivotRegion[i];
	region1[i]=pivotValue1;
	index1[numberNonZeroA++]=i;
	region2[i]=pivotValue2;
	index2[numberNonZeroB++]=i;
	break;
      }
    }
  }
  // Slacks 
    
  for ( i = numberSlacks_-1; i>=0;i--) {
    double value1 = region1[i];
    if ( value1 ) {
      region1[i]=-value1;
      index1[numberNonZeroA]=i;
      if ( fabs(value1) > tolerance ) 
	numberNonZeroA++;
      else 
	region1[i]=0.0;
    }
  }
  numberNonZero1=numberNonZeroA;
    
  for ( i = numberSlacks_-1; i>=0;i--) {
    double value2 = region2[i];
    if ( value2 ) {
      region2[i]=-value2;
      index2[numberNonZeroB]=i;
      if ( fabs(value2) > tolerance ) 
	numberNonZeroB++;
      else 
	region2[i]=0.0;
    }
  }
  numberNonZero2=numberNonZeroB;
}
