// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinUtilsConfig.h"

#include <cassert>
#include "CoinFactorization.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPackedMatrix.hpp"
#include <stdio.h>
//:class CoinFactorization.  Deals with Factorization and Updates
//  CoinFactorization.  Constructor
CoinFactorization::CoinFactorization (  )
{
  gutsOfInitialize(7);
}

/// Copy constructor 
CoinFactorization::CoinFactorization ( const CoinFactorization &other)
{
  gutsOfInitialize(3);
  gutsOfCopy(other);
}
/// The real work of constructors etc
void CoinFactorization::gutsOfDestructor()
{
  delete [] elementU_ ;
  delete [] startRowU_ ;
  delete [] convertRowToColumnU_ ;
  delete [] indexRowU_ ;
  delete [] indexColumnU_ ;
  delete [] startColumnU_ ;
  delete [] elementL_ ;
  delete [] indexRowL_ ;
  delete [] startColumnL_ ;
  delete [] startColumnR_ ;
  delete [] numberInRow_ ;
  delete [] numberInColumn_ ;
  delete [] numberInColumnPlus_ ;
  delete [] pivotColumn_ ;
  delete [] pivotColumnBack_ ;
  delete [] firstCount_ ;
  delete [] nextCount_ ;
  delete [] lastCount_ ;
  delete [] permute_ ;
  delete [] permuteBack_ ;
  delete [] nextColumn_ ;
  delete [] lastColumn_ ;
  delete [] nextRow_ ;
  delete [] lastRow_ ;
  delete [] saveColumn_ ;
  delete [] markRow_ ;
  delete [] pivotRowL_ ;
  delete [] pivotRegion_ ;
  delete [] elementByRowL_ ;
  delete [] startRowL_ ;
  delete [] indexColumnL_ ;
  delete [] sparse_;
  delete [] denseArea_;
  delete [] densePermute_;

  elementU_ = NULL;
  startRowU_ = NULL;
  convertRowToColumnU_ = NULL;
  indexRowU_ = NULL;
  indexColumnU_ = NULL;
  startColumnU_ = NULL;
  elementL_ = NULL;
  indexRowL_ = NULL;
  startColumnL_ = NULL;
  startColumnR_ = NULL;
  numberInRow_ = NULL;
  numberInColumn_ = NULL;
  numberInColumnPlus_ = NULL;
  pivotColumn_ = NULL;
  pivotColumnBack_ = NULL;
  firstCount_ = NULL;
  nextCount_ = NULL;
  lastCount_ = NULL;
  permute_ = NULL;
  permuteBack_ = NULL;
  nextColumn_ = NULL;
  lastColumn_ = NULL;
  nextRow_ = NULL;
  lastRow_ = NULL;
  saveColumn_ = NULL;
  markRow_ = NULL;
  pivotRowL_ = NULL;
  pivotRegion_ = NULL;
  elementByRowL_ = NULL;
  startRowL_ = NULL;
  indexColumnL_ = NULL;
  sparse_= NULL;
  numberCompressions_ = 0;
  biggerDimension_ = 0;
  numberRows_ = 0;
  numberRowsExtra_ = 0;
  maximumRowsExtra_ = 0;
  numberColumns_ = 0;
  numberColumnsExtra_ = 0;
  maximumColumnsExtra_ = 0;
  numberGoodU_ = 0;
  numberGoodL_ = 0;
  totalElements_ = 0;
  factorElements_ = 0;
  status_ = -1;
  numberSlacks_ = 0;
  numberU_ = 0;
  maximumU_=0;
  lengthU_ = 0;
  lengthAreaU_ = 0;
  numberL_ = 0;
  baseL_ = 0;
  lengthL_ = 0;
  lengthAreaL_ = 0;
  numberR_ = 0;
  lengthR_ = 0;
  lengthAreaR_ = 0;
  denseArea_=NULL;
  densePermute_=NULL;
  // next two offsets but this makes cleaner
  elementR_=NULL;
  indexRowR_=NULL;
  numberDense_=0;
  ////denseThreshold_=0;
  
}
// type - 1 bit tolerances etc, 2 rest
void CoinFactorization::gutsOfInitialize(int type)
{
  if ((type&1)!=0) {
    areaFactor_ = 0.0;
    pivotTolerance_ = 1.0e-1;
    zeroTolerance_ = 1.0e-13;
    slackValue_ = 1.0;
    messageLevel_=0;
    maximumPivots_=200;
    numberTrials_ = 4;
    relaxCheck_=1.0;
#if DENSE_CODE==1
    denseThreshold_=31;
    denseThreshold_=71;
#else
    denseThreshold_=0;
#endif
    biasLU_=2;
    doForrestTomlin_=true;
  }
  if ((type&2)!=0) {
    numberCompressions_ = 0;
    biggerDimension_ = 0;
    numberRows_ = 0;
    numberRowsExtra_ = 0;
    maximumRowsExtra_ = 0;
    numberColumns_ = 0;
    numberColumnsExtra_ = 0;
    maximumColumnsExtra_ = 0;
    numberGoodU_ = 0;
    numberGoodL_ = 0;
    totalElements_ = 0;
    factorElements_ = 0;
    status_ = -1;
    numberPivots_ = 0;
    pivotColumn_ = NULL;
    permute_ = NULL;
    permuteBack_ = NULL;
    pivotColumnBack_ = NULL;
    startRowU_ = NULL;
    numberInRow_ = NULL;
    numberInColumn_ = NULL;
    numberInColumnPlus_ = NULL;
    firstCount_ = NULL;
    nextCount_ = NULL;
    lastCount_ = NULL;
    nextColumn_ = NULL;
    lastColumn_ = NULL;
    nextRow_ = NULL;
    lastRow_ = NULL;
    saveColumn_ = NULL;
    markRow_ = NULL;
    indexColumnU_ = NULL;
    pivotRowL_ = NULL;
    pivotRegion_ = NULL;
    numberSlacks_ = 0;
    numberU_ = 0;
    maximumU_=0;
    lengthU_ = 0;
    lengthAreaU_ = 0;
    elementU_ = NULL;
    indexRowU_ = NULL;
    startColumnU_ = NULL;
    convertRowToColumnU_ = NULL;
    numberL_ = 0;
    baseL_ = 0;
    lengthL_ = 0;
    lengthAreaL_ = 0;
    elementL_ = NULL;
    indexRowL_ = NULL;
    startColumnL_ = NULL;
    numberR_ = 0;
    lengthR_ = 0;
    lengthAreaR_ = 0;
    elementR_ = NULL;
    indexRowR_ = NULL;
    startColumnR_ = NULL;
    elementByRowL_=NULL;
    startRowL_=NULL;
    indexColumnL_=NULL;
    // always switch off sparse
    sparseThreshold_=0;
    sparseThreshold2_= 0;
    sparse_=NULL;
    denseArea_ = NULL;
    densePermute_=NULL;
    numberDense_=0;
  }
  if ((type&4)!=0) {
    // we need to get 1 element arrays for any with length n+1 !!
    startColumnL_ = new CoinBigIndex [ 1 ];
    startColumnR_ = new CoinBigIndex [ 1 ];
    startRowU_ = new CoinBigIndex [ 1 ];
    numberInRow_ = new int [ 1 ];
    nextRow_ = new int [ 1 ];
    lastRow_ = new int [ 1 ];
    pivotRegion_ = new double [ 1 ];
    permuteBack_ = new int [ 1 ];
    permute_ = new int [ 1 ];
    pivotColumnBack_ = new int [ 1 ];
    startColumnU_ = new CoinBigIndex [ 1 ];
    numberInColumn_ = new int [ 1 ];
    numberInColumnPlus_ = new int [ 1 ];
    pivotColumn_ = new int [ 1 ];
    nextColumn_ = new int [ 1 ];
    lastColumn_ = new int [ 1 ];
    collectStatistics_=false;
    
    // Below are all to collect
    ftranCountInput_=0.0;
    ftranCountAfterL_=0.0;
    ftranCountAfterR_=0.0;
    ftranCountAfterU_=0.0;
    btranCountInput_=0.0;
    btranCountAfterU_=0.0;
    btranCountAfterR_=0.0;
    btranCountAfterL_=0.0;
    
    // We can roll over factorizations
    numberFtranCounts_=0;
    numberBtranCounts_=0;
    
    // While these are averages collected over last 
    ftranAverageAfterL_=0;
    ftranAverageAfterR_=0;
    ftranAverageAfterU_=0;
    btranAverageAfterU_=0;
    btranAverageAfterR_=0;
    btranAverageAfterL_=0; 
#ifdef ZEROFAULT
    startColumnL_[0] = 0;
    startColumnR_[0] = 0;
    startRowU_[0] = 0;
    numberInRow_[0] = 0;
    nextRow_[0] = 0;
    lastRow_[0] = 0;
    pivotRegion_[0] = 0.0;
    permuteBack_[0] = 0;
    permute_[0] = 0;
    pivotColumnBack_[0] = 0;
    startColumnU_[0] = 0;
    numberInColumn_[0] = 0;
    numberInColumnPlus_[0] = 0;
    pivotColumn_[0] = 0;
    nextColumn_[0] = 0;
    lastColumn_[0] = 0;
#endif
  }
}
//Part of LP
int CoinFactorization::factorize (
				 const CoinPackedMatrix & matrix,
				 int rowIsBasic[],
				 int columnIsBasic[],
				 double areaFactor )
{
  // maybe for speed will be better to leave as many regions as possible
  gutsOfDestructor();
  gutsOfInitialize(2);
  // ? is this correct
  //if (biasLU_==2)
  //biasLU_=3;
  if (areaFactor)
    areaFactor_ = areaFactor;
  const int * row = matrix.getIndices();
  const CoinBigIndex * columnStart = matrix.getVectorStarts();
  const int * columnLength = matrix.getVectorLengths(); 
  const double * element = matrix.getElements();
  int numberRows=matrix.getNumRows();
  int numberColumns=matrix.getNumCols();
  int numberBasic = 0;
  CoinBigIndex numberElements=0;
  int numberRowBasic=0;

  // compute how much in basis

  int i;

  for (i=0;i<numberRows;i++) {
    if (rowIsBasic[i]>=0)
      numberRowBasic++;
  }

  numberBasic = numberRowBasic;

  for (i=0;i<numberColumns;i++) {
    if (columnIsBasic[i]>=0) {
      numberBasic++;
      numberElements += columnLength[i];
    }
  }
  if ( numberBasic > numberRows ) {
    return -2; // say too many in basis
  }
  numberElements = 3 * numberBasic + 3 * numberElements + 10000;
  getAreas ( numberRows, numberBasic, numberElements,
	     2 * numberElements );
  //fill
  //copy
  numberBasic=0;
  numberElements=0;
  for (i=0;i<numberRows;i++) {
    if (rowIsBasic[i]>=0) {
      indexRowU_[numberElements]=i;
      indexColumnU_[numberElements]=numberBasic;
      elementU_[numberElements++]=slackValue_;
      numberBasic++;
    }
  }
  for (i=0;i<numberColumns;i++) {
    if (columnIsBasic[i]>=0) {
      CoinBigIndex j;
      for (j=columnStart[i];j<columnStart[i]+columnLength[i];j++) {
	indexRowU_[numberElements]=row[j];
	indexColumnU_[numberElements]=numberBasic;
	elementU_[numberElements++]=element[j];
      }
      numberBasic++;
    }
  }
  lengthU_ = numberElements;
  maximumU_ = numberElements;

  preProcess ( 0 );
  factor (  );
  numberBasic=0;
  if (status_ == 0) {
    int * permuteBack = permuteBack_;
    int * back = pivotColumnBack_;
    for (i=0;i<numberRows;i++) {
      if (rowIsBasic[i]>=0) {
	rowIsBasic[i]=permuteBack[back[numberBasic++]];
      }
    }
    for (i=0;i<numberColumns;i++) {
      if (columnIsBasic[i]>=0) {
	columnIsBasic[i]=permuteBack[back[numberBasic++]];
      }
    }
    // Set up permutation vector
    // these arrays start off as copies of permute
    // (and we could use permute_ instead of pivotColumn (not back though))
    CoinMemcpyN ( permute_, numberRows_ , pivotColumn_  );
    CoinMemcpyN ( permuteBack_, numberRows_ , pivotColumnBack_  );
  } else if (status_ == -1) {
    // mark as basic or non basic
    for (i=0;i<numberRows_;i++) {
      if (rowIsBasic[i]>=0) {
	if (pivotColumn_[numberBasic]>=0) 
	  rowIsBasic[i]=pivotColumn_[numberBasic];
	else 
	  rowIsBasic[i]=-1;
	numberBasic++;
      }
    }
    for (i=0;i<numberColumns;i++) {
      if (columnIsBasic[i]>=0) {
	if (pivotColumn_[numberBasic]>=0) 
	  columnIsBasic[i]=pivotColumn_[numberBasic];
	 else 
	  columnIsBasic[i]=-1;
	numberBasic++;
      }
    }
  }

  return status_;
}
//Given as triplets
int CoinFactorization::factorize (
			     int numberOfRows,
			     int numberOfColumns,
			     CoinBigIndex numberOfElements,
			     CoinBigIndex maximumL,
			     CoinBigIndex maximumU,
			     const int indicesRow[],
			     const int indicesColumn[],
			     const double elements[] ,
			     int permutation[],
			     double areaFactor)
{
  gutsOfDestructor();
  gutsOfInitialize(2);
  if (areaFactor)
    areaFactor_ = areaFactor;
  getAreas ( numberOfRows, numberOfColumns, maximumL, maximumU );
  //copy
  CoinMemcpyN ( indicesRow, numberOfElements, indexRowU_ );
  CoinMemcpyN ( indicesColumn, numberOfElements, indexColumnU_ );
  CoinMemcpyN ( elements, numberOfElements, elementU_ );
  lengthU_ = numberOfElements;
  maximumU_ = numberOfElements;
  preProcess ( 0 );
  factor (  );
  //say which column is pivoting on which row
  int i;
  if (status_ == 0) {
    int * permuteBack = permuteBack_;
    int * back = pivotColumnBack_;
    // permute so slacks on own rows etc
    for (i=0;i<numberOfColumns;i++) {
      permutation[i]=permuteBack[back[i]];
    }
    // Set up permutation vector
    // these arrays start off as copies of permute
    // (and we could use permute_ instead of pivotColumn (not back though))
    CoinMemcpyN ( permute_, numberRows_ , pivotColumn_  );
    CoinMemcpyN ( permuteBack_, numberRows_ , pivotColumnBack_  );
  } else if (status_ == -1) {
    // mark as basic or non basic
    for (i=0;i<numberOfColumns;i++) {
      if (pivotColumn_[i]>=0) {
	permutation[i]=pivotColumn_[i];
      } else {
	permutation[i]=-1;
      }
    }
  }

  return status_;
}
/* Two part version for flexibility
   This part creates arrays for user to fill.
   maximumL is guessed maximum size of L part of
   final factorization, maximumU of U part.  These are multiplied by
   areaFactor which can be computed by user or internally.  
   returns 0 -okay, -99 memory */
