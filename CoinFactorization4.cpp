// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinFactorization.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include <cassert>


//  updateColumnU.  Updates part of column (FTRANU)
void
CoinFactorization::updateColumnU ( CoinIndexedVector * regionSparse,
			      int * indices,
			      int numberIn ) const
{
  int numberNonZero = regionSparse->getNumElements (  );


  if (numberNonZero<sparseThreshold_) {
    updateColumnUSparse(regionSparse,indices,numberIn);
  } else {
    updateColumnUDensish ( regionSparse );
  }
}

//  unPermuteTranspose.  takes off permutation vector
//zeroes out region2 if desired
void
CoinFactorization::unPermuteTranspose ( CoinIndexedVector * regionSparse,
				   CoinIndexedVector * regionSparse2,
				   bool erase ) const
{
  //zero region
  regionSparse->clear (  );
  double *region = regionSparse->denseVector (  );
  double *region2 = regionSparse2->denseVector (  );
  int *regionIndex2 = regionSparse2->getIndices (  );
  int numberNonZero2 = regionSparse2->getNumElements (  );
  int *regionIndex = regionSparse->getIndices (  );

  if ( numberNonZero2 ) {
    int *permuteBack = permuteBack_;

    //permute indices and move
    if ( erase ) {
      int j;
      
      for ( j = 0; j <  numberNonZero2 ; j++ ) {
	int iRow = regionIndex2[j];
	int newRow = permuteBack[iRow];

	region[newRow] = region2[iRow];
	region2[iRow] = 0.0;
	regionIndex[j] = newRow;
      }
      regionSparse2->setNumElements ( 0 );
#ifdef COIN_DEBUG
      regionSparse2->checkClean();
#endif
    } else {
      int j;
      
      for ( j = 0; j < numberNonZero2 ; j++ ) {
	int iRow = regionIndex2[j];
	int newRow = permuteBack[iRow];

	region[newRow] = region2[iRow];
	regionIndex[j] = newRow;
      }
    }
    regionSparse->setNumElements (  numberNonZero2 );
  }
}

