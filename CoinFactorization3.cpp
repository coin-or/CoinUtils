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

//:class CoinFactorization.  Deals with Factorization and Updates

//  updateColumn.  Updates one column (FTRAN) when permuted
int
CoinFactorization::updateColumn ( CoinIndexedVector * regionSparse,
                                bool FTUpdate )
{
  double *region = regionSparse->denseVector (  );
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero = regionSparse->getNumElements (  );
  
  //  ******* L
  updateColumnL ( regionSparse );
  //permute extra
  //row bits here
  updateColumnR ( regionSparse );
  bool noRoom = false;
  
  //update counts
  //save in U
  //in at end
  if ( FTUpdate ) {
    //number may be slightly high because of R permutations
    numberNonZero = regionSparse->getNumElements (  );
    int iColumn = numberColumnsExtra_;
    
    //getColumnSpace also moves fixed part
    //getColumnSpace(iColumn,numberNonZero);
    startColumnU_[iColumn] = startColumnU_[maximumColumnsExtra_];
    CoinBigIndex start = startColumnU_[iColumn];
    CoinBigIndex space = lengthAreaU_ - ( start + numberNonZero );
    
    if ( space >= 0 ) {
      int * putIndex = indexRowU_ + start;
      double * putElement = elementU_ + start;
      int i,n=numberNonZero;
      numberNonZero=0;
      for (i=0;i<n;i++) {
        int indexValue = regionIndex[i];
        double value = region[indexValue];
        if (value) {
          putIndex[numberNonZero]=indexValue;
          putElement[numberNonZero++]=value;
        }
      }
      //redo in case packed down
      numberInColumn_[iColumn] = numberNonZero;
      startColumnU_[maximumColumnsExtra_] = start + numberNonZero;
      //  ******* U
      updateColumnU ( regionSparse, &indexRowU_[start], numberNonZero );
      startColumnU_[maximumColumnsExtra_] = start + numberNonZero;
    } else {
      //no room
      noRoom = true;
      //  ******* U
      updateColumnU ( regionSparse, 0, regionSparse->getNumElements (  ) );
      startColumnU_[maximumColumnsExtra_] = start + numberNonZero;
    }       
  } else {
    //  ******* U
    updateColumnU ( regionSparse, 0, regionSparse->getNumElements (  ) );
  }
  numberNonZero = regionSparse->getNumElements (  );
  if ( !noRoom ) {
    return numberNonZero;
  } else {
    return -numberNonZero;
  }       
}
// const version
int
CoinFactorization::updateColumn ( CoinIndexedVector * regionSparse) const
{
  int numberNonZero;
  
  //  ******* L
  updateColumnL ( regionSparse );
  //permute extra
  //row bits here
  updateColumnR ( regionSparse );
  
  //  ******* U
  updateColumnU ( regionSparse, 0, regionSparse->getNumElements (  ) );
  numberNonZero = regionSparse->getNumElements (  );
  return numberNonZero;
}

//  throwAwayColumn.  Throws away incoming column
void
CoinFactorization::throwAwayColumn (  )
{
  int iColumn = numberColumnsExtra_;
  
  numberInColumn_[iColumn] = 0;
}