int 
CoinFactorization::factorizePart1 ( int numberOfRows,
				   int numberOfColumns,
				   CoinBigIndex numberOfElements,
				   int * indicesRow[],
				   int * indicesColumn[],
				   double * elements[],
				   double areaFactor)
{
  // maybe for speed will be better to leave as many regions as possible
  gutsOfDestructor();
  gutsOfInitialize(2);
  if (areaFactor)
    areaFactor_ = areaFactor;
  CoinBigIndex numberElements = 3 * numberOfRows + 3 * numberOfElements + 10000;
  getAreas ( numberOfRows, numberOfRows, numberElements,
	     2 * numberElements );
  // need to trap memory for -99 code
  *indicesRow = indexRowU_ ;
  *indicesColumn = indexColumnU_ ;
  *elements = elementU_ ;
  lengthU_ = numberOfElements;
  maximumU_ = numberElements;
  return 0;
}
/* This is part two of factorization
   Arrays belong to factorization and were returned by part 1
   If status okay, permutation has pivot rows.
   If status is singular, then basic variables have +1 and ones thrown out have -INT_MAX
   to say thrown out.
   returns 0 -okay, -1 singular, -99 memory */
int 
CoinFactorization::factorizePart2 (int permutation[],int exactNumberElements)
{
  lengthU_ = exactNumberElements;
  preProcess ( 0 );
  factor (  );
  //say which column is pivoting on which row
  int i;
  int * permuteBack = permuteBack_;
  int * back = pivotColumnBack_;
  // permute so slacks on own rows etc
  for (i=0;i<numberColumns_;i++) {
    permutation[i]=permuteBack[back[i]];
  }
  if (status_ == 0) {
    // Set up permutation vector
    // these arrays start off as copies of permute
    // (and we could use permute_ instead of pivotColumn (not back though))
    CoinMemcpyN ( permute_, numberRows_ , pivotColumn_  );
    CoinMemcpyN ( permuteBack_, numberRows_ , pivotColumnBack_  );
  } else if (status_ == -1) {
    // mark as basic or non basic
    for (i=0;i<numberColumns_;i++) {
      if (pivotColumn_[i]>=0) {
	permutation[i]=pivotColumn_[i];
      } else {
	permutation[i]=-1;
      }
    }
  }

  return status_;
}

//  ~CoinFactorization.  Destructor
CoinFactorization::~CoinFactorization (  )
{
  gutsOfDestructor();
}