//  updateColumnUDensish.  Updates part of column (FTRANU)
void
CoinFactorization::updateColumnUDensish ( CoinIndexedVector * regionSparse) const
{

  double *region = regionSparse->denseVector (  );
  double tolerance = zeroTolerance_;
  const CoinBigIndex *startColumn = startColumnU_;
  const int *indexRow = indexRowU_;
  const double *element = elementU_;
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero = 0;
  const int *numberInColumn = numberInColumn_;
  int i;
  const double *pivotRegion = pivotRegion_;

  for (i = numberU_-1 ; i >= numberSlacks_; i-- ) {
    double pivotValue = region[i];
    if ( fabs ( pivotValue ) > tolerance ) {
      CoinBigIndex start = startColumn[i];
      const double * thisElement = element+start;
      const int * thisIndex = indexRow+start;
      
      CoinBigIndex j= numberInColumn[i]-1;;
      for ( ; j >=0; j-- ) {
	int iRow = thisIndex[j];
	double regionValue = region[iRow];
	double value = thisElement[j];
	region[iRow] = regionValue - value * pivotValue;
      }
      region[i] = pivotValue*pivotRegion[i];
      regionIndex[numberNonZero++] = i;
    } else {
      region[i] = 0.0;
    }
  }

  // now do slacks
  double factor = slackValue_;
  if (factor==1.0) {
    for ( i = numberSlacks_-1; i>=0;i--) {
      double value = region[i];
      double absValue = fabs ( value );
      if ( value ) {
	if ( absValue > tolerance ) {
	  regionIndex[numberNonZero++] = i;
	} else {
	  region[i] = 0.0;
	}
      }
    }
  } else {
    assert (factor==-1.0);
    for ( i = numberSlacks_-1; i>=0;i--) {
      double value = -region[i];
      double absValue = fabs ( value );
      if ( value ) {
	if ( absValue > tolerance ) {
	  regionIndex[numberNonZero++] = i;
	  region [i] = value;
	} else {
	  region[i] = 0.0;
	}
      }
    }
  }
  regionSparse->setNumElements ( numberNonZero );
}
//  updateColumnU.  Updates part of column (FTRANU)
void
CoinFactorization::updateColumnUSparse ( CoinIndexedVector * regionSparse,
			      int * indices,
			      int numberIn ) const
{
  int numberNonZero = regionSparse->getNumElements (  );
  int *regionIndex = regionSparse->getIndices (  );
  double *region = regionSparse->denseVector (  );
  double tolerance = zeroTolerance_;
  CoinBigIndex *startColumn = startColumnU_;
  int *indexRow = indexRowU_;
  double *element = elementU_;
  double *pivotRegion = pivotRegion_;
  // use sparse_ as temporary area
  // mark known to be zero
  int * stack = sparse_;  /* pivot */
  int * list = stack + maximumRowsExtra_;  /* final list */
  int * next = list + maximumRowsExtra_;  /* jnext */
  char * mark = (char *) (next + maximumRowsExtra_);
  int nList, nStack;
  int i , iPivot;
#ifdef COIN_DEBUG
  for (i=0;i<maximumRowsExtra_;i++) {
    assert (!mark[i]);
  }
#endif

  startColumn = startColumnU_;
  indexRow = indexRowU_;
  element = elementU_;
  // move slacks to end of stack list
  int * putLast = stack+maximumRowsExtra_;
  int * put = putLast;
  
  int *numberInColumn = numberInColumn_;
  nList = 0;
  if (!indices) 
    indices = regionIndex;
  for (i=0;i<numberIn;i++) {
    iPivot=indices[i];
    if (iPivot>=numberSlacks_) {
      nStack=1;
      stack[0]=iPivot;
      next[0]=startColumn[iPivot]+numberInColumn[iPivot]-1;
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
	      if (kPivot>=numberSlacks_) {
		/* and new one */
		stack[nStack]=kPivot;
		mark[kPivot]=2;
		next[nStack++]=startColumn[kPivot]+numberInColumn[kPivot]-1;
	      } else {
		// slack
		mark[kPivot]=1;
		--put;
		*put=kPivot;
	      }
	    }
	  }
	}
      }
    } else if (!mark[iPivot]) {
      // slack
      --put;
      *put=iPivot;
      mark[iPivot]=1;
    }
  }
  numberNonZero=0;
  for (i=nList-1;i>=0;i--) {
    iPivot = list[i];
    mark[iPivot]=0;
    double pivotValue = region[iPivot];
    if ( fabs ( pivotValue ) > tolerance ) {
      region[iPivot] *= pivotRegion[iPivot];
      regionIndex[numberNonZero++]= iPivot;
      CoinBigIndex start = startColumn[iPivot];
      int number = numberInColumn[iPivot];
      
      CoinBigIndex j;
      for ( j = start; j < start+number; j++ ) {
	double value = element[j];
	int iRow = indexRow[j];

	region[iRow] -=  value * pivotValue;
      }
    } else {
      region[iPivot]=0.0;
    }
  }
  // slacks
  if (slackValue_==1.0) {
    for (;put<putLast;put++) {
      int iPivot = *put;
      mark[iPivot]=0;
      double pivotValue = region[iPivot];
      if ( fabs ( pivotValue ) > tolerance ) {
	regionIndex[numberNonZero++]= iPivot;
      } else {
	region[iPivot]=0.0;
      }
    }
  } else {
    for (;put<putLast;put++) {
      int iPivot = *put;
      mark[iPivot]=0;
      double pivotValue = -region[iPivot];
      if ( fabs ( pivotValue ) > tolerance ) {
	region[iPivot] = pivotValue;
	regionIndex[numberNonZero++]= iPivot;
      } else {
	region[iPivot]=0.0;
      }
    }
  }
  regionSparse->setNumElements ( numberNonZero );
}

