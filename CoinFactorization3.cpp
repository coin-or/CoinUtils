// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <cassert>
#include <cstdio>

#include "CoinFactorization.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include <stdio.h>
#include <iostream>
#if DENSE_CODE==1
// using simple clapack interface
extern "C" int dgetrs_(const char *trans, const int *n, const int *nrhs, 
	const double *a, const int *lda, const int *ipiv, double *b, 
		       const int * ldb, int *info);
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
  
  const CoinBigIndex *startColumn = startColumnL_;
  const int *indexRow = indexRowL_;
  const double *element = elementL_;
  int last = baseL_ + numberL_;
  int smallestIndex = numberRowsExtra_;
  // do easy ones
  for (k=0;k<number;k++) {
    iPivot=regionIndex[k];
    if (iPivot<baseL_) 
      regionIndex[numberNonZero++]=iPivot;
    else
      smallestIndex = min(iPivot,smallestIndex);
  }
  // now others
  for ( i = smallestIndex; i < last; i++ ) {
    double pivotValue = region[i];
    CoinBigIndex start = startColumn[i];
    CoinBigIndex end = startColumn[i + 1];
    
    if ( fabs(pivotValue) > tolerance ) {
      CoinBigIndex j;
      for ( j = start; j < end; j ++ ) {
	int iRow0 = indexRow[j];
	double result0 = region[iRow0];
	double value0 = element[j];

	region[iRow0] = result0 - value0 * pivotValue;
      }     
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
  
  const CoinBigIndex *startColumn = startColumnL_;
  const int *indexRow = indexRowL_;
  const double *element = elementL_;
  int last = baseL_ + numberL_;
#ifdef DENSE_CODE2
  if (numberDense_) {
    //can take out last bit of sparse L as empty
    last = min(last,numberRows_-numberDense_);
  } 
#endif
  
  // use sparse_ as temporary area
  // mark known to be zero
  int * stack = sparse_;  /* pivot */
  int * list = stack + maximumRowsExtra_;  /* final list */
  int * next = list + maximumRowsExtra_;  /* jnext */
  CoinCheckZero * mark = (CoinCheckZero *) (next + maximumRowsExtra_);
  int nMarked=0;
  int smallestIndex = numberRowsExtra_;
  // do easy ones
  for (k=0;k<number;k++) {
    iPivot=regionIndex[k];
    if (iPivot<baseL_) { 
      regionIndex[numberNonZero++]=iPivot;
    } else {
      smallestIndex = min(iPivot,smallestIndex);
      int iWord = iPivot>>CHECK_SHIFT;
      int iBit = iPivot-(iWord<<CHECK_SHIFT);
      if (mark[iWord]) {
	mark[iWord] |= 1<<iBit;
      } else {
	mark[iWord] = 1<<iBit;
	stack[nMarked++]=iWord;
      }
    }
  }
  // now others
  // First do up to convenient power of 2
  int jLast = (smallestIndex+BITS_PER_CHECK-1)>>CHECK_SHIFT;
  jLast = min((jLast<<CHECK_SHIFT),last);
  for ( i = smallestIndex; i < jLast; i++ ) {
    double pivotValue = region[i];
    CoinBigIndex start = startColumn[i];
    CoinBigIndex end = startColumn[i + 1];
    
    if ( fabs(pivotValue) > tolerance ) {
      CoinBigIndex j;
      for ( j = start; j < end; j ++ ) {
	int iRow0 = indexRow[j];
	double result0 = region[iRow0];
	double value0 = element[j];
	region[iRow0] = result0 - value0 * pivotValue;
	int iWord = iRow0>>CHECK_SHIFT;
	int iBit = iRow0-(iWord<<CHECK_SHIFT);
	if (mark[iWord]) {
	  mark[iWord] |= 1<<iBit;
	} else {
	  mark[iWord] = 1<<iBit;
	  stack[nMarked++]=iWord;
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
	      int iRow0 = indexRow[j];
	      double result0 = region[iRow0];
	      double value0 = element[j];
	      region[iRow0] = result0 - value0 * pivotValue;
	      int iWord = iRow0>>CHECK_SHIFT;
	      int iBit = iRow0-(iWord<<CHECK_SHIFT);
	      if (mark[iWord]) {
		mark[iWord] |= 1<<iBit;
	      } else {
		mark[iWord] = 1<<iBit;
		stack[nMarked++]=iWord;
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
	int iRow0 = indexRow[j];
	double result0 = region[iRow0];
	double value0 = element[j];
	region[iRow0] = result0 - value0 * pivotValue;
	int iWord = iRow0>>CHECK_SHIFT;
	int iBit = iRow0-(iWord<<CHECK_SHIFT);
	if (mark[iWord]) {
	  mark[iWord] |= 1<<iBit;
	} else {
	  mark[iWord] = 1<<iBit;
	  stack[nMarked++]=iWord;
	}
      }     
      regionIndex[numberNonZero++] = i;
    } else {
      region[i] = 0.0;
    }       
  }
  // zero out ones that might have been skipped
  mark[smallestIndex>>CHECK_SHIFT]=0;
  mark[kLast]=0;
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
  int i , iPivot;
  
  const CoinBigIndex *startColumn = startColumnL_;
  const int *indexRow = indexRowL_;
  const double *element = elementL_;
  // use sparse_ as temporary area
  // mark known to be zero
  int * stack = sparse_;  /* pivot */
  int * list = stack + maximumRowsExtra_;  /* final list */
  int * next = list + maximumRowsExtra_;  /* jnext */
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
    iPivot=regionIndex[k];
    if (iPivot>=baseL_) {
      if(!mark[iPivot]) {
	stack[0]=iPivot;
	int j=startColumn[iPivot+1]-1;
	if (j>=startColumn[iPivot]) {
	  int kPivot=indexRow[j];
	  /* put back on stack */
	  next[0] =j-1;
	  /* and new one */
	  if (!mark[kPivot]) {
	    stack[1]=kPivot;
	    mark[kPivot]=2;
	    next[1]=startColumn[kPivot+1]-1;
	    nStack=2;
	  } else {
	    nStack=1;
	  }
	  while (nStack) {
	    int kPivot,j;
	    /* take off stack */
	    kPivot=stack[--nStack];
	    j=next[nStack];
	    if (j<startColumn[kPivot]) {
	      /* finished so mark */
	      list[nList++]=kPivot;
	      mark[kPivot]=1;
	    } else {
	      kPivot=indexRow[j];
	      /* put back on stack */
	      next[nStack++] --;
	      if (!mark[kPivot]) {
		/* and new one */
		stack[nStack]=kPivot;
		mark[kPivot]=2;
		next[nStack++]=startColumn[kPivot+1]-1;
	      }
	    }
	  }
	} else {
	  // nothing there - just put on list
	  list[nList++]=iPivot;
	  mark[iPivot]=1;
	}
      }
    } else {
      // just put on list
      regionIndex[numberNonZero++]=iPivot;
    }
  }
  for (i=nList-1;i>=0;i--) {
    iPivot = list[i];
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
      dgetrs_(&trans,&numberDense_,&ione,denseArea_,&numberDense_,
	      densePermute_,region+lastSparse,&numberDense_,&info);
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
  if ( fabs ( saveFromU ) > 1.0e-7 ) {
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
  CoinBigIndex *startColumn;
  int *indexRow;
  double *element;
  
  //return at once if too many iterations
  if ( numberColumnsExtra_ >= maximumColumnsExtra_ ) {
    return 5;
  }       
  if ( lengthAreaU_ < startColumnU_[maximumColumnsExtra_] ) {
    return 3;
  }   
  
  int realPivotRow;
  realPivotRow = pivotColumn_[pivotRow];
  //zeroed out region
  double *region = regionSparse->denseVector (  );
  
  element = elementU_;
  //take out old pivot column

  // If we have done no pivots then always check before modification
  if (!numberPivots_)
    checkBeforeModifying=true;
  
  totalElements_ -= numberInColumn_[realPivotRow];
  double oldPivot = pivotRegion_[realPivotRow];
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
      if (pivotRegion_[i])
      printf("%d %g\n",i,pivotRegion_[i]);
  }*/
  }   
#endif
  pivotRegion_[realPivotRow] = 0.0;
  CoinBigIndex i;

  CoinBigIndex saveEnd = startColumnU_[realPivotRow]
                         + numberInColumn_[realPivotRow];
  // not necessary at present - but take no chances for future
  numberInColumn_[realPivotRow] = 0;
  //get entries in row (pivot not stored)
  CoinBigIndex *startRow = startRowU_;
  CoinBigIndex start;
  CoinBigIndex end;

  start = startRow[realPivotRow];
  end = start + numberInRow_[realPivotRow];
  int numberNonZero = 0;
  int *indexColumn = indexColumnU_;
  CoinBigIndex *convertRowToColumn = convertRowToColumnU_;
  int *regionIndex = regionSparse->getIndices (  );
  
#if COIN_DEBUG>1
  if (numberR_>=checkNumber) 
    printf("Before btranu\n");
#endif
  int smallestIndex=numberRowsExtra_;
  if (!checkBeforeModifying) {
    for ( i = start; i < end ; i ++ ) {
      int iColumn = indexColumn[i];
      smallestIndex = min(smallestIndex,iColumn);
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
      smallestIndex = min(smallestIndex,iColumn);
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

  CoinBigIndex startU = startColumnU_[numberColumnsExtra_];
  int *indexU = &indexRowU_[startU];
  double *elementU = &elementU_[startU];
  

  // Do accuracy test here if caller is paranoid
  if (checkBeforeModifying) {
    double tolerance = zeroTolerance_;
    int number = numberInColumn_[numberColumnsExtra_];
  
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
      pivotRegion_[realPivotRow] = oldPivot;
      number = saveEnd-startColumnU_[realPivotRow];
      totalElements_ += number;
      numberInColumn_[realPivotRow]=number;
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
  for ( i = startColumnU_[realPivotRow]; i < saveEnd ; i ++ ) {
    element[i] = 0.0;
  }       
  //zero out pivot Row (before or after?)
  //add to R
  startColumn = startColumnR_;
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
  int next = nextRow_[realPivotRow];
  int last = lastRow_[realPivotRow];
  
  nextRow_[last] = next;
  lastRow_[next] = last;
  numberInRow_[realPivotRow]=0;
#if COIN_DEBUG
  nextRow_[realPivotRow] = 777777;
  lastRow_[realPivotRow] = 777777;
#endif
  //do permute
  permute_[numberRowsExtra_] = realPivotRow;
  permuteBack_[numberRowsExtra_] = -1;
  //and for safety
  permute_[numberRowsExtra_ + 1] = 0;

  pivotColumn_[pivotRow] = numberRowsExtra_;
  pivotColumnBack_[numberRowsExtra_] = pivotRow;
  startColumn = startColumnU_;
  indexRow = indexRowU_;
  element = elementU_;

  numberU_++;
  number = numberInColumn_[numberColumnsExtra_];

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
        int next = nextRow_[iRow];
        int numberInRow = numberInRow_[iRow];
        CoinBigIndex space;
        CoinBigIndex put = startRow[iRow] + numberInRow;
        
        space = startRow[next] - put;
        if ( space <= 0 ) {
          getRowSpaceIterate ( iRow, numberInRow + 4 );
          put = startRow[iRow] + numberInRow;
        }     
        indexColumn[put] = numberColumnsExtra_;
        convertRowToColumn[put] = i + startU;
        numberInRow_[iRow] = numberInRow + 1;
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
  last = lastRow_[maximumRowsExtra_];
  nextRow_[last] = numberRowsExtra_;
  lastRow_[maximumRowsExtra_] = numberRowsExtra_;
  lastRow_[numberRowsExtra_] = last;
  nextRow_[numberRowsExtra_] = maximumRowsExtra_;
  startRow[numberRowsExtra_] = startRow[maximumRowsExtra_];
  numberInRow_[numberRowsExtra_] = 0;
  //column in at beginning (as empty)
  next = nextColumn_[maximumColumnsExtra_];
  lastColumn_[next] = numberColumnsExtra_;
  nextColumn_[maximumColumnsExtra_] = numberColumnsExtra_;
  nextColumn_[numberColumnsExtra_] = next;
  lastColumn_[numberColumnsExtra_] = maximumColumnsExtra_;
  //check accuracy
  int status = checkPivot(saveFromU,pivotCheck);

  if (status!=2) {
  
    double pivotValue = 1.0 / saveFromU;
    
    pivotRegion_[numberRowsExtra_] = pivotValue;
    //modify by pivot
    for ( i = 0; i < number; i++ ) {
      elementU[i] *= pivotValue;
    }       
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
  if (numberInColumnPlus_&&status<2) {
    // we are going to put another copy of R in R
    double * elementR = elementR_ + lengthAreaR_;
    int * indexRowR = indexRowR_ + lengthAreaR_;
    int * startR = startColumnR_+maximumPivots_+1;
    int pivotRow = numberRowsExtra_-1;
    for ( i = 0; i < numberNonZero; i++ ) {
      int iRow = regionIndex[i];
      assert (pivotRow>iRow);
      next = nextColumn_[iRow];
      int space;
      if (next!=maximumColumnsExtra_)
	space = startR[next]-startR[iRow];
      else
	space = lengthAreaR_-startR[iRow];
      int numberInR = numberInColumnPlus_[iRow];
      if (space>numberInR) {
	// there is space
	int put=startR[iRow]+numberInR;
	numberInColumnPlus_[iRow]=numberInR+1;
	indexRowR[put]=pivotRow;
	elementR[put]=region[iRow];
	//add 4 for luck
	if (next==maximumColumnsExtra_)
	  startR[maximumColumnsExtra_] = min(put + 4,lengthAreaR_);
      } else {
	// no space - do we shuffle?
	if (!getColumnSpaceIterate(iRow,region[iRow],pivotRow)) {
	  // printf("Need more space for R\n");
	  delete [] numberInColumnPlus_;
	  numberInColumnPlus_=NULL;
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
  int number = regionSparse2->getNumElements();
  int i;
  const int * pivotColumn = pivotColumn_;
  
  //move indices into index array
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero = number;
  int iRow;
  bool packed = regionSparse2->packedMode();
  if (packed) {
    for ( i = 0; i < number; i ++ ) {
      iRow = index[i];
      double value = vector[i];
      vector[i]=0.0;
      iRow=pivotColumn[iRow];
      region[iRow] = value;
      regionIndex[i] = iRow;
    }
  } else {
    for ( i = 0; i < number; i ++ ) {
      iRow = index[i];
      double value = vector[iRow];
      vector[iRow]=0.0;
      iRow=pivotColumn[iRow];
      region[iRow] = value;
      regionIndex[i] = iRow;
    }
  }
  regionSparse->setNumElements ( numberNonZero );
  //  ******* U
  // Apply pivot region - could be combined for speed
  int j;
  double *pivotRegion = pivotRegion_;
  
  if (collectStatistics_) {
    numberBtranCounts_++;
    btranCountInput_ += (double) numberNonZero;
  }

  int smallestIndex=numberRowsExtra_;
  for ( j = 0; j < numberNonZero; j++ ) {
    int iRow = regionIndex[j];
    smallestIndex = min (smallestIndex,iRow);
    region[iRow] *= pivotRegion[iRow];
  }
  updateColumnTransposeU ( regionSparse,smallestIndex );
  if (collectStatistics_) 
    btranCountAfterU_ += (double) regionSparse->getNumElements();
  //numberNonZero=regionSparse->getNumElements();
  //permute extra
  //row bits here
  double countBefore = btranCountAfterR_;
  updateColumnTransposeR ( regionSparse );
  // Done in updateColumnTransposeR
  //May have lost counts
  numberNonZero = regionSparse->getNumElements (  );
  //  ******* L
  updateColumnTransposeL ( regionSparse );
  // May have lost counts
  if (collectStatistics_) 
    if (numberNonZero<=numberRows_)
      btranCountAfterL_ += (double) regionSparse->getNumElements();
    else
      btranCountAfterL_ += btranCountAfterR_-countBefore;
  number = regionSparse->getNumElements (  );
  const int * permuteBack = permuteBack_;
  if (packed) {
    for (i=0;i<number;i++) {
      int iRow=regionIndex[i];
      double value = region[iRow];
      region[iRow]=0.0;
      iRow=permuteBack[iRow];
      vector[i]=value;
      index[i]=iRow;
    }
  } else {
    for (i=0;i<number;i++) {
      int iRow=regionIndex[i];
      double value = region[iRow];
      region[iRow]=0.0;
      iRow=permuteBack[iRow];
      vector[iRow]=value;
      index[i]=iRow;
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
  
  const CoinBigIndex *startRow = startRowU_;
  
  const CoinBigIndex *convertRowToColumn = convertRowToColumnU_;
  const int *indexColumn = indexColumnU_;
  
  const double * element = elementU_;
  int last = numberU_;
  
  double pivotValue;
  
  const int *numberInRow = numberInRow_;
  
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
  
  const CoinBigIndex *startRow = startRowU_;
  
  const CoinBigIndex *convertRowToColumn = convertRowToColumnU_;
  const int *indexColumn = indexColumnU_;
  
  const double * element = elementU_;
  int last = numberU_;
  
  double pivotValue;
  
  const int *numberInRow = numberInRow_;
  
  // use sparse_ as temporary area
  // mark known to be zero
  int * stack = sparse_;  /* pivot */
  int * list = stack + maximumRowsExtra_;  /* final list */
  int * next = list + maximumRowsExtra_;  /* jnext */
  CoinCheckZero * mark = (CoinCheckZero *) (next + maximumRowsExtra_);
  int nMarked=0;

  for (i=0;i<numberNonZero;i++) {
    int iPivot=regionIndex[i];
    int iWord = iPivot>>CHECK_SHIFT;
    int iBit = iPivot-(iWord<<CHECK_SHIFT);
    if (mark[iWord]) {
      mark[iWord] |= 1<<iBit;
    } else {
      mark[iWord] = 1<<iBit;
      stack[nMarked++]=iWord;
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
	      stack[nMarked++]=iWord;
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
  
  const CoinBigIndex *startRow = startRowU_;
  
  const CoinBigIndex *convertRowToColumn = convertRowToColumnU_;
  const int *indexColumn = indexColumnU_;
  
  const double * element = elementU_;
  
  double pivotValue;
  
  int *numberInRow = numberInRow_;
  
  // use sparse_ as temporary area
  // mark known to be zero
  int * stack = sparse_;  /* pivot */
  int * list = stack + maximumRowsExtra_;  /* final list */
  int * next = list + maximumRowsExtra_;  /* jnext */
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
      int krs = startRow[i];
      int kre = krs + numberInRow[i];
      int k;
      for (k=krs;k<kre;k++)
	assert (indexColumn[k]>i);
    }
  }
#endif
  int k,nStack;
  nList=0;
  for (k=0;k<numberNonZero;k++) {
    iPivot=regionIndex[k];
    if(!mark[iPivot]) {
      stack[0]=iPivot;
      mark[iPivot]=1;
      int j=startRow[iPivot]+numberInRow[iPivot];
      if (j>startRow[iPivot]) {
	int kPivot=indexColumn[--j];
	/* put back on stack */
	next[0] =j;
	/* and new one */
	if (!mark[kPivot]) {
	  stack[1]=kPivot;
	  mark[kPivot]=1;
	  next[1]=startRow[kPivot]+numberInRow[kPivot];
	  nStack=1;
	} else {
	  nStack=0;
	}
	while (nStack>=0) {
	  int kPivot,j;
	  /* take off stack */
	  kPivot=stack[nStack];
	  j=next[nStack];
	  // Could change to while
#if 0
	  if (j==startRow[kPivot]) {
	    /* finished */
	    --nStack;
	    list[nList++]=kPivot;
	  } else {
	    kPivot=indexColumn[--j];
	    /* put back on stack */
	    next[nStack] =j;
	    if (!mark[kPivot]) {
	      /* and new one */
	      stack[++nStack]=kPivot;
	      mark[kPivot]=1;
	      next[nStack]=startRow[kPivot]+numberInRow[kPivot];
	    }
	  }
#else
	  bool finished=true;
	  while (j!=startRow[kPivot]) {
	    int jPivot=indexColumn[--j];
	    if (!mark[jPivot]) {
	      /* put back on stack */
	      next[nStack] =j;
	      /* and new one */
	      stack[++nStack]=jPivot;
	      mark[jPivot]=1;
	      next[nStack]=startRow[jPivot]+numberInRow[jPivot];
	      finished=false;
	      break;
	    }
	  }
	  if (finished) {
	    --nStack;
	    list[nList++]=kPivot;
	  }
#endif
	}
      } else {
	// nothing there - just put on list
	list[nList++]=iPivot;
	mark[iPivot]=1;
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

	region[iRow] = region[iRow]
	  - value * pivotValue;
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
    const CoinBigIndex *startColumn = startColumnL_;
    const int *indexRow = indexRowL_;
    const double *element = elementL_;
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
  const double * element = elementByRowL_;
  const CoinBigIndex * startRow = startRowL_;
  const int * column = indexColumnL_;
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
  const double * element = elementByRowL_;
  const CoinBigIndex * startRow = startRowL_;
  const int * column = indexColumnL_;
  int i;
  CoinBigIndex j;
  // use sparse_ as temporary area
  // mark known to be zero
  int * stack = sparse_;  /* pivot */
  int * list = stack + maximumRowsExtra_;  /* final list */
  int * next = list + maximumRowsExtra_;  /* jnext */
  CoinCheckZero * mark = (CoinCheckZero *) (next + maximumRowsExtra_);
  int nMarked=0;
#if 1
  for (i=0;i<numberNonZero;i++) {
    int iPivot=regionIndex[i];
    int iWord = iPivot>>CHECK_SHIFT;
    int iBit = iPivot-(iWord<<CHECK_SHIFT);
    if (mark[iWord]) {
      mark[iWord] |= 1<<iBit;
    } else {
      mark[iWord] = 1<<iBit;
      stack[nMarked++]=iWord;
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
	  stack[nMarked++]=iWord;
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
	      stack[nMarked++]=iWord;
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
#else
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
  const double * element = elementByRowL_;
  const CoinBigIndex * startRow = startRowL_;
  const int * column = indexColumnL_;
  int i;
  CoinBigIndex j;
  // use sparse_ as temporary area
  // mark known to be zero
  int * stack = sparse_;  /* pivot */
  int * list = stack + maximumRowsExtra_;  /* final list */
  int * next = list + maximumRowsExtra_;  /* jnext */
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
    iPivot=regionIndex[k];
    if(!mark[iPivot]) {
      stack[0]=iPivot;
      int j=startRow[iPivot+1]-1;
      if (j>=startRow[iPivot]) {
	int kPivot=column[j];
	/* put back on stack */
	next[0] =j-1;
	/* and new one */
	if (!mark[kPivot]) {
	  stack[1]=kPivot;
	  mark[kPivot]=2;
	  next[1]=startRow[kPivot+1]-1;
	  nStack=2;
	} else {
	  nStack=1;
	}
	while (nStack) {
	  int kPivot,j;
	  /* take off stack */
	  kPivot=stack[--nStack];
	  j=next[nStack];
	  if (j<startRow[kPivot]) {
	    /* finished so mark */
	    list[nList++]=kPivot;
	    mark[kPivot]=1;
	  } else {
	    kPivot=column[j];
	    /* put back on stack */
	    next[nStack++] --;
	    if (!mark[kPivot]) {
	      /* and new one */
	      stack[nStack]=kPivot;
	      mark[kPivot]=2;
	      next[nStack++]=startRow[kPivot+1]-1;
	    }
	  }
	}
      } else {
	// nothing there - just put on list
	list[nList++]=iPivot;
	mark[iPivot]=1;
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
  if (!numberL_&&!numberDense_)
    return;
  int goSparse;
  // Guess at number at end
  // we may need to rethink on dense
  if (sparseThreshold_>0) {
    int number = regionSparse->getNumElements (  );
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
      dgetrs_(&trans,&numberDense_,&ione,denseArea_,&numberDense_,
	     densePermute_,region+lastSparse,&numberDense_,&info);
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
  int number = numberInRow_[iRow];
  CoinBigIndex *startRow = startRowU_;
  int *indexColumn = indexColumnU_;
  CoinBigIndex *convertRowToColumn = convertRowToColumnU_;
  CoinBigIndex space = lengthAreaU_ - startRow[maximumRowsExtra_];
  if ( space < extraNeeded + number + 2 ) {
    //compression
    int iRow = nextRow_[maximumRowsExtra_];
    CoinBigIndex put = 0;
    while ( iRow != maximumRowsExtra_ ) {
      //move
      CoinBigIndex get = startRow[iRow];
      CoinBigIndex getEnd = startRow[iRow] + numberInRow_[iRow];
      
      startRow[iRow] = put;
      CoinBigIndex i;
      for ( i = get; i < getEnd; i++ ) {
	indexColumn[put] = indexColumn[i];
	convertRowToColumn[put] = convertRowToColumn[i];
	put++;
      }       
      iRow = nextRow_[iRow];
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
  int next = nextRow_[iRow];
  int last = lastRow_[iRow];
  
  //out
  nextRow_[last] = next;
  lastRow_[next] = last;
  //in at end
  last = lastRow_[maximumRowsExtra_];
  nextRow_[last] = iRow;
  lastRow_[maximumRowsExtra_] = iRow;
  lastRow_[iRow] = last;
  nextRow_[iRow] = maximumRowsExtra_;
  //move
  CoinBigIndex get = startRow[iRow];
  
  startRow[iRow] = put;
  while ( number ) {
    number--;
    indexColumnU_[put] = indexColumnU_[get];
    convertRowToColumn[put] = convertRowToColumn[get];
    put++;
    get++;
  }       /* endwhile */
  //add four for luck
  startRow[maximumRowsExtra_] = put + extraNeeded + 4;
  return true;
}

//  getColumnSpaceIterate.  Gets space for one extra R elemnent in Column
//may have to do compression  (returns true)
//also moves existing vector
bool
CoinFactorization::getColumnSpaceIterate ( int iColumn, double value,
					   int iRow)
{
  double * elementR = elementR_ + lengthAreaR_;
  int * indexRowR = indexRowR_ + lengthAreaR_;
  int * startR = startColumnR_+maximumPivots_+1;
  int number = numberInColumnPlus_[iColumn];
  //see if it can go in at end
  if (lengthAreaR_-startR[maximumColumnsExtra_]<number+1) {
    //compression
    int jColumn = nextColumn_[maximumColumnsExtra_];
    CoinBigIndex put = 0;
    while ( jColumn != maximumColumnsExtra_ ) {
      //move
      CoinBigIndex get;
      CoinBigIndex getEnd;

      get = startR[jColumn];
      getEnd = get + numberInColumnPlus_[jColumn];
      startR[jColumn] = put;
      CoinBigIndex i;
      for ( i = get; i < getEnd; i++ ) {
	indexRowR[put] = indexRowR[i];
	elementR[put] = elementR[i];
	put++;
      }
      jColumn = nextColumn_[jColumn];
    }
    numberCompressions_++;
    startR[maximumColumnsExtra_]=put;
  }
  // Still may not be room (as iColumn was still in)
  if (lengthAreaR_-startR[maximumColumnsExtra_]<number+1) 
    return false;

  int next = nextColumn_[iColumn];
  int last = lastColumn_[iColumn];

  //out
  nextColumn_[last] = next;
  lastColumn_[next] = last;

  CoinBigIndex put = startR[maximumColumnsExtra_];
  //in at end
  last = lastColumn_[maximumColumnsExtra_];
  nextColumn_[last] = iColumn;
  lastColumn_[maximumColumnsExtra_] = iColumn;
  lastColumn_[iColumn] = last;
  nextColumn_[iColumn] = maximumColumnsExtra_;
  
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
  numberInColumnPlus_[iColumn]++;
  //add 4 for luck
  startR[maximumColumnsExtra_] = min(put + 4,lengthAreaR_);
  return true;
}