//  show_self.  Debug show object
void
CoinFactorization::show_self (  ) const
{
  int i;

  for ( i = 0; i < numberRows_; i++ ) {
    std::cout << "r " << i << " " << pivotColumn_[i];
    if (pivotColumnBack_) std::cout<< " " << pivotColumnBack_[i];
    std::cout<< " " << permute_[i];
    if (permuteBack_) std::cout<< " " << permuteBack_[i];
    std::cout<< " " << pivotRegion_[i];
    std::cout << std::endl;
  }
  for ( i = 0; i < numberRows_; i++ ) {
    std::cout << "u " << i << " " << numberInColumn_[i] << std::endl;
    int j;
    CoinSort_2(indexRowU_+startColumnU_[i],
	       indexRowU_+startColumnU_[i]+numberInColumn_[i],
	       elementU_+startColumnU_[i]);
    for ( j = startColumnU_[i]; j < startColumnU_[i] + numberInColumn_[i];
	  j++ ) {
      assert (indexRowU_[j]>=0&&indexRowU_[j]<numberRows_);
      assert (elementU_[j]>-1.0e50&&elementU_[j]<1.0e50);
      std::cout << indexRowU_[j] << " " << elementU_[j] << std::endl;
    }
  }
  for ( i = 0; i < numberRows_; i++ ) {
    std::cout << "l " << i << " " << startColumnL_[i + 1] -
      startColumnL_[i] << std::endl;
    CoinSort_2(indexRowL_+startColumnL_[i],
	       indexRowL_+startColumnL_[i+1],
	       elementL_+startColumnL_[i]);
    int j;
    for ( j = startColumnL_[i]; j < startColumnL_[i + 1]; j++ ) {
      std::cout << indexRowL_[j] << " " << elementL_[j] << std::endl;
    }
  }

}
//  sort so can compare
void
CoinFactorization::sort (  ) const
{
  int i;

  for ( i = 0; i < numberRows_; i++ ) {
    CoinSort_2(indexRowU_+startColumnU_[i],
	       indexRowU_+startColumnU_[i]+numberInColumn_[i],
	       elementU_+startColumnU_[i]);
  }
  for ( i = 0; i < numberRows_; i++ ) {
    CoinSort_2(indexRowL_+startColumnL_[i],
	       indexRowL_+startColumnL_[i+1],
	       elementL_+startColumnL_[i]);
  }

}

//  getAreas.  Gets space for a factorization
//called by constructors
void
CoinFactorization::getAreas ( int numberOfRows,
			 int numberOfColumns,
			 CoinBigIndex maximumL,
			 CoinBigIndex maximumU )
{
  int extraSpace = maximumPivots_;

  numberRows_ = numberOfRows;
  numberColumns_ = numberOfColumns;
  maximumRowsExtra_ = numberRows_ + extraSpace;
  numberRowsExtra_ = numberRows_;
  maximumColumnsExtra_ = numberColumns_ + extraSpace;
  numberColumnsExtra_ = numberColumns_;
  lengthAreaU_ = maximumU;
  lengthAreaL_ = maximumL;
  if ( !areaFactor_ ) {
    areaFactor_ = 1.0;
  }
  if ( areaFactor_ != 1.0 ) {
    if ((messageLevel_&16)!=0) 
      printf("Increasing factorization areas by %g\n",areaFactor_);
    lengthAreaU_ =  (CoinBigIndex) (areaFactor_*lengthAreaU_);
    lengthAreaL_ =  (CoinBigIndex) (areaFactor_*lengthAreaL_);
  }
  elementU_ = new double [ lengthAreaU_ ];
  indexRowU_ = new int [ lengthAreaU_ ];
  indexColumnU_ = new int [ lengthAreaU_ ];
  elementL_ = new double [ lengthAreaL_ ];
  indexRowL_ = new int [ lengthAreaL_ ];
  startColumnL_ = new CoinBigIndex [ numberRows_ + 1 ];
  startColumnL_[0] = 0;
  startRowU_ = new CoinBigIndex [ maximumRowsExtra_ + 1 ];
  // make sure this is valid
  startRowU_[maximumRowsExtra_]=0;
  numberInRow_ = new int [ maximumRowsExtra_ + 1 ];
  markRow_ = new int [ numberRows_ ];
  pivotRowL_ = new int [ numberRows_ + 1 ];
  nextRow_ = new int [ maximumRowsExtra_ + 1 ];
  lastRow_ = new int [ maximumRowsExtra_ + 1 ];
  permute_ = new int [ maximumRowsExtra_ + 1 ];
  pivotRegion_ = new double [ maximumRowsExtra_ + 1 ];
#ifdef ZEROFAULT
  memset(elementU_,'a',lengthAreaU_*sizeof(double));
  memset(indexRowU_,'b',lengthAreaU_*sizeof(int));
  memset(indexColumnU_,'c',lengthAreaU_*sizeof(int));
  memset(elementL_,'d',lengthAreaL_*sizeof(double));
  memset(indexRowL_,'e',lengthAreaL_*sizeof(int));
  memset(startColumnL_+1,'f',numberRows_*sizeof(CoinBigIndex));
  memset(startRowU_,'g',maximumRowsExtra_*sizeof(CoinBigIndex));
  memset(numberInRow_,'h',(maximumRowsExtra_+1)*sizeof(int));
  memset(markRow_,'i',numberRows_*sizeof(int));
  memset(pivotRowL_,'j',(numberRows_+1)*sizeof(int));
  memset(nextRow_,'k',(maximumRowsExtra_+1)*sizeof(int));
  memset(lastRow_,'l',(maximumRowsExtra_+1)*sizeof(int));
  memset(permute_,'l',(maximumRowsExtra_+1)*sizeof(int));
  memset(pivotRegion_,'m',(maximumRowsExtra_+1)*sizeof(double));
#endif
  startColumnU_ = new CoinBigIndex [ maximumColumnsExtra_ + 1 ];
  numberInColumn_ = new int [ maximumColumnsExtra_ + 1 ];
  numberInColumnPlus_ = new int [ maximumColumnsExtra_ + 1 ];
  pivotColumn_ = new int [ maximumColumnsExtra_ + 1 ];
  nextColumn_ = new int [ maximumColumnsExtra_ + 1 ];
  lastColumn_ = new int [ maximumColumnsExtra_ + 1 ];
  saveColumn_ = new int [ numberColumns_ ];
#ifdef ZEROFAULT
  memset(startColumnU_,'a',(maximumColumnsExtra_+1)*sizeof(CoinBigIndex));
  memset(numberInColumn_,'b',(maximumColumnsExtra_+1)*sizeof(int));
  memset(numberInColumnPlus_,'c',(maximumColumnsExtra_+1)*sizeof(int));
  memset(pivotColumn_,'d',(maximumColumnsExtra_+1)*sizeof(int));
  memset(nextColumn_,'e',(maximumColumnsExtra_+1)*sizeof(int));
  memset(lastColumn_,'f',(maximumColumnsExtra_+1)*sizeof(int));
  memset(saveColumn_,'g',numberColumns_*sizeof(int));
#endif
  if ( numberRows_ + numberColumns_ ) {
    if ( numberRows_ > numberColumns_ ) {
      biggerDimension_ = numberRows_;
    } else {
      biggerDimension_ = numberColumns_;
    }
    firstCount_ = new int [ biggerDimension_ + 2 ];
    nextCount_ = new int [ numberRows_ + numberColumns_ ];
    lastCount_ = new int [ numberRows_ + numberColumns_ ];
#ifdef ZEROFAULT
    memset(firstCount_,'g',(biggerDimension_ + 2 )*sizeof(int));
    memset(nextCount_,'h',(numberRows_+numberColumns_)*sizeof(int));
    memset(lastCount_,'i',(numberRows_+numberColumns_)*sizeof(int));
#endif
  } else {
    firstCount_ = new int [ 2 ];
    nextCount_ = NULL;
    lastCount_ = NULL;
#ifdef ZEROFAULT
    memset(firstCount_,'g', 2 *sizeof(int));
#endif
    biggerDimension_ = 0;
  }
}