//  =
CoinFactorization & CoinFactorization::operator = ( const CoinFactorization & other ) {
  if (this != &other) {    
    gutsOfDestructor();
    gutsOfInitialize(3);
    gutsOfCopy(other);
  }
  return *this;
}
void CoinFactorization::gutsOfCopy(const CoinFactorization &other)
{
  elementU_ = new double [ other.lengthAreaU_ ];
  indexRowU_ = new int [ other.lengthAreaU_ ];
  indexColumnU_ = new int [ other.lengthAreaU_ ];
  convertRowToColumnU_ = new CoinBigIndex [ other.lengthAreaU_ ];
  if (other.sparseThreshold_) {
    elementByRowL_ = new double [ other.lengthAreaL_ ];
    indexColumnL_ = new int [ other.lengthAreaL_ ];
    startRowL_ = new CoinBigIndex [other.numberRows_+1];
  }
  elementL_ = new double [ other.lengthAreaL_ ];
  indexRowL_ = new int [ other.lengthAreaL_ ];
  startColumnL_ = new CoinBigIndex [ other.numberRows_ + 1 ];
  int extraSpace = other.maximumColumnsExtra_ - other.numberColumns_;

  startColumnR_ = new CoinBigIndex [ extraSpace + 1 ];
  startRowU_ = new CoinBigIndex [ other.maximumRowsExtra_ + 1 ];
  numberInRow_ = new int [ other.maximumRowsExtra_ + 1 ];
  nextRow_ = new int [ other.maximumRowsExtra_ + 1 ];
  lastRow_ = new int [ other.maximumRowsExtra_ + 1 ];
  pivotRegion_ = new double [ other.maximumRowsExtra_ + 1 ];
  permuteBack_ = new int [ other.maximumRowsExtra_ + 1 ];
  permute_ = new int [ other.maximumRowsExtra_ + 1 ];
  pivotColumnBack_ = new int [ other.maximumRowsExtra_ + 1 ];
  startColumnU_ = new CoinBigIndex [ other.maximumColumnsExtra_ + 1 ];
  numberInColumn_ = new int [ other.maximumColumnsExtra_ + 1 ];
  pivotColumn_ = new int [ other.maximumColumnsExtra_ + 1 ];
  nextColumn_ = new int [ other.maximumColumnsExtra_ + 1 ];
  lastColumn_ = new int [ other.maximumColumnsExtra_ + 1 ];
  numberTrials_ = other.numberTrials_;
  biggerDimension_ = other.biggerDimension_;
  numberSlacks_ = other.numberSlacks_;
  numberU_ = other.numberU_;
  lengthU_ = other.lengthU_;
  lengthAreaU_ = other.lengthAreaU_;
  numberL_ = other.numberL_;
  baseL_ = other.baseL_;
  lengthL_ = other.lengthL_;
  lengthAreaL_ = other.lengthAreaL_;
  numberR_ = other.numberR_;
  lengthR_ = other.lengthR_;
  lengthAreaR_ = other.lengthAreaR_;
  pivotTolerance_ = other.pivotTolerance_;
  zeroTolerance_ = other.zeroTolerance_;
  slackValue_ = other.slackValue_;
  areaFactor_ = other.areaFactor_;
  numberRows_ = other.numberRows_;
  numberRowsExtra_ = other.numberRowsExtra_;
  maximumRowsExtra_ = other.maximumRowsExtra_;
  numberColumns_ = other.numberColumns_;
  numberColumnsExtra_ = other.numberColumnsExtra_;
  maximumColumnsExtra_ = other.maximumColumnsExtra_;
  maximumPivots_=other.maximumPivots_;
  numberGoodU_ = other.numberGoodU_;
  numberGoodL_ = other.numberGoodL_;
  numberPivots_ = other.numberPivots_;
  messageLevel_ = other.messageLevel_;
  totalElements_ = other.totalElements_;
  factorElements_ = other.factorElements_;
  status_ = other.status_;
  doForrestTomlin_ = other.doForrestTomlin_;
  increasingRows_ = other.increasingRows_;
  sparseThreshold_=other.sparseThreshold_;
  CoinBigIndex space = lengthAreaL_ - lengthL_;

  lengthAreaR_ = space;
  elementR_ = elementL_ + lengthL_;
  indexRowR_ = indexRowL_ + lengthL_;
  //now copy everything
  //assuming numberRowsExtra==numberColumnsExtra
  if (numberRowsExtra_) {
    CoinDisjointCopyN ( other.startRowU_, numberRowsExtra_ + 1, startRowU_ );
    CoinDisjointCopyN ( other.numberInRow_, numberRowsExtra_ + 1, numberInRow_ );
    CoinDisjointCopyN ( other.nextRow_, numberRowsExtra_ + 1, nextRow_ );
    CoinDisjointCopyN ( other.lastRow_, numberRowsExtra_ + 1, lastRow_ );
    CoinDisjointCopyN ( other.pivotRegion_, numberRowsExtra_ + 1, pivotRegion_ );
    CoinDisjointCopyN ( other.permuteBack_, numberRowsExtra_ + 1, permuteBack_ );
    CoinDisjointCopyN ( other.permute_, numberRowsExtra_ + 1, permute_ );
    CoinDisjointCopyN ( other.pivotColumnBack_, numberRowsExtra_ + 1, pivotColumnBack_ );
    CoinDisjointCopyN ( other.startColumnU_, numberRowsExtra_ + 1, startColumnU_ );
    CoinDisjointCopyN ( other.numberInColumn_, numberRowsExtra_ + 1, numberInColumn_ );
    CoinDisjointCopyN ( other.pivotColumn_, numberRowsExtra_ + 1, pivotColumn_ );
    CoinDisjointCopyN ( other.nextColumn_, numberRowsExtra_ + 1, nextColumn_ );
    CoinDisjointCopyN ( other.lastColumn_, numberRowsExtra_ + 1, lastColumn_ );
    CoinDisjointCopyN ( other.startColumnR_ , numberRowsExtra_ - numberColumns_ + 1,
			startColumnR_ );  
    //extra one at end
    startColumnU_[maximumColumnsExtra_] =
      other.startColumnU_[maximumColumnsExtra_];
    nextColumn_[maximumColumnsExtra_] = other.nextColumn_[maximumColumnsExtra_];
    lastColumn_[maximumColumnsExtra_] = other.lastColumn_[maximumColumnsExtra_];
    startRowU_[maximumRowsExtra_] = other.startRowU_[maximumRowsExtra_];
    nextRow_[maximumRowsExtra_] = other.nextRow_[maximumRowsExtra_];
    lastRow_[maximumRowsExtra_] = other.lastRow_[maximumRowsExtra_];
  }
  CoinDisjointCopyN ( other.elementR_, lengthR_, elementR_ );
  CoinDisjointCopyN ( other.indexRowR_, lengthR_, indexRowR_ );
  //row and column copies of U
  int iRow;
  for ( iRow = 0; iRow < numberRowsExtra_; iRow++ ) {
    //row
    CoinBigIndex start = startRowU_[iRow];
    int numberIn = numberInRow_[iRow];

    CoinDisjointCopyN ( other.indexColumnU_ + start, numberIn, indexColumnU_ + start );
    CoinDisjointCopyN (other.convertRowToColumnU_ + start , numberIn,
	     convertRowToColumnU_ + start );
    //column
    start = startColumnU_[iRow];
    numberIn = numberInColumn_[iRow];
    CoinDisjointCopyN ( other.indexRowU_ + start, numberIn, indexRowU_ + start );
    CoinDisjointCopyN ( other.elementU_ + start, numberIn, elementU_ + start );
    CoinDisjointCopyN ( other.startColumnL_, numberRows_ + 1, startColumnL_ );
  }
  // L is contiguous
  CoinDisjointCopyN ( other.elementL_, lengthL_, elementL_ );
  CoinDisjointCopyN ( other.indexRowL_, lengthL_, indexRowL_ );
  if (other.sparseThreshold_) {
    goSparse();
  }
}
//  updateColumnR.  Updates part of column (FTRANR)
void
CoinFactorization::updateColumnR ( CoinIndexedVector * regionSparse ) const
{
  double *region = regionSparse->denseVector (  );
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero = regionSparse->getNumElements (  );

  if ( !numberR_ )
    return;	//return if nothing to do
  double tolerance = zeroTolerance_;

  int * indexRow = indexRowR_;
  double * element = elementR_;
  CoinBigIndex * startColumn = startColumnR_-numberRows_;

  int iRow;
  double pivotValue;

  int i;
  for ( i = numberRows_; i < numberRowsExtra_; i++ ) {
    //move using permute_ (stored in inverse fashion)
    iRow = permute_[i];
    pivotValue = region[iRow];
    //zero out pre-permuted
    region[iRow] = 0.0;

    CoinBigIndex j;
    for ( j = startColumn[i]; j < startColumn[i+1]; j ++ ) {
      double value = element[j];
      int jRow = indexRow[j];
      pivotValue = pivotValue - value * region[jRow];
    }
    if ( fabs ( pivotValue ) > tolerance ) {
      region[i] = pivotValue;
      regionIndex[numberNonZero++] = i;
    } else {
      region[i] = 0.0;
    }
  }
  //set counts
  regionSparse->setNumElements ( numberNonZero );
}