//  updateColumnL.  Updates part of column (FTRANL)
void
CoinFactorization::updateColumnL ( CoinIndexedVector * regionSparse) const
{
  double *region = regionSparse->denseVector (  );
  int *regionIndex = regionSparse->getIndices (  );
  int number = regionSparse->getNumElements (  );
  int numberNonZero;
  double tolerance = zeroTolerance_;
  
  if (numberL_) {
    numberNonZero = 0;
    int j, k;
    int i , iPivot;
    
    CoinBigIndex *startColumn = startColumnL_;
    int *indexRow = indexRowL_;
    double *element = elementL_;
    if (number<sparseThreshold_) {
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
          nStack=1;
          stack[0]=iPivot;
          next[0]=startColumn[iPivot+1]-1;
          while (nStack) {
            int kPivot,j;
            /* take off stack */
            kPivot=stack[--nStack];
            if (mark[kPivot]!=1) {
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
          }
        } else {
          // just put on list
          regionIndex[numberNonZero++]=iPivot;
        }
      }
      for (i=nList-1;i>=0;i--) {
        iPivot = list[i];
        mark[iPivot]=0;
        //printf("pivot %d %d\n",i,iPivot);
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
    } else {
      int last = baseL_ + numberL_;
      
      // do easy ones
      for (k=0;k<number;k++) {
        iPivot=regionIndex[k];
        if (iPivot<baseL_) 
          regionIndex[numberNonZero++]=iPivot;
      }
      // now others
      for ( i = baseL_; i < last; i++ ) {
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
      //clean up end
      for ( ; i < numberRowsExtra_; i++ ) {
        double pivotValue = region[i];
        if ( fabs(pivotValue) > tolerance ) {
          regionIndex[numberNonZero++] = i;
        } else {
          region[i] = 0.0;
        }       
      }     
      regionSparse->setNumElements ( numberNonZero );
    } 
  }
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

int               
CoinFactorization::replaceColumn ( int pivotRow,
                                 double pivotCheck,
                                 int numberOfElements,
				  int indicesRow[], double elements[],
				  bool checkBeforeModifying)
{
  CoinIndexedVector *region = new CoinIndexedVector;
  region->reserve(numberRowsExtra_ );
  int status;
  
  if (increasingRows_>2) {
    status =
      updateColumn ( region, elements, indicesRow, numberOfElements, true );
  } else {
    status =
      updateColumn ( region, elements, indicesRow, numberOfElements, true );
  }
  if ( status >= 0 ) {
    status = replaceColumn ( region, pivotRow, pivotCheck );
  } else {
    status = 3;
  }       
  delete region;
  
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
    return 3;
  }       
  if ( lengthAreaU_ < startColumnU_[maximumColumnsExtra_] ) {
    return 3;
  }   
  
  int realPivotRow;
  if (increasingRows_>2) {
    realPivotRow = pivotRow;
  } else {
    realPivotRow = pivotColumn_[pivotRow];
  }
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
  // not necessary at present - but take no cahnces for future
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
  if (!checkBeforeModifying) {
    for ( i = start; i < end ; i ++ ) {
      int iColumn = indexColumn[i];
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
  updateColumnTransposeU ( regionSparse );
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
      if ( fabs ( elementU[i] ) > tolerance ) {
	if ( iRow != realPivotRow ) {
	  saveFromU = saveFromU - elementU[i] * region[iRow];
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
#if COIN_DEBUG
  nextRow_[realPivotRow] = 777777;
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
  regionSparse->clear();
  return status;
}

//  updateColumnTranspose.  Updates one column transpose (BTRAN)
int
CoinFactorization::updateColumnTranspose ( CoinIndexedVector * regionSparse,
                                         double vector[],
                                         int index[],
                                         int number ) const
{
  //zero region
  regionSparse->clear (  );
  double *region = regionSparse->denseVector (  );
  int i;
  
  //move indices into index array
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero = number;
  int j;
  int iRow;
  if (increasingRows_ > 2) {
    for ( j = 0; j < number; j ++ ) {
      iRow = index[j];
      double value = vector[iRow];
      vector[iRow]=0.0;
      region[iRow] = value;
      regionIndex[j] = iRow;
    } 
  } else {
    for ( j = 0; j < number; j ++ ) {
      iRow = index[j];
      double value = vector[iRow];
      vector[iRow]=0.0;
      iRow=pivotColumn_[iRow];
      region[iRow] = value;
      regionIndex[j] = iRow;
    } 
  }
  regionSparse->setNumElements ( numberNonZero );
  number =  updateColumnTranspose ( regionSparse );
  if (increasingRows_ > 2 ) {
    for (i=0;i<number;i++) {
      int iRow=regionIndex[i];
      double value = region[iRow];
      region[iRow]=0.0;
      vector[iRow]=value;
      index[i]=iRow;
    }
  } else {
    for (i=0;i<number;i++) {
      int iRow=regionIndex[i];
      double value = region[iRow];
      region[iRow]=0.0;
      iRow=permuteBack_[iRow];
      vector[iRow]=value;
      index[i]=iRow;
    }
  }
  regionSparse->setNumElements(0);
#ifdef COIN_DEBUG
  for (i=0;i<numberRowsExtra_;i++) {
    assert (!region[i]);
  }
#endif
  return number;
}

//  updateColumnTranspose.  Updates one column transpose (BTRAN)
int
CoinFactorization::updateColumnTranspose ( CoinIndexedVector * regionSparse,
					  double vector[]) const
{
  //zero region
  regionSparse->clear (  );
  double *region = regionSparse->denseVector (  );
  int i;
  
  //move indices into index array
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero = 0;
  int j;
  int iRow;
  if (increasingRows_ > 2) {
    for ( j = 0; j < numberRows_; j ++ ) {
      if (vector[j]) {
	double value = vector[j];
	vector[j]=0.0;
	iRow = j;
	region[iRow] = value;
	regionIndex[numberNonZero++] = iRow;
      }
    }
  } else {
    for ( j = 0; j < numberRows_; j ++ ) {
      if (vector[j]) {
	double value = vector[j];
	vector[j]=0.0;
	iRow = pivotColumn_[j];
	region[iRow] = value;
	regionIndex[numberNonZero++] = iRow;
      }
    }
  }
  regionSparse->setNumElements ( numberNonZero );
  int number =  updateColumnTranspose ( regionSparse );
  if (increasingRows_ > 2 ) {
    for (i=0;i<number;i++) {
      iRow=regionIndex[i];
      double value = region[iRow];
      region[iRow]=0.0;
      vector[iRow]=value;
    }
  } else {
    for (i=0;i<number;i++) {
      iRow=regionIndex[i];
      double value = region[iRow];
      region[iRow]=0.0;
      iRow=permuteBack_[iRow];
      vector[iRow]=value;
    }
  }
  regionSparse->setNumElements(0);
#ifdef COIN_DEBUG
  for (i=0;i<numberRowsExtra_;i++) {
    assert (!region[i]);
  }
#endif
  return number;
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
  
  //move indices into index array
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero = number;
  int j;
  int iRow;
  if (increasingRows_ > 2) {
    abort(); // needs coding
  } else {
    for ( j = 0; j < number; j ++ ) {
      iRow = index[j];
      double value = vector[iRow];
      vector[iRow]=0.0;
      iRow=pivotColumn_[iRow];
      region[iRow] = value;
      regionIndex[j] = iRow;
    } 
  }
  regionSparse->setNumElements ( numberNonZero );
  number =  updateColumnTranspose ( regionSparse );
  if (increasingRows_ < 3) {
    for (i=0;i<number;i++) {
      int iRow=regionIndex[i];
      double value = region[iRow];
      region[iRow]=0.0;
      iRow=permuteBack_[iRow];
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

//  updateColumnTranspose.  Updates one column transpose (BTRAN)
//assumes index is sorted i.e. region is correct
int
CoinFactorization::updateColumnTranspose ( CoinIndexedVector * regionSparse ) const
{
  //  ******* U
  // Apply pivot region - could be combined for speed
  int j;
  double *region = regionSparse->denseVector (  );
  int numberNonZero = regionSparse->getNumElements (  );
  double *pivotRegion = pivotRegion_;
  int *regionIndex = regionSparse->getIndices (  );
  
  for ( j = 0; j < numberNonZero; j++ ) {
    int iRow = regionIndex[j];
    region[iRow] *= pivotRegion[iRow];
  }
  updateColumnTransposeU ( regionSparse );
  //numberNonZero=regionSparse->getNumElements();
  //permute extra
  //row bits here
  updateColumnTransposeR ( regionSparse );
  //regionSparse->getNumElements(numberNonZero);
  //  ******* L
  updateColumnTransposeL ( regionSparse );
  return regionSparse->getNumElements (  );
}

//  updateColumnTransposeU.  Updates part of column transpose (BTRANU)
//assumes index is sorted i.e. region is correct
//does not sort by sign
void
CoinFactorization::updateColumnTransposeU ( CoinIndexedVector * regionSparse) const
{
  double *region = regionSparse->denseVector (  );
  int numberNonZero = regionSparse->getNumElements (  );
  double tolerance = zeroTolerance_;
  
  int *regionIndex = regionSparse->getIndices (  );
  
  int i,j;
  
  CoinBigIndex *startRow = startRowU_;
  
  CoinBigIndex *convertRowToColumn = convertRowToColumnU_;
  int *indexColumn = indexColumnU_;
  
  double * element = elementU_;
  int last = numberU_;
  
  double pivotValue;
  
  int *numberInRow = numberInRow_;
  
  if (numberNonZero<sparseThreshold_) {
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
    int k,nStack;
    nList=0;
    for (k=0;k<numberNonZero;k++) {
      nStack=1;
      iPivot=regionIndex[k];
      stack[0]=iPivot;
      next[0]=startRow[iPivot]+numberInRow[iPivot]-1;
      while (nStack) {
        int kPivot,j;
        /* take off stack */
        kPivot=stack[--nStack];
        if (mark[kPivot]!=1) {
          j=next[nStack];
          if (j<startRow[kPivot]) {
            /* finished so mark */
            list[nList++]=kPivot;
            mark[kPivot]=1;
          } else {
            kPivot=indexColumn[j];
            /* put back on stack */
            next[nStack++] --;
	    // could have been zapped - if so ignore
	    CoinBigIndex getElement = convertRowToColumn[j];
	    double value = element[getElement];
            if (value&&!mark[kPivot]) {
              /* and new one */
              stack[nStack]=kPivot;
	      mark[kPivot]=2;
              next[nStack++]=startRow[kPivot]+numberInRow[kPivot]-1;
            }
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
          
          region[iRow] = region[iRow]
            - value * pivotValue;
        }     
        regionIndex[numberNonZero++] = iPivot;
      } else {
        region[iPivot] = 0.0;
      }       
    }       
  } else {
    numberNonZero = 0;
    for (i=0 ; i < last; i++ ) {
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
  }
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}

//  updateColumnTransposeL.  Updates part of column transpose (BTRANL)
void
CoinFactorization::updateColumnTransposeL ( CoinIndexedVector * regionSparse ) const
{
  double *region = regionSparse->denseVector (  );
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero = regionSparse->getNumElements (  );
  double tolerance = zeroTolerance_;
  int base;
  int first = -1;
  
  if (sparseThreshold_) {
    // use row copy of L
    double * element = elementByRowL_;
    CoinBigIndex * startRow = startRowL_;
    int * column = indexColumnL_;
    int i;
    CoinBigIndex j;
    if (numberNonZero<sparseThreshold_) {
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
        nStack=1;
        stack[0]=iPivot;
        next[0]=startRow[iPivot+1]-1;
        while (nStack) {
          int kPivot,j;
          /* take off stack */
          kPivot=stack[--nStack];
          if (mark[kPivot]!=1) {
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
    } else {
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
    }
  } else {
    numberNonZero=0;
    //scan
    for (first=numberRows_-1;first>=0;first--) {
      if (region[first]) 
        break;
    }
    if ( first >= 0 ) {
      base = baseL_;
      CoinBigIndex *startColumn = startColumnL_;
      int *indexRow = indexRowL_;
      double *element = elementL_;
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
  } 
  //set counts
  regionSparse->setNumElements ( numberNonZero );
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
      numberCompressions_++;
    }       /* endwhile */
    startRow[maximumRowsExtra_] = put;
    space = lengthAreaU_ - put;
    if ( space < extraNeeded + number + 2 ) {
      //need more space
      //if we can allocate bigger then do so and copy
      //if not then return so code can start again
      status_ = -99;
      return false;
    }       
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