//  preProcess.  PreProcesses raw triplet data
//state is 0 - triplets, 1 - some counts etc , 2 - ..
void
CoinFactorization::preProcess ( int state,
			   int possibleDuplicates )
{
  int *indexRow = indexRowU_;
  int *indexColumn = indexColumnU_;
  double *element = elementU_;
  CoinBigIndex numberElements = lengthU_;
  int *numberInRow = numberInRow_;
  int *numberInColumn = numberInColumn_;
  CoinBigIndex *startRow = startRowU_;
  CoinBigIndex *startColumn = startColumnU_;
  int numberRows = numberRows_;
  int numberColumns = numberColumns_;

  if (state<4)
    totalElements_ = numberElements;
  //state falls through to next state
  switch ( state ) {
  case 0:			//counts
    {
      CoinZeroN ( numberInRow, numberRows + 1 );
      CoinZeroN ( numberInColumn, maximumColumnsExtra_ + 1 );
      CoinBigIndex i;
      for ( i = 0; i < numberElements; i++ ) {
	int iRow = indexRow[i];
	int iColumn = indexColumn[i];

	numberInRow[iRow]++;
	numberInColumn[iColumn]++;
      }
    }
  case -1:			//sort
  case 1:			//sort
    {
      CoinBigIndex i, k;

      i = 0;
      int iColumn;
      for ( iColumn = 0; iColumn < numberColumns; iColumn++ ) {
	//position after end of Column
	i += numberInColumn[iColumn];
	startColumn[iColumn] = i;
      }
      for ( k = numberElements - 1; k >= 0; k-- ) {
	int iColumn = indexColumn[k];

	if ( iColumn >= 0 ) {
	  double value = element[k];
	  int iRow = indexRow[k];

	  indexColumn[k] = -1;
	  while ( true ) {
	    CoinBigIndex iLook = startColumn[iColumn] - 1;

	    startColumn[iColumn] = iLook;
	    double valueSave = element[iLook];
	    int iColumnSave = indexColumn[iLook];
	    int iRowSave = indexRow[iLook];

	    element[iLook] = value;
	    indexRow[iLook] = iRow;
	    indexColumn[iLook] = -1;
	    if ( iColumnSave >= 0 ) {
	      iColumn = iColumnSave;
	      value = valueSave;
	      iRow = iRowSave;
	    } else {
	      break;
	    }
	  }			/* endwhile */
	}
      }
    }
  case 2:			//move largest in column to beginning
    //and do row part
    {
      CoinBigIndex i, k;

      i = 0;
      int iRow;
      for ( iRow = 0; iRow < numberRows; iRow++ ) {
	startRow[iRow] = i;
	i += numberInRow[iRow];
      }
      CoinZeroN ( numberInRow, numberRows );
      int iColumn;
      for ( iColumn = 0; iColumn < numberColumns; iColumn++ ) {
	int number = numberInColumn[iColumn];

	if ( number ) {
	  CoinBigIndex first = startColumn[iColumn];
	  CoinBigIndex largest = first;
	  int iRowSave = indexRow[first];
	  double valueSave = element[first];
	  double valueLargest = fabs ( valueSave );
	  int iLook = numberInRow[iRowSave];

	  numberInRow[iRowSave] = iLook + 1;
	  indexColumn[startRow[iRowSave] + iLook] = iColumn;
	  for ( k = first + 1; k < first + number; k++ ) {
	    int iRow = indexRow[k];
	    int iLook = numberInRow[iRow];

	    numberInRow[iRow] = iLook + 1;
	    indexColumn[startRow[iRow] + iLook] = iColumn;
	    double value = element[k];
	    double valueAbs = fabs ( value );

	    if ( valueAbs > valueLargest ) {
	      valueLargest = valueAbs;
	      largest = k;
	    }
	  }
	  indexRow[first] = indexRow[largest];
	  element[first] = element[largest];
	  indexRow[largest] = iRowSave;
	  element[largest] = valueSave;
	}
      }
    }
  case 3:			//links and initialize pivots
    {
      //set markRow so no rows updated
      CoinFillN ( markRow_, numberRows_, -1 );
      int *lastRow = lastRow_;
      int *nextRow = nextRow_;
      int *lastColumn = lastColumn_;
      int *nextColumn = nextColumn_;

      CoinFillN ( firstCount_, biggerDimension_ + 2, -1 );
      CoinFillN ( pivotColumn_, numberColumns_, -1 );
      CoinZeroN ( numberInColumnPlus_, maximumColumnsExtra_ + 1 );
      int iRow;
      for ( iRow = 0; iRow < numberRows; iRow++ ) {
	lastRow[iRow] = iRow - 1;
	nextRow[iRow] = iRow + 1;
	int number = numberInRow[iRow];

	addLink ( iRow, number );
      }
      lastRow[maximumRowsExtra_] = numberRows - 1;
      nextRow[maximumRowsExtra_] = 0;
      lastRow[0] = maximumRowsExtra_;
      nextRow[numberRows - 1] = maximumRowsExtra_;
      startRow[maximumRowsExtra_] = numberElements;
      int iColumn;
      for ( iColumn = 0; iColumn < numberColumns; iColumn++ ) {
	lastColumn[iColumn] = iColumn - 1;
	nextColumn[iColumn] = iColumn + 1;
	int number = numberInColumn[iColumn];

	addLink ( iColumn + numberRows, number );
      }
      lastColumn[maximumColumnsExtra_] = numberColumns - 1;
      nextColumn[maximumColumnsExtra_] = 0;
      lastColumn[0] = maximumColumnsExtra_;
      if (numberColumns)
	nextColumn[numberColumns - 1] = maximumColumnsExtra_;
      startColumn[maximumColumnsExtra_] = numberElements;
    }
    break;
  case 4:			//move largest in column to beginning
    {
      CoinBigIndex i, k;

      int iColumn;
      int iRow;
      for ( iRow = 0; iRow < numberRows; iRow++ ) {
	if( numberInRow[iRow]>=0) {
	  // zero count
	  numberInRow[iRow]=0;
	} else {
	  // empty
	  //numberInRow[iRow]=-1; already that
	}
      }
      //CoinZeroN ( numberInColumnPlus_, maximumColumnsExtra_ + 1 );
      for ( iColumn = 0; iColumn < numberColumns; iColumn++ ) {
	int number = numberInColumn[iColumn];

	if ( number ) {
	  // use pivotRegion and startRow for remaining elements
	  CoinBigIndex first = startColumn[iColumn];
	  CoinBigIndex largest = -1;
	  
	  double valueLargest = -1.0;
	  int nOther=0;
	  k = first;
	  CoinBigIndex end = first+number;
	  for (  ; k < end; k++ ) {
	    int iRow = indexRow[k];
	    double value = element[k];
	    if (numberInRow[iRow]>=0) {
	      numberInRow[iRow]++;
	      double valueAbs = fabs ( value );
	      if ( valueAbs > valueLargest ) {
		valueLargest = valueAbs;
		largest = nOther;
	      }
	      startRow[nOther]=iRow;
	      pivotRegion_[nOther++]=value;
	    } else {
	      indexRow[first] = iRow;
	      element[first++] = value;
	    }
	  }
	  numberInColumnPlus_[iColumn]=first-startColumn[iColumn];
	  startColumn[iColumn]=first;
	  //largest
	  if (largest>=0) {
	    indexRow[first] = startRow[largest];
	    element[first++] = pivotRegion_[largest];
	  }
	  for (k=0;k<nOther;k++) {
	    if (k!=largest) {
	      indexRow[first] = startRow[k];
	      element[first++] = pivotRegion_[k];
	    }
	  }
	  numberInColumn[iColumn]=first-startColumn[iColumn];
	}
      }
      //and do row part
      i = 0;
      for ( iRow = 0; iRow < numberRows; iRow++ ) {
	startRow[iRow] = i;
	int n=numberInRow[iRow];
	if (n>0) {
	  numberInRow[iRow]=0;
	  i += n;
	}
      }
      for ( iColumn = 0; iColumn < numberColumns; iColumn++ ) {
	int number = numberInColumn[iColumn];

	if ( number ) {
	  CoinBigIndex first = startColumn[iColumn];
	  for ( k = first; k < first + number; k++ ) {
	    int iRow = indexRow[k];
	    int iLook = numberInRow[iRow];

	    numberInRow[iRow] = iLook + 1;
	    indexColumn[startRow[iRow] + iLook] = iColumn;
	  }
	}
      }
    }
    // modified 3
    {
      //set markRow so no rows updated
      CoinFillN ( markRow_, numberRows_, -1 );
      int *lastColumn = lastColumn_;
      int *nextColumn = nextColumn_;

      int iRow;
      int numberGood=0;
      startColumnL_[0] = 0;	//for luck
      for ( iRow = 0; iRow < numberRows; iRow++ ) {
	int number = numberInRow[iRow];
	if (number<0) {
	  numberInRow[iRow]=0;
	  pivotRegion_[numberGood++]=slackValue_;
	}
      }
      int iColumn;
      for ( iColumn = 0 ; iColumn < numberColumns_; iColumn++ ) {
	lastColumn[iColumn] = iColumn - 1;
	nextColumn[iColumn] = iColumn + 1;
	int number = numberInColumn[iColumn];
	deleteLink(iColumn+numberRows);
	addLink ( iColumn + numberRows, number );
      }
      lastColumn[maximumColumnsExtra_] = numberColumns - 1;
      nextColumn[maximumColumnsExtra_] = 0;
      lastColumn[0] = maximumColumnsExtra_;
      if (numberColumns)
	nextColumn[numberColumns - 1] = maximumColumnsExtra_;
      startColumn[maximumColumnsExtra_] = numberElements;
    }
  }				/* endswitch */
}

//Does most of factorization
int
CoinFactorization::factor (  )
{
  //sparse
  status_ = factorSparse (  );
  switch ( status_ ) {
  case 0:			//finished
    totalElements_ = 0;
    {
      if ( numberGoodU_ < numberRows_ ) {
	int i, k;

	int * swap = permute_;
	permute_ = nextRow_;
	nextRow_ = swap;
	for ( i = 0; i < numberRows_; i++ ) {
	  lastRow_[i] = -1;
	}
	for ( i = 0; i < numberColumns_; i++ ) {
	  lastColumn_[i] = -1;
	}
	for ( i = 0; i < numberGoodU_; i++ ) {
	  int goodRow = pivotRowL_[i];	//valid pivot row
	  int goodColumn = pivotColumn_[i];

	  lastRow_[goodRow] = goodColumn;	//will now have -1 or column sequence
	  lastColumn_[goodColumn] = goodRow;	//will now have -1 or row sequence
	}
	delete [] nextRow_;
	nextRow_ = NULL;
	k = 0;
	//copy back and count
	for ( i = 0; i < numberRows_; i++ ) {
	  permute_[i] = lastRow_[i];
	  if ( permute_[i] < 0 ) {
	    //std::cout << i << " " <<permute_[i] << std::endl;
	  } else {
	    k++;
	  }
	}
	for ( i = 0; i < numberColumns_; i++ ) {
	  pivotColumn_[i] = lastColumn_[i];
	}
	if ((messageLevel_&4)!=0) 
	  std::cout <<"Factorization has "<<numberRows_-k
		    <<" singularities"<<std::endl;
	status_ = -1;
      }
    }
    break;
    // dense
  case 2:
    status_=factorDense();
    if(!status_) 
      break;
  default:
    //singular ? or some error
    if ((messageLevel_&4)!=0) 
      std::cout << "Error " << status_ << std::endl;
    break;
  }				/* endswitch */
  //clean up
  if ( !status_ ) {
    if ( (messageLevel_ & 16)&&numberCompressions_)
      std::cout<<"        Factorization did "<<numberCompressions_
	       <<" compressions"<<std::endl;
    if ( numberCompressions_ > 10 ) {
      areaFactor_ *= 1.1;
    }
    numberCompressions_=0;
    cleanup (  );
  }
  return status_;
}