//  updateColumnTransposeR.  Updates part of column (FTRANR)
void
CoinFactorization::updateColumnTransposeR ( CoinIndexedVector * regionSparse ) const
{
  double *region = regionSparse->denseVector (  );
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero = regionSparse->getNumElements (  );
  double tolerance = zeroTolerance_;
  int i;

#ifdef COIN_DEBUG
  regionSparse->checkClean();
#endif
  if (numberNonZero) {
    int last = numberRowsExtra_-1;

    
    int *indexRow = indexRowR_;
    double *element = elementR_;
    CoinBigIndex * startColumn = startColumnR_-numberRows_;
    //move using permute_ (stored in inverse fashion)
    int putRow;
    double pivotValue;
    
    if (numberNonZero < sparseThreshold_) {
      // we can use sparse_ as temporary array
      int * spare = sparse_;
      for (i=0;i<numberNonZero;i++) {
	spare[regionIndex[i]]=i;
      }
      // still need to do in correct order (for now)
      for ( i = last ; i >= numberRows_; i-- ) {
	putRow = permute_[i];
	pivotValue = region[i];
	//zero out  old permuted
	region[i] = 0.0;
	if ( pivotValue ) {
	  CoinBigIndex j;
	  for ( j = startColumn[i]; j < startColumn[i+1]; j++ ) {
	    double value = element[j];
	    int iRow = indexRow[j];
	    double oldValue = region[iRow];
	    double newValue = oldValue - value * pivotValue;
	    if (oldValue) {
	      if (!newValue)
		newValue=1.0e-100;
	      region[iRow]=newValue;
	    } else if (fabs(newValue)>tolerance) {
	      region[iRow] = newValue;
	      spare[iRow]=numberNonZero;
	      regionIndex[numberNonZero++]=iRow;
	    }
	  }
	  region[putRow] = pivotValue;
	  // modify list
	  int position=spare[i];
	  regionIndex[position]=putRow;
	  spare[putRow]=position;
	}
      }
      regionSparse->setNumElements(numberNonZero);
    } else {
      for ( i = last ; i >= numberRows_; i-- ) {
	putRow = permute_[i];
	pivotValue = region[i];
	//zero out  old permuted
	region[i] = 0.0;
	if ( pivotValue ) {
	  CoinBigIndex j;
	  for ( j = startColumn[i]; j < startColumn[i+1]; j++ ) {
	    double value = element[j];
	    int iRow = indexRow[j];
	    region[iRow] -= value * pivotValue;
	  }
	  region[putRow] = pivotValue;
	  //putRow must have been zero before so put on list ??
	  //but can't catch up so will have to do L from end
	  //unless we update lookBtran in replaceColumn - yes
	}
      }
    }
  }
}
/* Updates one column (FTRAN) from region2 and permutes.
   region1 starts as zero
   If increasingRows_>2
   - returns permuted result in region1 and region2 is zero.
   otherwise
   - returns un-permuted result in region2 and region1 is zero */
int CoinFactorization::updateColumn ( CoinIndexedVector * regionSparse,
					     CoinIndexedVector * regionSparse2,
					     bool FTUpdate)
{
  //permute and move indices into index array
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero = regionSparse2->getNumElements();
  int *permute = permute_;
  int * index = regionSparse2->getIndices();
  double * region = regionSparse->denseVector();
  double * array = regionSparse2->denseVector();

  int j;
  for ( j = 0; j < numberNonZero; j ++ ) {
    int iRow = index[j];
    double value = array[iRow];
    array[iRow]=0.0;
    iRow = permute[iRow];
    region[iRow] = value;
    regionIndex[j] = iRow;
  }
  regionSparse->setNumElements ( numberNonZero );
  // will be negative if no room
  int number=updateColumn ( regionSparse, FTUpdate );
  if (increasingRows_>2) {
    // say region2 empty
    regionSparse2->setNumElements(0);
  } else {
    // permute back
    numberNonZero = regionSparse->getNumElements();
    int * permuteBack = pivotColumnBack_;
    for ( j = 0; j < numberNonZero; j ++ ) {
      int iRow = regionIndex[j];
      double value = region[iRow];
      region[iRow]=0.0;
      iRow = permuteBack[iRow];
      array[iRow] = value;
      index[j] = iRow;
    }
    regionSparse->setNumElements(0);
    regionSparse2->setNumElements(numberNonZero);
  }
  return number;
}
/* Updates one column (FTRAN) to/from array
   This assumes user is thinking non-permuted
   - returns un-permuted result in array.
   region starts as zero and is zero at end */