//  pivotRowSingleton.  Does one pivot on Row Singleton in factorization
bool
CoinFactorization::pivotRowSingleton ( int pivotRow,
				  int pivotColumn )
{
  //store pivot columns (so can easily compress)
  CoinBigIndex startColumn = startColumnU_[pivotColumn];
  int numberDoColumn = numberInColumn_[pivotColumn] - 1;
  CoinBigIndex endColumn = startColumn + numberDoColumn + 1;
  CoinBigIndex pivotRowPosition = startColumn;
  int iRow = indexRowU_[pivotRowPosition];

  while ( iRow != pivotRow ) {
    pivotRowPosition++;
    iRow = indexRowU_[pivotRowPosition];
  }				/* endwhile */
  assert ( pivotRowPosition < endColumn );
  //store column in L, compress in U and take column out
  CoinBigIndex l = lengthL_;

  if ( l + numberDoColumn > lengthAreaL_ ) {
    //need more memory
    if ((messageLevel_&4)!=0) 
      std::cout << "more memory needed in middle of invert" << std::endl;
    return false;
  }
  pivotRowL_[numberGoodL_] = pivotRow;
  startColumnL_[numberGoodL_] = l;	//for luck and first time
  numberGoodL_++;
  startColumnL_[numberGoodL_] = l + numberDoColumn;
  lengthL_ += numberDoColumn;
  double pivotElement = elementU_[pivotRowPosition];
  double pivotMultiplier = 1.0 / pivotElement;

  pivotRegion_[numberGoodU_] = pivotMultiplier;
  CoinBigIndex i;

  for ( i = startColumn; i < pivotRowPosition; i++ ) {
    int iRow = indexRowU_[i];

    indexRowL_[l] = iRow;
    elementL_[l] = elementU_[i] * pivotMultiplier;
    l++;
    //take out of row list
    CoinBigIndex start = startRowU_[iRow];
    int numberInRow = numberInRow_[iRow];
    CoinBigIndex end = start + numberInRow;
    CoinBigIndex where = start;

    while ( indexColumnU_[where] != pivotColumn ) {
      where++;
    }				/* endwhile */
    assert ( where < end );
    indexColumnU_[where] = indexColumnU_[end - 1];
    numberInRow--;
    numberInRow_[iRow] = numberInRow;
    deleteLink ( iRow );
    addLink ( iRow, numberInRow );
  }
  for ( i = pivotRowPosition + 1; i < endColumn; i++ ) {
    int iRow = indexRowU_[i];

    indexRowL_[l] = iRow;
    elementL_[l] = elementU_[i] * pivotMultiplier;
    l++;
    //take out of row list
    CoinBigIndex start = startRowU_[iRow];
    int numberInRow = numberInRow_[iRow];
    CoinBigIndex end = start + numberInRow;
    CoinBigIndex where = start;

    while ( indexColumnU_[where] != pivotColumn ) {
      where++;
    }				/* endwhile */
    assert ( where < end );
    indexColumnU_[where] = indexColumnU_[end - 1];
    numberInRow--;
    numberInRow_[iRow] = numberInRow;
    deleteLink ( iRow );
    addLink ( iRow, numberInRow );
  }
  numberInColumn_[pivotColumn] = 0;
  //modify linked list for pivots
  numberInRow_[pivotRow] = 0;
  deleteLink ( pivotRow );
  deleteLink ( pivotColumn + numberRows_ );
  //take out this bit of indexColumnU
  int next = nextRow_[pivotRow];
  int last = lastRow_[pivotRow];

  nextRow_[last] = next;
  lastRow_[next] = last;
  lastRow_[pivotRow] =-2;
  nextRow_[pivotRow] = numberGoodU_;	//use for permute
  return true;
}

//  pivotColumnSingleton.  Does one pivot on Column Singleton in factorization
bool
CoinFactorization::pivotColumnSingleton ( int pivotRow,
				     int pivotColumn )
{
  //store pivot columns (so can easily compress)
  int numberDoRow = numberInRow_[pivotRow] - 1;
  CoinBigIndex startColumn = startColumnU_[pivotColumn];
  int put = 0;
  CoinBigIndex startRow = startRowU_[pivotRow];
  CoinBigIndex endRow = startRow + numberDoRow + 1;
  CoinBigIndex i;

  for ( i = startRow; i < endRow; i++ ) {
    int iColumn = indexColumnU_[i];

    if ( iColumn != pivotColumn ) {
      saveColumn_[put++] = iColumn;
    }
  }
  //take out this bit of indexColumnU
  int next = nextRow_[pivotRow];
  int last = lastRow_[pivotRow];

  nextRow_[last] = next;
  lastRow_[next] = last;
  nextRow_[pivotRow] = numberGoodU_;	//use for permute
  //clean up counts
  double pivotElement = elementU_[startColumn];

  pivotRegion_[numberGoodU_] = 1.0 / pivotElement;
  numberInColumn_[pivotColumn] = 0;
  //totalElements_ --;
  //numberInColumnPlus_[pivotColumn]++;
  //move pivot row in other columns to safe zone
  for ( i = 0; i < numberDoRow; i++ ) {
    int iColumn = saveColumn_[i];

    if ( numberInColumn_[iColumn] ) {
      int number = numberInColumn_[iColumn] - 1;

      //modify linked list
      deleteLink ( iColumn + numberRows_ );
      addLink ( iColumn + numberRows_, number );
      //move pivot row element
      if ( number ) {
	CoinBigIndex start = startColumnU_[iColumn];
	CoinBigIndex pivot = start;
	int iRow = indexRowU_[pivot];
	while ( iRow != pivotRow ) {
	  pivot++;
	  iRow = indexRowU_[pivot];
	}
        assert (pivot < startColumnU_[iColumn] +
                numberInColumn_[iColumn]);
	if ( pivot != start ) {
	  //move largest one up
	  double value = elementU_[start];

	  iRow = indexRowU_[start];
	  elementU_[start] = elementU_[pivot];
	  indexRowU_[start] = indexRowU_[pivot];
	  elementU_[pivot] = elementU_[start + 1];
	  indexRowU_[pivot] = indexRowU_[start + 1];
	  elementU_[start + 1] = value;
	  indexRowU_[start + 1] = iRow;
	} else {
	  //find new largest element
	  int iRowSave = indexRowU_[start + 1];
	  double valueSave = elementU_[start + 1];
	  double valueLargest = fabs ( valueSave );
	  CoinBigIndex end = start + numberInColumn_[iColumn];
	  CoinBigIndex largest = start + 1;

	  CoinBigIndex k;
	  for ( k = start + 2; k < end; k++ ) {
	    double value = elementU_[k];
	    double valueAbs = fabs ( value );

	    if ( valueAbs > valueLargest ) {
	      valueLargest = valueAbs;
	      largest = k;
	    }
	  }
	  indexRowU_[start + 1] = indexRowU_[largest];
	  elementU_[start + 1] = elementU_[largest];
	  indexRowU_[largest] = iRowSave;
	  elementU_[largest] = valueSave;
	}
      }
      //clean up counts
      numberInColumn_[iColumn]--;
      numberInColumnPlus_[iColumn]++;
      startColumnU_[iColumn]++;
      //totalElements_--;
    }
  }
  //modify linked list for pivots
  deleteLink ( pivotRow );
  deleteLink ( pivotColumn + numberRows_ );
  numberInRow_[pivotRow] = 0;
  //put in dummy pivot in L
  CoinBigIndex l = lengthL_;

  pivotRowL_[numberGoodL_] = pivotRow;
  startColumnL_[numberGoodL_] = l;	//for luck and first time
  numberGoodL_++;
  startColumnL_[numberGoodL_] = l;
  return true;
}


//  getColumnSpace.  Gets space for one Column with given length
//may have to do compression  (returns true)
//also moves existing vector
bool
CoinFactorization::getColumnSpace ( int iColumn,
			       int extraNeeded )
{
  int number = numberInColumnPlus_[iColumn] +

    numberInColumn_[iColumn];
  CoinBigIndex space = lengthAreaU_ - startColumnU_[maximumColumnsExtra_];

  if ( space < extraNeeded + number + 2 ) {
    //compression
    int iColumn = nextColumn_[maximumColumnsExtra_];
    CoinBigIndex put = 0;

    while ( iColumn != maximumColumnsExtra_ ) {
      //move
      CoinBigIndex get;
      CoinBigIndex getEnd;

      if ( startColumnU_[iColumn] >= 0 ) {
	get = startColumnU_[iColumn]
	  - numberInColumnPlus_[iColumn];
	getEnd = startColumnU_[iColumn] + numberInColumn_[iColumn];
	startColumnU_[iColumn] = put + numberInColumnPlus_[iColumn];
      } else {
	get = -startColumnU_[iColumn];
	getEnd = get + numberInColumn_[iColumn];
	startColumnU_[iColumn] = -put;
      }
      CoinBigIndex i;
      for ( i = get; i < getEnd; i++ ) {
	indexRowU_[put] = indexRowU_[i];
	elementU_[put] = elementU_[i];
	put++;
      }
      iColumn = nextColumn_[iColumn];
    }				/* endwhile */
    numberCompressions_++;
    startColumnU_[maximumColumnsExtra_] = put;
    space = lengthAreaU_ - put;
    if ( extraNeeded == INT_MAX >> 1 ) {
      return true;
    }
    if ( space < extraNeeded + number + 2 ) {
      //need more space
      //if we can allocate bigger then do so and copy
      //if not then return so code can start again
      status_ = -99;
      return false;
    }
  }
  CoinBigIndex put = startColumnU_[maximumColumnsExtra_];
  int next = nextColumn_[iColumn];
  int last = lastColumn_[iColumn];

  if ( extraNeeded || next != maximumColumnsExtra_ ) {
    //out
    nextColumn_[last] = next;
    lastColumn_[next] = last;
    //in at end
    last = lastColumn_[maximumColumnsExtra_];
    nextColumn_[last] = iColumn;
    lastColumn_[maximumColumnsExtra_] = iColumn;
    lastColumn_[iColumn] = last;
    nextColumn_[iColumn] = maximumColumnsExtra_;
    //move
    CoinBigIndex get = startColumnU_[iColumn]
      - numberInColumnPlus_[iColumn];

    startColumnU_[iColumn] = put + numberInColumnPlus_[iColumn];
    if ( number < 50 ) {
      int *indexRow = indexRowU_;
      double *element = elementU_;
      int i = 0;

      if ( ( number & 1 ) != 0 ) {
	element[put] = element[get];
	indexRow[put] = indexRow[get];
	i = 1;
      }
      for ( ; i < number; i += 2 ) {
	double value0 = element[get + i];
	double value1 = element[get + i + 1];
	int index0 = indexRow[get + i];
	int index1 = indexRow[get + i + 1];

	element[put + i] = value0;
	element[put + i + 1] = value1;
	indexRow[put + i] = index0;
	indexRow[put + i + 1] = index1;
      }
    } else {
      CoinMemcpyN ( &indexRowU_[get], number, &indexRowU_[put] );
      CoinMemcpyN ( &elementU_[get], number, &elementU_[put] );
    }
    put += number;
    get += number;
    //add 4 for luck
    startColumnU_[maximumColumnsExtra_] = put + extraNeeded + 4;
  } else {
    //take off space
    startColumnU_[maximumColumnsExtra_] = startColumnU_[last] +
      numberInColumn_[last];
  }
  return true;
}

//  getRowSpace.  Gets space for one Row with given length
//may have to do compression  (returns true)
//also moves existing vector
bool
CoinFactorization::getRowSpace ( int iRow,
			    int extraNeeded )
{
  int number = numberInRow_[iRow];
  CoinBigIndex space = lengthAreaU_ - startRowU_[maximumRowsExtra_];

  if ( space < extraNeeded + number + 2 ) {
    //compression
    int iRow = nextRow_[maximumRowsExtra_];
    CoinBigIndex put = 0;

    while ( iRow != maximumRowsExtra_ ) {
      //move
      CoinBigIndex get = startRowU_[iRow];
      CoinBigIndex getEnd = startRowU_[iRow] + numberInRow_[iRow];

      startRowU_[iRow] = put;
      CoinBigIndex i;
      for ( i = get; i < getEnd; i++ ) {
	indexColumnU_[put] = indexColumnU_[i];
	put++;
      }
      iRow = nextRow_[iRow];
    }				/* endwhile */
    numberCompressions_++;
    startRowU_[maximumRowsExtra_] = put;
    space = lengthAreaU_ - put;
    if ( space < extraNeeded + number + 2 ) {
      //need more space
      //if we can allocate bigger then do so and copy
      //if not then return so code can start again
      status_ = -99;
      return false;;
    }
  }
  CoinBigIndex put = startRowU_[maximumRowsExtra_];
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
  CoinBigIndex get = startRowU_[iRow];

  startRowU_[iRow] = put;
  while ( number ) {
    number--;
    indexColumnU_[put] = indexColumnU_[get];
    put++;
    get++;
  }				/* endwhile */
  //add 4 for luck
  startRowU_[maximumRowsExtra_] = put + extraNeeded + 4;
  return true;
}