int CoinFactorization::updateColumn ( CoinIndexedVector * regionSparse,
			double array[], //unpacked
			int index[],
			int number,
			bool FTUpdate ) 
{
  //permute and move indices into index array
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero;
  int *permute = permute_;
  double * region = regionSparse->denseVector();

  int j;
  for ( j = 0; j < number; j ++ ) {
    int iRow = index[j];
    double value = array[iRow];
    array[iRow]=0.0;
    iRow = permute[iRow];
    region[iRow] = value;
    regionIndex[j] = iRow;
  }
  regionSparse->setNumElements ( number );
  // if no room will return negative
  numberNonZero = updateColumn ( regionSparse, FTUpdate );
  // permute back
  number = regionSparse->getNumElements();
  int * permuteBack = pivotColumnBack_;
  for ( j = 0; j < number; j ++ ) {
    int iRow = regionIndex[j];
    double value = region[iRow];
    region[iRow]=0.0;
    iRow = permuteBack[iRow];
    array[iRow] = value;
    index[j] = iRow;
  }
  regionSparse->setNumElements(0);
  return numberNonZero;
}

// const version
int CoinFactorization::updateColumn ( CoinIndexedVector * regionSparse,
			double array[], //unpacked
			int index[],
				    int number) const
{
  //permute and move indices into index array
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero;
  int *permute = permute_;
  double * region = regionSparse->denseVector();

  int j;
  for ( j = 0; j < number; j ++ ) {
    int iRow = index[j];
    double value = array[iRow];
    array[iRow]=0.0;
    iRow = permute[iRow];
    region[iRow] = value;
    regionIndex[j] = iRow;
  }
  regionSparse->setNumElements ( number );
  // if no room will return negative
  numberNonZero = updateColumn ( regionSparse );
  // permute back
  number = regionSparse->getNumElements();
  int * permuteBack = pivotColumnBack_;
  for ( j = 0; j < number; j ++ ) {
    int iRow = regionIndex[j];
    double value = region[iRow];
    region[iRow]=0.0;
    iRow = permuteBack[iRow];
    array[iRow] = value;
    index[j] = iRow;
  }
  regionSparse->setNumElements(0);
  return numberNonZero;
}
/* Updates one column (FTRAN) to/from array - no indices
   This assumes user is thinking non-permuted
   - returns un-permuted result in array.
   region starts as zero and is zero at end */
int CoinFactorization::updateColumn ( CoinIndexedVector * regionSparse,
			double array[], //unpacked 
			bool FTUpdate ) 
{
  //permute and move indices into index array
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero=0;
  int *permute = permute_;
  double * region = regionSparse->denseVector();

  int j;
  for ( j = 0; j < numberRows_; j ++ ) {
    if (array[j]) {
      double value = array[j];
      array[j]=0.0;
      int iRow = permute[j];
      region[iRow] = value;
      regionIndex[numberNonZero++] = iRow;
    }
  }
  regionSparse->setNumElements ( numberNonZero );
  // if no room will return negative
  numberNonZero = updateColumn ( regionSparse, FTUpdate );
  // permute back
  int number = regionSparse->getNumElements();
  int * permuteBack = pivotColumnBack_;
  for ( j = 0; j < number; j ++ ) {
    int iRow = regionIndex[j];
    double value = region[iRow];
    region[iRow]=0.0;
    iRow = permuteBack[iRow];
    array[iRow] = value;
  }
  regionSparse->setNumElements(0);
  return numberNonZero;
}