//  cleanup.  End of factorization
void
CoinFactorization::cleanup (  )
{
  getColumnSpace ( 0, INT_MAX >> 1 );	//compress
  CoinBigIndex lastU = startColumnU_[maximumColumnsExtra_];

  //free some memory here
  delete []  saveColumn_ ;
  delete []  markRow_ ;
  delete []  firstCount_ ;
  delete []  nextCount_ ;
  delete []  lastCount_ ;
  saveColumn_ = 0;
  markRow_ = 0;
  firstCount_ = 0;
  nextCount_ = 0;
  lastCount_ = 0;
  //make column starts OK
  //for best cache behavior get in order (last pivot at bottom of space)
  //that will need thinking about
  //use nextRow for permutation  (as that is what it is)
  int i;

  int * swap = permute_;
  permute_ = nextRow_;
  nextRow_ = swap;
  //safety feature
  permute_[numberRows_] = 0;
  permuteBack_ = new int [ maximumRowsExtra_ + 1 ];
#ifdef ZEROFAULT
  memset(permuteBack_,'w',(maximumRowsExtra_+1)*sizeof(int));
#endif
  for ( i = 0; i < numberRows_; i++ ) {
    int iRow = permute_[i];

    permuteBack_[iRow] = i;
  }
  //redo nextRow_
  int extraSpace = maximumPivots_;

  // Redo total elements
  totalElements_=0;
  for ( i = 0; i < numberColumns_; i++ ) {
    int number = numberInColumn_[i];	//always 0?
    int processed = numberInColumnPlus_[i];
    CoinBigIndex start = startColumnU_[i] - processed;

    number += processed;
    numberInColumn_[i] = number;
    totalElements_ += number;
    startColumnU_[i] = start;
    //full list
    numberInColumnPlus_[i] = 0;
  }
  if ( (messageLevel_ & 8)) {
    std::cout<<"        length of U "<<totalElements_<<", length of L "<<lengthL_;
    if (numberDense_)
      std::cout<<" plus "<<numberDense_*numberDense_<<" from "<<numberDense_<<" dense rows";
    std::cout<<std::endl;
  }
  // and add L and dense
  totalElements_ += numberDense_*numberDense_+lengthL_;
  int numberU = 0;

  pivotColumnBack_ = new int [ maximumRowsExtra_ + 1 ];
#ifdef ZEROFAULT
  memset(pivotColumnBack_,'q',(maximumRowsExtra_+1)*sizeof(int));
#endif
  for ( i = 0; i < numberColumns_; i++ ) {
    int iColumn = pivotColumn_[i];

    pivotColumnBack_[iColumn] = i;
    if ( iColumn >= 0 ) {
      if ( !numberInColumnPlus_[iColumn] ) {
	//wanted
	if ( numberU != iColumn ) {
	  numberInColumnPlus_[iColumn] = numberU;
	} else {
	  numberInColumnPlus_[iColumn] = -1;	//already in correct place
	}
	numberU++;
      }
    }
  }
  for ( i = 0; i < numberColumns_; i++ ) {
    int number = numberInColumn_[i];	//always 0?
    int where = numberInColumnPlus_[i];

    numberInColumnPlus_[i] = -1;
    CoinBigIndex start = startColumnU_[i];

    while ( where >= 0 ) {
      //put where it should be
      int numberNext = numberInColumn_[where];	//always 0?
      int whereNext = numberInColumnPlus_[where];
      CoinBigIndex startNext = startColumnU_[where];

      numberInColumn_[where] = number;
      numberInColumnPlus_[where] = -1;
      startColumnU_[where] = start;
      number = numberNext;
      where = whereNext;
      start = startNext;
    }				/* endwhile */
  }
  //sort - using indexColumn
  CoinFillN ( indexColumnU_, lastU, -1 );
  CoinBigIndex k = 0;
  int *numberInColumn = numberInColumn_;
  int *indexColumnU = indexColumnU_;
  CoinBigIndex *startColumn = startColumnU_;
  int *indexRowU = indexRowU_;
  double *elementU = elementU_;

  for ( i = numberSlacks_; i < numberRows_; i++ ) {
    CoinBigIndex start = startColumn[i];
    CoinBigIndex end = start + numberInColumn[i];

    CoinBigIndex j;
    for ( j = start; j < end; j++ ) {
      indexColumnU[j] = k++;
    }
  }
  for ( i = numberSlacks_; i < numberRows_; i++ ) {
    CoinBigIndex start = startColumn[i];
    CoinBigIndex end = start + numberInColumn[i];

    CoinBigIndex j;
    for ( j = start; j < end; j++ ) {
      CoinBigIndex k = indexColumnU[j];
      int iRow = indexRowU[j];
      double element = elementU[j];

      while ( k != -1 ) {
	CoinBigIndex kNext = indexColumnU[k];
	int iRowNext = indexRowU[k];
	double elementNext = elementU[k];

	indexColumnU_[k] = -1;
	indexRowU[k] = iRow;
	elementU[k] = element;
	k = kNext;
	iRow = iRowNext;
	element = elementNext;
      }				/* endwhile */
    }
  }
  CoinZeroN ( startColumnU_, numberSlacks_ );
  k = 0;
  for ( i = numberSlacks_; i < numberRows_; i++ ) {
    startColumnU_[i] = k;
    k += numberInColumn_[i];
  }
  maximumU_=k;
  // See whether to have extra copy of R
  if (k>10*numberRows_) {
    // NO
    delete []  numberInColumnPlus_ ;
    numberInColumnPlus_ = NULL;
  } else {
    for ( i = 0; i < numberColumns_; i++ ) {
      lastColumn_[i] = i - 1;
      nextColumn_[i] = i + 1;
      numberInColumnPlus_[i]=0;
    }
    nextColumn_[numberColumns_ - 1] = maximumColumnsExtra_;
    lastColumn_[maximumColumnsExtra_] = numberColumns_ - 1;
    nextColumn_[maximumColumnsExtra_] = 0;
    lastColumn_[0] = maximumColumnsExtra_;
  }
  numberU_ = numberU;
  numberGoodU_ = numberU;
  numberL_ = numberGoodL_;
#if COIN_DEBUG
  for ( i = 0; i < numberRows_; i++ ) {
    if ( permute_[i] < 0 ) {
      std::cout << i << std::endl;
      abort (  );
    }
  }
#endif
  for ( i = numberSlacks_; i < numberU; i++ ) {
    CoinBigIndex start = startColumnU_[i];
    CoinBigIndex end = start + numberInColumn_[i];

    totalElements_ += numberInColumn_[i];
    CoinBigIndex j;
    for ( j = start; j < end; j++ ) {
      int iRow = indexRowU_[j];
      iRow = permute_[iRow];
      indexRowU_[j] = iRow;
      numberInRow_[iRow]++;
    }
  }
  //space for cross reference
  convertRowToColumnU_ = new CoinBigIndex [ lengthAreaU_ ];
  CoinBigIndex *convertRowToColumn = convertRowToColumnU_;
  CoinBigIndex j = 0;
  CoinBigIndex *startRow = startRowU_;

  int iRow;
  for ( iRow = 0; iRow < numberRows_; iRow++ ) {
    startRow[iRow] = j;
    j += numberInRow_[iRow];
  }
  CoinBigIndex numberInU = j;

  CoinZeroN ( numberInRow_, numberRows_ );
  CoinBigIndex lowCount = 0;
  CoinBigIndex highCount = numberInU;
  int lowC = 0;
  int highC = numberRows_ - numberSlacks_;

  for ( i = numberSlacks_; i < numberRows_; i++ ) {
    CoinBigIndex start = startColumnU_[i];
    CoinBigIndex end = start + numberInColumn_[i];

    lowCount += numberInColumn_[i];
    highCount -= numberInColumn_[i];
    lowC++;
    highC--;
    double pivotValue = pivotRegion_[i];

    CoinBigIndex j;
    for ( j = start; j < end; j++ ) {
      int iRow = indexRowU_[j];
      int iLook = numberInRow_[iRow];

      numberInRow_[iRow] = iLook + 1;
      CoinBigIndex k = startRow[iRow] + iLook;

      indexColumnU_[k] = i;
      convertRowToColumn[k] = j;
      //multiply by pivot
      elementU_[j] *= pivotValue;
    }
  }
  for ( j = 0; j < numberRows_; j++ ) {
    lastRow_[j] = j - 1;
    nextRow_[j] = j + 1;
  }
  nextRow_[numberRows_ - 1] = maximumRowsExtra_;
  lastRow_[maximumRowsExtra_] = numberRows_ - 1;
  nextRow_[maximumRowsExtra_] = 0;
  lastRow_[0] = maximumRowsExtra_;
  startRow[maximumRowsExtra_] = numberInU;
  CoinBigIndex *startColumnL = startColumnL_;

  int firstReal = numberRows_;

  for ( i = numberRows_ - 1; i >= 0; i-- ) {
    CoinBigIndex start = startColumnL[i];
    CoinBigIndex end = startColumnL[i + 1];

    totalElements_ += end - start;
    int pivotRow = pivotRowL_[i];

    pivotRow = permute_[pivotRow];
    pivotRowL_[i] = pivotRow;
    if ( end > start ) {
      firstReal = i;
      CoinBigIndex j;
      for ( j = start; j < end; j++ ) {
	int iRow = indexRowL_[j];

	iRow = permute_[iRow];
	indexRowL_[j] = iRow;
      }
    }
  }
  baseL_ = firstReal;
  numberL_ = numberGoodL_ - firstReal;
  factorElements_ = totalElements_;
  pivotRowL_[numberGoodL_] = numberRows_;	//so loop will be clean
  //can deletepivotRowL_ as not used
  delete []  pivotRowL_ ;
  pivotRowL_ = NULL;
  //use L for R if room
  CoinBigIndex space = lengthAreaL_ - lengthL_;
  CoinBigIndex spaceUsed = lengthL_ + lengthU_;

  extraSpace = maximumPivots_;
  int needed = ( spaceUsed + numberRows_ - 1 ) / numberRows_;

  needed = needed * 2 * maximumPivots_;
  if ( needed < 2 * numberRows_ ) {
    needed = 2 * numberRows_;
  }
  if (numberInColumnPlus_) {
    // Need double the space for R
    space = space/2;
    startColumnR_ = new CoinBigIndex 
      [ extraSpace + 1 + maximumColumnsExtra_ + 1 ];
    CoinBigIndex * startR = startColumnR_ + extraSpace+1;
    CoinZeroN (startR,(maximumColumnsExtra_+1));
  } else {
    startColumnR_ = new CoinBigIndex [ extraSpace + 1 ];
  }
#ifdef ZEROFAULT
  memset(startColumnR_,'z',(extraSpace + 1)*sizeof(CoinBigIndex));
#endif
  if ( space >= needed ) {
    lengthR_ = 0;
    lengthAreaR_ = space;
    elementR_ = elementL_ + lengthL_;
    indexRowR_ = indexRowL_ + lengthL_;
  } else {
    lengthR_ = 0;
    lengthAreaR_ = space;
    elementR_ = elementL_ + lengthL_;
    indexRowR_ = indexRowL_ + lengthL_;
    if ((messageLevel_&4))
      std::cout<<"Factorization may need some increasing area space"
	       <<std::endl;
    if ( areaFactor_ ) {
      areaFactor_ *= 1.1;
    } else {
      areaFactor_ = 1.1;
    }
  }
  numberR_ = 0;
}
// Returns areaFactor but adjusted for dense
double 
CoinFactorization::adjustedAreaFactor() const
{
  double factor = areaFactor_;
  if (numberDense_&&areaFactor_>1.0) {
    double dense = numberDense_;
    dense *= dense;
    double withoutDense = totalElements_ - dense +1.0;
    factor *= 1.0 +dense/withoutDense;
  }
  return factor;
}

//  checkConsistency.  Checks that row and column copies look OK
void
CoinFactorization::checkConsistency (  )
{
  bool bad = false;

  int iRow;
  for ( iRow = 0; iRow < numberRows_; iRow++ ) {
    if ( numberInRow_[iRow] ) {
      CoinBigIndex startRow = startRowU_[iRow];
      CoinBigIndex endRow = startRow + numberInRow_[iRow];

      CoinBigIndex j;
      for ( j = startRow; j < endRow; j++ ) {
	int iColumn = indexColumnU_[j];
	CoinBigIndex startColumn = startColumnU_[iColumn];
	CoinBigIndex endColumn = startColumn + numberInColumn_[iColumn];
	bool found = false;

	CoinBigIndex k;
	for ( k = startColumn; k < endColumn; k++ ) {
	  if ( indexRowU_[k] == iRow ) {
	    found = true;
	    break;
	  }
	}
	if ( !found ) {
	  bad = true;
	  std::cout << "row " << iRow << " column " << iColumn << " Rows" << std::endl;
	}
      }
    }
  }
  int iColumn;
  for ( iColumn = 0; iColumn < numberColumns_; iColumn++ ) {
    if ( numberInColumn_[iColumn] ) {
      CoinBigIndex startColumn = startColumnU_[iColumn];
      CoinBigIndex endColumn = startColumn + numberInColumn_[iColumn];

      CoinBigIndex j;
      for ( j = startColumn; j < endColumn; j++ ) {
	int iRow = indexRowU_[j];
	CoinBigIndex startRow = startRowU_[iRow];
	CoinBigIndex endRow = startRow + numberInRow_[iRow];
	bool found = false;

	CoinBigIndex k;
	for (  k = startRow; k < endRow; k++ ) {
	  if ( indexColumnU_[k] == iColumn ) {
	    found = true;
	    break;
	  }
	}
	if ( !found ) {
	  bad = true;
	  std::cout << "row " << iRow << " column " << iColumn << " Columns" <<
	    std::endl;
	}
      }
    }
  }
  if ( bad ) {
    abort (  );
  }
}
  //  pivotOneOtherRow.  When just one other row so faster
bool 
CoinFactorization::pivotOneOtherRow ( int pivotRow,
					   int pivotColumn )
{
  int numberInPivotRow = numberInRow_[pivotRow] - 1;
  CoinBigIndex startColumn = startColumnU_[pivotColumn];
  CoinBigIndex startRow = startRowU_[pivotRow];
  CoinBigIndex endRow = startRow + numberInPivotRow + 1;

  //take out this bit of indexColumnU
  int next = nextRow_[pivotRow];
  int last = lastRow_[pivotRow];

  nextRow_[last] = next;
  lastRow_[next] = last;
  nextRow_[pivotRow] = numberGoodU_;	//use for permute
  lastRow_[pivotRow] = -2;
  numberInRow_[pivotRow] = 0;
  //store column in L, compress in U and take column out
  CoinBigIndex l = lengthL_;

  if ( l + 1 > lengthAreaL_ ) {
    //need more memory
    if ((messageLevel_&4)!=0) 
      std::cout << "more memory needed in middle of invert" << std::endl;
    return false;
  }
  //l+=currentAreaL_->elementByColumn-elementL_;
  //CoinBigIndex lSave=l;
  pivotRowL_[numberGoodL_] = pivotRow;
  startColumnL_[numberGoodL_] = l;	//for luck and first time
  numberGoodL_++;
  startColumnL_[numberGoodL_] = l + 1;
  lengthL_++;
  double pivotElement;
  double otherMultiplier;
  int otherRow;

  if ( indexRowU_[startColumn] == pivotRow ) {
    pivotElement = elementU_[startColumn];
    otherMultiplier = elementU_[startColumn + 1];
    otherRow = indexRowU_[startColumn + 1];
  } else {
    pivotElement = elementU_[startColumn + 1];
    otherMultiplier = elementU_[startColumn];
    otherRow = indexRowU_[startColumn];
  }
  int numberSave = numberInRow_[otherRow];
  double pivotMultiplier = 1.0 / pivotElement;

  pivotRegion_[numberGoodU_] = pivotMultiplier;
  numberInColumn_[pivotColumn] = 0;
  otherMultiplier = otherMultiplier * pivotMultiplier;
  indexRowL_[l] = otherRow;
  elementL_[l] = otherMultiplier;
  //take out of row list
  CoinBigIndex start = startRowU_[otherRow];
  CoinBigIndex end = start + numberSave;
  CoinBigIndex where = start;

  while ( indexColumnU_[where] != pivotColumn ) {
    where++;
  }				/* endwhile */
  assert ( where < end );
  end--;
  indexColumnU_[where] = indexColumnU_[end];
  int numberAdded = 0;
  int numberDeleted = 0;

  //pack down and move to work
  int j;

  for ( j = startRow; j < endRow; j++ ) {
    int iColumn = indexColumnU_[j];

    if ( iColumn != pivotColumn ) {
      CoinBigIndex startColumn = startColumnU_[iColumn];
      CoinBigIndex endColumn = startColumn + numberInColumn_[iColumn];
      int iRow = indexRowU_[startColumn];
      double value = elementU_[startColumn];
      double largest;
      bool foundOther = false;

      //leave room for pivot
      CoinBigIndex put = startColumn + 1;
      CoinBigIndex positionLargest = -1;
      double thisPivotValue = 0.0;
      double otherElement = 0.0;
      double nextValue = elementU_[put];;
      int nextIRow = indexRowU_[put];

      //compress column and find largest not updated
      if ( iRow != pivotRow ) {
	if ( iRow != otherRow ) {
	  largest = fabs ( value );
	  elementU_[put] = value;
	  indexRowU_[put] = iRow;
	  positionLargest = put;
	  put++;
	  CoinBigIndex i;
	  for ( i = startColumn + 1; i < endColumn; i++ ) {
	    iRow = nextIRow;
	    value = nextValue;
#ifdef ZEROFAULT
	    // doesn't matter reading uninitialized but annoys checking
	    if ( i + 1 < endColumn ) {
#endif
	      nextIRow = indexRowU_[i + 1];
	      nextValue = elementU_[i + 1];
#ifdef ZEROFAULT
	    }
#endif
	    if ( iRow != pivotRow ) {
	      if ( iRow != otherRow ) {
		//keep
		indexRowU_[put] = iRow;
		elementU_[put] = value;;
		put++;
	      } else {
		otherElement = value;
		foundOther = true;
	      }
	    } else {
	      thisPivotValue = value;
	    }
	  }
	} else {
	  otherElement = value;
	  foundOther = true;
	  //need to find largest
	  largest = 0.0;
	  CoinBigIndex i;
	  for ( i = startColumn + 1; i < endColumn; i++ ) {
	    iRow = nextIRow;
	    value = nextValue;
#ifdef ZEROFAULT
	    // doesn't matter reading uninitialized but annoys checking
	    if ( i + 1 < endColumn ) {
#endif
	      nextIRow = indexRowU_[i + 1];
	      nextValue = elementU_[i + 1];
#ifdef ZEROFAULT
	    }
#endif
	    if ( iRow != pivotRow ) {
	      //keep
	      indexRowU_[put] = iRow;
	      elementU_[put] = value;;
	      double absValue = fabs ( value );

	      if ( absValue > largest ) {
		largest = absValue;
		positionLargest = put;
	      }
	      put++;
	    } else {
	      thisPivotValue = value;
	    }
	  }
	}
      } else {
	//need to find largest
	largest = 0.0;
	thisPivotValue = value;
	CoinBigIndex i;
	for ( i = startColumn + 1; i < endColumn; i++ ) {
	  iRow = nextIRow;
	  value = nextValue;
#ifdef ZEROFAULT
	  // doesn't matter reading uninitialized but annoys checking
	  if ( i + 1 < endColumn ) {
#endif
	    nextIRow = indexRowU_[i + 1];
	    nextValue = elementU_[i + 1];
#ifdef ZEROFAULT
	  }
#endif
	  if ( iRow != otherRow ) {
	    //keep
	    indexRowU_[put] = iRow;
	    elementU_[put] = value;;
	    double absValue = fabs ( value );

	    if ( absValue > largest ) {
	      largest = absValue;
	      positionLargest = put;
	    }
	    put++;
	  } else {
	    otherElement = value;
	    foundOther = true;
	  }
	}
      }
      //slot in pivot
      elementU_[startColumn] = thisPivotValue;
      indexRowU_[startColumn] = pivotRow;
      //clean up counts
      startColumn++;
      numberInColumn_[iColumn] = put - startColumn;
      numberInColumnPlus_[iColumn]++;
      startColumnU_[iColumn]++;
      otherElement = otherElement - thisPivotValue * otherMultiplier;
      double absValue = fabs ( otherElement );

      if ( absValue > zeroTolerance_ ) {
	if ( !foundOther ) {
	  //have we space
	  saveColumn_[numberAdded++] = iColumn;
	  int next = nextColumn_[iColumn];
	  CoinBigIndex space;

	  space = startColumnU_[next] - put - numberInColumnPlus_[next];
	  if ( space <= 0 ) {
	    //getColumnSpace also moves fixed part
	    int number = numberInColumn_[iColumn];

	    if ( !getColumnSpace ( iColumn, number + 1 ) ) {
	      return false;
	    }
	    //redo starts
	    positionLargest =
	      positionLargest + startColumnU_[iColumn] - startColumn;
	    startColumn = startColumnU_[iColumn];
	    put = startColumn + number;
	  }
	}
	elementU_[put] = otherElement;
	indexRowU_[put] = otherRow;
	if ( absValue > largest ) {
	  largest = absValue;
	  positionLargest = put;
	}
	put++;
      } else {
	if ( foundOther ) {
	  numberDeleted++;
	  //take out of row list
	  CoinBigIndex where = start;

	  while ( indexColumnU_[where] != iColumn ) {
	    where++;
	  }			/* endwhile */
	  assert ( where < end );
	  end--;
	  indexColumnU_[where] = indexColumnU_[end];
	}
      }
      numberInColumn_[iColumn] = put - startColumn;
      //move largest
      if ( positionLargest >= 0 ) {
	value = elementU_[positionLargest];
	iRow = indexRowU_[positionLargest];
	elementU_[positionLargest] = elementU_[startColumn];
	indexRowU_[positionLargest] = indexRowU_[startColumn];
	elementU_[startColumn] = value;
	indexRowU_[startColumn] = iRow;
      }
      //linked list for column
      if ( nextCount_[iColumn + numberRows_] != -2 ) {
	//modify linked list
	deleteLink ( iColumn + numberRows_ );
	addLink ( iColumn + numberRows_, numberInColumn_[iColumn] );
      }
    }
  }
  //get space for row list
  next = nextRow_[otherRow];
  CoinBigIndex space;

  space = startRowU_[next] - end;
  totalElements_ += numberAdded - numberDeleted;
  int number = numberAdded + ( end - start );

  if ( space < numberAdded ) {
    numberInRow_[otherRow] = end - start;
    if ( !getRowSpace ( otherRow, number ) ) {
      return false;
    }
    end = startRowU_[otherRow] + end - start;
  }
  // do linked lists and update counts
  numberInRow_[otherRow] = number;
  if ( number != numberSave ) {
    deleteLink ( otherRow );
    addLink ( otherRow, number );
  }
  for ( j = 0; j < numberAdded; j++ ) {
    indexColumnU_[end++] = saveColumn_[j];
  }
  //modify linked list for pivots
  deleteLink ( pivotRow );
  deleteLink ( pivotColumn + numberRows_ );
  return true;
}