// const version
int CoinFactorization::updateColumn ( CoinIndexedVector * regionSparse,
			double array[] ) const
{
  //permute and move indices into index array
  int *regionIndex = regionSparse->getIndices (  );
  int numberNonZero=0;
  int *permute = permute_;
  double * region = regionSparse->denseVector();

  int j;
  for ( j = 0; j < numberRows_; j ++ ) {
    if (array[j]) {
      double value = array[j];
      array[j]=0.0;
      int iRow = permute[j];
      region[iRow] = value;
      regionIndex[numberNonZero++] = iRow;
    }
  }
  regionSparse->setNumElements ( numberNonZero );
  // if no room will return negative
  numberNonZero = updateColumn ( regionSparse );
  // permute back
  int number = regionSparse->getNumElements();
  int * permuteBack = pivotColumnBack_;
  for ( j = 0; j < number; j ++ ) {
    int iRow = regionIndex[j];
    double value = region[iRow];
    region[iRow]=0.0;
    iRow = permuteBack[iRow];
    array[iRow] = value;
  }
  regionSparse->setNumElements(0);
  return numberNonZero;
}
//  makes a row copy of L
void
CoinFactorization::goSparse ( )
{
  if (!sparseThreshold_&&numberRows_>400)
    sparseThreshold_=(numberRows_-300)/7;
  //sparseThreshold_=99999;
  // allow for stack, list, next and char map of mark
  int nRowIndex = (maximumRowsExtra_+sizeof(int)-1)/
    sizeof(char);
  delete []sparse_;
  sparse_ = new int [ 3*maximumRowsExtra_ + nRowIndex ];
  // zero out mark
  memset(sparse_+3*maximumRowsExtra_,0,maximumRowsExtra_*sizeof(char));
  delete []elementByRowL_;
  delete []startRowL_;
  delete []indexColumnL_;
  elementByRowL_=new double[lengthAreaL_];
  startRowL_=new CoinBigIndex[numberRows_+1];
  indexColumnL_=new int[lengthAreaL_];
  // counts
  memset(startRowL_,0,numberRows_*sizeof(CoinBigIndex));
  int i;
  for (i=baseL_;i<baseL_+numberL_;i++) {
    CoinBigIndex j;
    for (j=startColumnL_[i];j<startColumnL_[i+1];j++) {
      int iRow = indexRowL_[j];
      startRowL_[iRow]++;
    }
  }
  // convert count to lasts
  CoinBigIndex count=0;
  for (i=0;i<numberRows_;i++) {
    int numberInRow=startRowL_[i];
    count += numberInRow;
    startRowL_[i]=count;
  }
  startRowL_[numberRows_]=count;
  // now insert
  for (i=baseL_+numberL_-1;i>=baseL_;i--) {
    CoinBigIndex j;
    for (j=startColumnL_[i];j<startColumnL_[i+1];j++) {
      int iRow = indexRowL_[j];
      CoinBigIndex start = startRowL_[iRow]-1;
      startRowL_[iRow]=start;
      elementByRowL_[start]=elementL_[j];
      indexColumnL_[start]=i;
    }
  }
}
//  get sparse threshold
int
CoinFactorization::sparseThreshold ( ) const
{
  return sparseThreshold_;
}

//  set sparse threshold
void
CoinFactorization::sparseThreshold ( int value ) 
{
  if (value>0&&sparseThreshold_) {
    sparseThreshold_=value;
  } else if (!value&&sparseThreshold_) {
    // delete copy
    sparseThreshold_=0;
    delete []elementByRowL_;
    delete []startRowL_;
    delete []indexColumnL_;
    elementByRowL_=NULL;
    startRowL_=NULL;
    indexColumnL_=NULL;
    delete []sparse_;
    sparse_=NULL;
  } else if (value>0&&!sparseThreshold_) {
    sparseThreshold_=value;
    goSparse();
  }
}
void CoinFactorization::maximumPivots (  int value )
{
  if (value>0) {
    maximumPivots_=value;
  }
}
void CoinFactorization::messageLevel (  int value )
{
  if (value>0&&value<16) {
    messageLevel_=value;
  }
}
void CoinFactorization::pivotTolerance (  double value )
{
  if (value>0.0&&value<=1.0) {
    pivotTolerance_=value;
  }
}
void CoinFactorization::zeroTolerance (  double value )
{
  if (value>0.0&&value<1.0) {
    zeroTolerance_=value;
  }
}
void CoinFactorization::slackValue (  double value )
{
  if (value>=0.0) {
    slackValue_=1.0;
  } else {
    slackValue_=-1.0;
  }
}
