// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

/* 
   Authors
   
   John Forrest

 */
#ifndef CoinFactorization_H
#define CoinFactorization_H

#include <iostream>
#include <string>
#include <cassert>
#include "CoinFinite.hpp"
class CoinPackedMatrix;
class CoinIndexedVector;
/** This deals with Factorization and Updates

    This class started with a parallel simplex code I was writing in the
    mid 90's.  The need for parallelism led to many complications and
    I have simplified as much as I could to get back to this.

    I was aiming at problems where I might get speed-up so I was looking at dense
    problems or ones with structure.  This led to permuting input and output
    vectors and to increasing the number of rows each rank-one update.  This is 
    still in as a minor overhead.

    I have also put in handling for hyper-sparsity.  I have taken out
    all outer loop unrolling, dense matrix handling and most of the
    book-keeping for slacks.  Also I always use FTRAN approach to updating
    even if factorization fairly dense.  All these could improve performance.

    I blame some of the coding peculiarities on the history of the code
    but mostly it is just because I can't do elegant code (or useful
    comments).

    I am assuming that 32 bits is enough for number of rows or columns, but CoinBigIndex
    may be redefined to get 64 bits.
 */



class CoinFactorization {
   friend void CoinFactorizationUnitTest( const std::string & mpsDir );

public:

  /**@name Constructors and destructor and copy */
  //@{
  /// Default constructor
    CoinFactorization (  );
  /// Copy constructor 
  CoinFactorization ( const CoinFactorization &other);

  /// Destructor
   ~CoinFactorization (  );
  /// Debug show object (shows one representation)
  void show_self (  ) const;
  /// Debug - save on file - 0 if no error
  int saveFactorization (const char * file  ) const;
  /** Debug - restore from file - 0 if no error on file.
      If factor true then factorizes as if called from ClpFactorization
  */
  int restoreFactorization (const char * file  , bool factor=false) ;
  /// Debug - sort so can compare
  void sort (  ) const;
  /// = copy
    CoinFactorization & operator = ( const CoinFactorization & other );
  //@}

  /**@name Do factorization */
  //@{
  /** When part of LP - given by basic variables.
  Actually does factorization.
  Arrays passed in have non negative value to say basic.
  If status is okay, basic variables have pivot row - this is only needed
  If status is singular, then basic variables have pivot row
  and ones thrown out have -1
  returns 0 -okay, -1 singular, -2 too many in basis, -99 memory */
  int factorize ( const CoinPackedMatrix & matrix, 
		  int rowIsBasic[], int columnIsBasic[] , 
		  double areaFactor = 0.0 );
  /** When given as triplets.
  Actually does factorization.  maximumL is guessed maximum size of L part of
  final factorization, maximumU of U part.  These are multiplied by
  areaFactor which can be computed by user or internally.  
  Arrays are copied in.  I could add flag to delete arrays to save a 
  bit of memory.
  If status okay, permutation has pivot rows - this is only needed
  If status is singular, then basic variables have pivot row
  and ones thrown out have -1
  returns 0 -okay, -1 singular, -99 memory */
  int factorize ( int numberRows,
		  int numberColumns,
		  CoinBigIndex numberElements,
		  CoinBigIndex maximumL,
		  CoinBigIndex maximumU,
		  const int indicesRow[],
		  const int indicesColumn[], const double elements[] ,
		  int permutation[],
		  double areaFactor = 0.0);
  /** Two part version for maximum flexibility
      This part creates arrays for user to fill.
      estimateNumberElements is safe estimate of number
      returns 0 -okay, -99 memory */
  int factorizePart1 ( int numberRows,
		       int numberColumns,
		       CoinBigIndex estimateNumberElements,
		       int * indicesRow[],
		       int * indicesColumn[],
		       double * elements[],
		  double areaFactor = 0.0);
  /** This is part two of factorization
      Arrays belong to factorization and were returned by part 1
      If status okay, permutation has pivot rows - this is only needed
      If status is singular, then basic variables have pivot row
      and ones thrown out have -1
      returns 0 -okay, -1 singular, -99 memory */
  int factorizePart2 (int permutation[],int exactNumberElements);
  //@}

  /**@name general stuff such as permutation or status */
  //@{ 
  /// Returns status
  inline int status (  ) const {
    return status_;
  };
  /// Returns number of pivots since factorization
  inline int pivots (  ) const {
    return numberPivots_;
  };
  /// Returns address of permute region
  inline int *permute (  ) const {
    return permute_;
  };
  /// Returns address of pivotColumn region (also used for permuting)
  inline int *pivotColumn (  ) const {
    return pivotColumn_;
  };
  /// Returns address of permuteBack region
  inline int *permuteBack (  ) const {
    return permuteBack_;
  };
  /// Returns address of pivotColumnBack region (also used for permuting)
  inline int *pivotColumnBack (  ) const {
    return pivotColumnBack_;
  };
  /// Number of Rows after iterating
  inline int numberRowsExtra (  ) const {
    return numberRowsExtra_;
  };
  /// Number of Rows after factorization
  inline int numberRows (  ) const {
    return numberRows_;
  };
  /// Maximum of Rows after iterating
  inline int maximumRowsExtra (  ) const {
    return maximumRowsExtra_;
  };
  /// Total number of columns in factorization
  inline int numberColumns (  ) const {
    return numberColumns_;
  };
  /// Total number of elements in factorization
  inline int numberElements (  ) const {
    return totalElements_;
  };
  /// Length of FT vector
  inline int numberForrestTomlin (  ) const {
    return numberInColumn_[numberColumnsExtra_];
  };
  /// Number of good columns in factorization
  inline int numberGoodColumns (  ) const {
    return numberGoodU_;
  };
  /// Whether larger areas needed
  inline double areaFactor (  ) const {
    return areaFactor_;
  };
  inline void areaFactor ( double value ) {
    areaFactor_=value;
  };
  /// Returns areaFactor but adjusted for dense
  double adjustedAreaFactor() const;
  /// Allows change of pivot accuracy check 1.0 == none >1.0 relaxed
  inline void relaxAccuracyCheck(double value)
  { relaxCheck_ = value;};
  inline double getAccuracyCheck() const
  { return relaxCheck_;};
  /// Whether rows increase after pivoting - dummy
  inline bool increasingRows (  ) const 
  { return true; };
  /** 0 - no increasing rows - no nothing (not coded)
      1 - no permutation (i.e. basis order in is pivot order), 
      2 user wants slacks pivoting on own rows,
      3 user needs to know everything as row are really increasing 
      - OUT so dummy */
  inline void increasingRows ( int value  ) {};
  /// Level of detail of messages
  inline int messageLevel (  ) const {
    return messageLevel_ ;
  };
  void messageLevel (  int value );
  /// Maximum number of pivots between factorizations
  inline int maximumPivots (  ) const {
    return maximumPivots_ ;
  };
  void maximumPivots (  int value );

  /// Gets dense threshold
  inline int denseThreshold() const 
    { return denseThreshold_;};
  /// Sets dense threshold
  inline void setDenseThreshold(int value)
    { denseThreshold_ = value;};
  /// Pivot tolerance
  inline double pivotTolerance (  ) const {
    return pivotTolerance_ ;
  };
  void pivotTolerance (  double value );
  /// Zero tolerance
  inline double zeroTolerance (  ) const {
    return zeroTolerance_ ;
  };
  void zeroTolerance (  double value );
  /// Whether slack value is +1 or -1
  inline double slackValue (  ) const {
    return slackValue_ ;
  };
  void slackValue (  double value );
  /// Returns maximum absolute value in factorization
  double maximumCoefficient() const;
  /// true if Forrest Tomlin update, false if PFI 
  inline bool forrestTomlin() const
  { return doForrestTomlin_;};
  inline void setForrestTomlin(bool value)
  { doForrestTomlin_=value;};
  //@}

  /**@name some simple stuff */
  //@{

  /// Returns number of dense rows
  inline int numberDense() const
  { return numberDense_;};

  /// Returns number in U area
  inline CoinBigIndex numberElementsU (  ) const {
    return lengthU_;
  };
  /// Returns length of U area
  inline CoinBigIndex lengthAreaU (  ) const {
    return lengthAreaU_;
  };
  /// Returns number in L area
  inline CoinBigIndex numberElementsL (  ) const {
    return lengthL_;
  };
  /// Returns length of L area
  inline CoinBigIndex lengthAreaL (  ) const {
    return lengthAreaL_;
  };
  /// Returns number in R area
  inline CoinBigIndex numberElementsR (  ) const {
    return lengthR_;
  };
  /// Number of compressions done
  inline CoinBigIndex numberCompressions() const
  { return numberCompressions_;};
  /** L to U bias
      0 - U bias, 1 - some U bias, 2 some L bias, 3 L bias
  */
  inline int biasLU() const
  { return biasLU_;};
  inline void setBiasLU(int value)
  { biasLU_=value;};
  //@}

  /**@name rank one updates which do exist */
  //@{

  /** Replaces one Column to basis,
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room
      If checkBeforeModifying is true will do all accuracy checks
      before modifying factorization.  Whether to set this depends on
      speed considerations.  You could just do this on first iteration
      after factorization and thereafter re-factorize
   partial update already in U */
  int replaceColumn ( CoinIndexedVector * regionSparse,
		      int pivotRow,
		      double pivotCheck ,
		      bool checkBeforeModifying=false);
  //@}

  /**@name various uses of factorization (return code number elements) 
   which user may want to know about */
  //@{
  /** Updates one column (FTRAN) from regionSparse2
      Tries to do FT update
      number returned is negative if no room
      regionSparse starts as zero and is zero at end.
      Note - if regionSparse2 packed on input - will be packed on output
  */
  int updateColumnFT ( CoinIndexedVector * regionSparse,
		       CoinIndexedVector * regionSparse2);
  /** This version has same effect as above with FTUpdate==false
      so number returned is always >=0 */
  int updateColumn ( CoinIndexedVector * regionSparse,
		     CoinIndexedVector * regionSparse2,
		     bool noPermute=false) const;
  /** Updates one column (BTRAN) from regionSparse2
      regionSparse starts as zero and is zero at end 
      Note - if regionSparse2 packed on input - will be packed on output
  */
  int updateColumnTranspose ( CoinIndexedVector * regionSparse,
			      CoinIndexedVector * regionSparse2) const;
  /** makes a row copy of L for speed and to allow very sparse problems */
  void goSparse();
  /**  get sparse threshold */
  int sparseThreshold ( ) const;
  /**  set sparse threshold */
  void sparseThreshold ( int value );
  //@}
  /// *** Below this user may not want to know about

  /**@name various uses of factorization (return code number elements) 
   which user may not want to know about (left over from my LP code) */
  //@{
  /// Get rid of all memory
  inline void clearArrays()
  { gutsOfDestructor();};
  //@}

  /**@name various updates - none of which have been written! */
  //@{

  /** Adds given elements to Basis and updates factorization,
      can increase size of basis. Returns rank */
  int add ( CoinBigIndex numberElements,
	       int indicesRow[],
	       int indicesColumn[], double elements[] );

  /** Adds one Column to basis,
      can increase size of basis. Returns rank */
  int addColumn ( CoinBigIndex numberElements,
		     int indicesRow[], double elements[] );

  /** Adds one Row to basis,
      can increase size of basis. Returns rank */
  int addRow ( CoinBigIndex numberElements,
		  int indicesColumn[], double elements[] );

  /// Deletes one Column from basis, returns rank
  int deleteColumn ( int Row );
  /// Deletes one Row from basis, returns rank
  int deleteRow ( int Row );

  /** Replaces one Row in basis,
      At present assumes just a singleton on row is in basis
      returns 0=OK, 1=Probably OK, 2=singular, 3 no space */
  int replaceRow ( int whichRow, int numberElements,
		      const int indicesColumn[], const double elements[] );
  /// Takes out all entries for given rows
  void emptyRows(int numberToEmpty, const int which[]);
  //@}
protected:

  /**@name used by factorization */
  /// Gets space for a factorization, called by constructors
  void getAreas ( int numberRows,
		  int numberColumns,
		  CoinBigIndex maximumL,
		  CoinBigIndex maximumU );

  /** PreProcesses raw triplet data.
      state is 0 - triplets, 1 - some counts etc , 2 - .. */
  void preProcess ( int state,
		    int possibleDuplicates = -1 );
  /// Does most of factorization
  int factor (  );
  /// Does sparse phase of factorization
  /// return code is <0 error, 0= finished
  int factorSparse (  );
  /// Does dense phase of factorization
  /// return code is <0 error, 0= finished
  int factorDense (  );

  /// Pivots when just one other row so faster?
  bool pivotOneOtherRow ( int pivotRow,
			  int pivotColumn );
  /// Does one pivot on Row Singleton in factorization
  bool pivotRowSingleton ( int pivotRow,
			   int pivotColumn );
  /// Does one pivot on Column Singleton in factorization
  bool pivotColumnSingleton ( int pivotRow,
			      int pivotColumn );

  /** Gets space for one Column with given length,
   may have to do compression  (returns True if successful),
   also moves existing vector,
   extraNeeded is over and above present */
  bool getColumnSpace ( int iColumn,
			int extraNeeded );

  /**  getColumnSpaceIterateR.  Gets space for one extra R element in Column
       may have to do compression  (returns true)
       also moves existing vector */
  bool getColumnSpaceIterateR ( int iColumn, double value,
			       int iRow);
  /**  getColumnSpaceIterate.  Gets space for one extra U element in Column
       may have to do compression  (returns true)
       also moves existing vector.
       Returns -1 if no memory or where element was put
       Used by replaceRow (turns off R version) */
  CoinBigIndex getColumnSpaceIterate ( int iColumn, double value,
			       int iRow);
  /** Gets space for one Row with given length,
  may have to do compression  (returns True if successful),
  also moves existing vector */
  bool getRowSpace ( int iRow, int extraNeeded );

  /** Gets space for one Row with given length while iterating,
  may have to do compression  (returns True if successful),
  also moves existing vector */
  bool getRowSpaceIterate ( int iRow,
			    int extraNeeded );
  /// Checks that row and column copies look OK
  void checkConsistency (  );
  /// Adds a link in chain of equal counts
  inline void addLink ( int index, int count ) {
    int next = firstCount_[count];
      lastCount_[index] = -2 - count;
    if ( next < 0 ) {
      //first with that count
      firstCount_[count] = index;
      nextCount_[index] = -1;
    } else {
      firstCount_[count] = index;
      nextCount_[index] = next;
      lastCount_[next] = index;
  }};
  /// Deletes a link in chain of equal counts
  inline void deleteLink ( int index ) {
    int next = nextCount_[index];
    int last = lastCount_[index];
    if ( last >= 0 ) {
      nextCount_[last] = next;
    } else {
      int count = -last - 2;

      firstCount_[count] = next;
    }
    if ( next >= 0 ) {
      lastCount_[next] = last;
    }
    nextCount_[index] = -2;
    lastCount_[index] = -2;
    return;
  };
  /// Separate out links with same row/column count
  void separateLinks(int count,bool rowsFirst);
  /// Cleans up at end of factorization
  void cleanup (  );

  /// Updates part of column (FTRANL)
  void updateColumnL ( CoinIndexedVector * region, int * indexIn ) const;
  /// Updates part of column (FTRANL) when densish
  void updateColumnLDensish ( CoinIndexedVector * region, int * indexIn ) const;
  /// Updates part of column (FTRANL) when sparse
  void updateColumnLSparse ( CoinIndexedVector * region, int * indexIn ) const;
  /// Updates part of column (FTRANL) when sparsish
  void updateColumnLSparsish ( CoinIndexedVector * region, int * indexIn ) const;

  /// Updates part of column (FTRANR) without FT update
  void updateColumnR ( CoinIndexedVector * region ) const;
  /** Updates part of column (FTRANR) with FT update.
      Also stores update after L and R */
  void updateColumnRFT ( CoinIndexedVector * region, int * indexIn );

  /// Updates part of column (FTRANU)
  void updateColumnU ( CoinIndexedVector * region, int * indexIn) const;

  /// Updates part of column (FTRANU) when sparse
  void updateColumnUSparse ( CoinIndexedVector * regionSparse, 
			     int * indexIn) const;
  /// Updates part of column (FTRANU) when sparsish
  void updateColumnUSparsish ( CoinIndexedVector * regionSparse, 
			       int * indexIn) const;
  /// Updates part of column (FTRANU)
  void updateColumnUDensish ( CoinIndexedVector * regionSparse, 
			      int * indexIn) const;
  /// Updates part of column PFI (FTRAN) (after rest)
  void updateColumnPFI ( CoinIndexedVector * regionSparse) const; 
  /// Permutes back at end of updateColumn
  void permuteBack ( CoinIndexedVector * regionSparse, 
		     CoinIndexedVector * outVector) const;

  /// Updates part of column transpose PFI (BTRAN) (before rest)
  void updateColumnTransposePFI ( CoinIndexedVector * region) const;
  /** Updates part of column transpose (BTRANU),
      assumes index is sorted i.e. region is correct */
  void updateColumnTransposeU ( CoinIndexedVector * region,
				int smallestIndex) const;
  /** Updates part of column transpose (BTRANU) when sparsish,
      assumes index is sorted i.e. region is correct */
  void updateColumnTransposeUSparsish ( CoinIndexedVector * region,
					int smallestIndex) const;
  /** Updates part of column transpose (BTRANU) when densish,
      assumes index is sorted i.e. region is correct */
  void updateColumnTransposeUDensish ( CoinIndexedVector * region,
				       int smallestIndex) const;
  /** Updates part of column transpose (BTRANU) when sparse,
      assumes index is sorted i.e. region is correct */
  void updateColumnTransposeUSparse ( CoinIndexedVector * region) const;

  /// Updates part of column transpose (BTRANR)
  void updateColumnTransposeR ( CoinIndexedVector * region ) const;
  /// Updates part of column transpose (BTRANR) when dense
  void updateColumnTransposeRDensish ( CoinIndexedVector * region ) const;
  /// Updates part of column transpose (BTRANR) when sparse
  void updateColumnTransposeRSparse ( CoinIndexedVector * region ) const;

  /// Updates part of column transpose (BTRANL)
  void updateColumnTransposeL ( CoinIndexedVector * region ) const;
  /// Updates part of column transpose (BTRANL) when densish by column
  void updateColumnTransposeLDensish ( CoinIndexedVector * region ) const;
  /// Updates part of column transpose (BTRANL) when densish by row
  void updateColumnTransposeLByRow ( CoinIndexedVector * region ) const;
  /// Updates part of column transpose (BTRANL) when sparsish by row
  void updateColumnTransposeLSparsish ( CoinIndexedVector * region ) const;
  /// Updates part of column transpose (BTRANL) when sparse (by Row)
  void updateColumnTransposeLSparse ( CoinIndexedVector * region ) const;
  /** Replaces one Column to basis for PFI
   returns 0=OK, 1=Probably OK, 2=singular, 3=no room.
   In this case region is not empty - it is incoming variable (updated)
  */
  int replaceColumnPFI ( CoinIndexedVector * regionSparse,
			 int pivotRow, double alpha);

  /** Returns accuracy status of replaceColumn
      returns 0=OK, 1=Probably OK, 2=singular */
  int checkPivot(double saveFromU, double oldPivot) const;
  /// The real work of constructors etc 
  void gutsOfDestructor();
  /// 1 bit - tolerances etc, 2 more, 4 dummy arrays
  void gutsOfInitialize(int type);
  void gutsOfCopy(const CoinFactorization &other);

  /// Reset all sparsity etc statistics
  void resetStatistics();

  /********************************* START LARGE TEMPLATE ********/
#ifdef INT_IS_8
#define COINFACTORIZATION_BITS_PER_INT 64
#define COINFACTORIZATION_SHIFT_PER_INT 6
#define COINFACTORIZATION_MASK_PER_INT 0x3f
#else
#define COINFACTORIZATION_BITS_PER_INT 32
#define COINFACTORIZATION_SHIFT_PER_INT 5
#define COINFACTORIZATION_MASK_PER_INT 0x1f
#endif
  template <class T>  bool
  pivot ( int pivotRow,
	  int pivotColumn,
	  CoinBigIndex pivotRowPosition,
	  CoinBigIndex pivotColumnPosition,
	  double work[],
	  unsigned int workArea2[],
	  int increment,
	  int increment2,
	  T markRow[] ,
	  int largeInteger)
{
  int *indexColumnU = indexColumnU_;
  CoinBigIndex *startColumnU = startColumnU_;
  int *numberInColumn = numberInColumn_;
  double *elementU = elementU_;
  int *indexRowU = indexRowU_;
  CoinBigIndex *startRowU = startRowU_;
  int *numberInRow = numberInRow_;
  double *elementL = elementL_;
  int *indexRowL = indexRowL_;
  int *saveColumn = saveColumn_;
  int *nextRow = nextRow_;
  int *lastRow = lastRow_;

  //store pivot columns (so can easily compress)
  int numberInPivotRow = numberInRow[pivotRow] - 1;
  CoinBigIndex startColumn = startColumnU[pivotColumn];
  int numberInPivotColumn = numberInColumn[pivotColumn] - 1;
  CoinBigIndex endColumn = startColumn + numberInPivotColumn + 1;
  int put = 0;
  CoinBigIndex startRow = startRowU[pivotRow];
  CoinBigIndex endRow = startRow + numberInPivotRow + 1;

  if ( pivotColumnPosition < 0 ) {
    for ( pivotColumnPosition = startRow; pivotColumnPosition < endRow; pivotColumnPosition++ ) {
      int iColumn = indexColumnU[pivotColumnPosition];
      if ( iColumn != pivotColumn ) {
	saveColumn[put++] = iColumn;
      } else {
        break;
      }
    }
  } else {
    for (CoinBigIndex i = startRow ; i < pivotColumnPosition ; i++ ) {
      saveColumn[put++] = indexColumnU[i];
    }
  }
  assert (pivotColumnPosition<endRow);
  assert (indexColumnU[pivotColumnPosition]==pivotColumn);
  pivotColumnPosition++;
  for ( ; pivotColumnPosition < endRow; pivotColumnPosition++ ) {
    saveColumn[put++] = indexColumnU[pivotColumnPosition];
  }
  //take out this bit of indexColumnU
  int next = nextRow[pivotRow];
  int last = lastRow[pivotRow];

  nextRow[last] = next;
  lastRow[next] = last;
  nextRow[pivotRow] = numberGoodU_;	//use for permute
  lastRow[pivotRow] = -2;
  numberInRow[pivotRow] = 0;
  //store column in L, compress in U and take column out
  CoinBigIndex l = lengthL_;

  if ( l + numberInPivotColumn > lengthAreaL_ ) {
    //need more memory
    printf("more memory needed in middle of invert\n");
    return false;
  }
  //l+=currentAreaL_->elementByColumn-elementL;
  CoinBigIndex lSave = l;

  pivotRowL_[numberGoodL_] = pivotRow;
  startColumnL_[numberGoodL_] = l;	//for luck and first time
  numberGoodL_++;
  startColumnL_[numberGoodL_] = l + numberInPivotColumn;
  lengthL_ += numberInPivotColumn;
  if ( pivotRowPosition < 0 ) {
    for ( pivotRowPosition = startColumn; pivotRowPosition < endColumn; pivotRowPosition++ ) {
      int iRow = indexRowU[pivotRowPosition];
      if ( iRow != pivotRow ) {
	indexRowL[l] = iRow;
	elementL[l] = elementU[pivotRowPosition];
	markRow[iRow] = l - lSave;
	l++;
	//take out of row list
	CoinBigIndex start = startRowU[iRow];
	CoinBigIndex end = start + numberInRow[iRow];
	CoinBigIndex where = start;

	while ( indexColumnU[where] != pivotColumn ) {
	  where++;
	}			/* endwhile */
#if DEBUG_COIN
	if ( where >= end ) {
	  abort (  );
	}
#endif
	indexColumnU[where] = indexColumnU[end - 1];
	numberInRow[iRow]--;
      } else {
	break;
      }
    }
  } else {
    CoinBigIndex i;

    for ( i = startColumn; i < pivotRowPosition; i++ ) {
      int iRow = indexRowU[i];

      markRow[iRow] = l - lSave;
      indexRowL[l] = iRow;
      elementL[l] = elementU[i];
      l++;
      //take out of row list
      CoinBigIndex start = startRowU[iRow];
      CoinBigIndex end = start + numberInRow[iRow];
      CoinBigIndex where = start;

      while ( indexColumnU[where] != pivotColumn ) {
	where++;
      }				/* endwhile */
#if DEBUG_COIN
      if ( where >= end ) {
	abort (  );
      }
#endif
      indexColumnU[where] = indexColumnU[end - 1];
      numberInRow[iRow]--;
      assert (numberInRow[iRow]>=0);
    }
  }
  assert (pivotRowPosition<endColumn);
  assert (indexRowU[pivotRowPosition]==pivotRow);
  double pivotElement = elementU[pivotRowPosition];
  double pivotMultiplier = 1.0 / pivotElement;

  pivotRegion_[numberGoodU_] = pivotMultiplier;
  pivotRowPosition++;
  for ( ; pivotRowPosition < endColumn; pivotRowPosition++ ) {
    int iRow = indexRowU[pivotRowPosition];
    
    markRow[iRow] = l - lSave;
    indexRowL[l] = iRow;
    elementL[l] = elementU[pivotRowPosition];
    l++;
    //take out of row list
    CoinBigIndex start = startRowU[iRow];
    CoinBigIndex end = start + numberInRow[iRow];
    CoinBigIndex where = start;
    
    while ( indexColumnU[where] != pivotColumn ) {
      where++;
    }				/* endwhile */
#if DEBUG_COIN
    if ( where >= end ) {
      abort (  );
    }
#endif
    indexColumnU[where] = indexColumnU[end - 1];
    numberInRow[iRow]--;
    assert (numberInRow[iRow]>=0);
  }
  markRow[pivotRow] = largeInteger;
  //compress pivot column (move pivot to front including saved)
  numberInColumn[pivotColumn] = 0;
  //use end of L for temporary space
  int *indexL = &indexRowL[lSave];
  double *multipliersL = &elementL[lSave];

  //adjust
  int j;

  for ( j = 0; j < numberInPivotColumn; j++ ) {
    multipliersL[j] *= pivotMultiplier;
  }
  //zero out fill
  CoinBigIndex iErase;
  for ( iErase = 0; iErase < increment2 * numberInPivotRow;
	iErase++ ) {
    workArea2[iErase] = 0;
  }
  CoinBigIndex added = numberInPivotRow * numberInPivotColumn;
  unsigned int *temp2 = workArea2;

  //pack down and move to work
  int jColumn;
  for ( jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
    int iColumn = saveColumn[jColumn];
    CoinBigIndex startColumn = startColumnU[iColumn];
    CoinBigIndex endColumn = startColumn + numberInColumn[iColumn];
    int iRow = indexRowU[startColumn];
    double value = elementU[startColumn];
    double largest;
    CoinBigIndex put = startColumn;
    CoinBigIndex positionLargest = -1;
    double thisPivotValue = 0.0;

    //compress column and find largest not updated
    bool checkLargest;
    int mark = markRow[iRow];

    if ( mark < 0 ) {
      largest = fabs ( value );
      positionLargest = put;
      put++;
      checkLargest = false;
    } else {
      //need to find largest
      largest = 0.0;
      checkLargest = true;
      if ( mark != largeInteger ) {
	//will be updated
	work[mark] = value;
	int word = mark >> COINFACTORIZATION_SHIFT_PER_INT;
	int bit = mark & COINFACTORIZATION_MASK_PER_INT;

	temp2[word] = temp2[word] | ( 1 << bit );	//say already in counts
	added--;
      } else {
	thisPivotValue = value;
      }
    }
    CoinBigIndex i;
    for ( i = startColumn + 1; i < endColumn; i++ ) {
      iRow = indexRowU[i];
      value = elementU[i];
      int mark = markRow[iRow];

      if ( mark < 0 ) {
	//keep
	indexRowU[put] = iRow;
	elementU[put] = value;;
	if ( checkLargest ) {
	  double absValue = fabs ( value );

	  if ( absValue > largest ) {
	    largest = absValue;
	    positionLargest = put;
	  }
	}
	put++;
      } else if ( mark != largeInteger ) {
	//will be updated
	work[mark] = value;;
	int word = mark >> COINFACTORIZATION_SHIFT_PER_INT;
	int bit = mark & COINFACTORIZATION_MASK_PER_INT;

	temp2[word] = temp2[word] | ( 1 << bit );	//say already in counts
	added--;
      } else {
	thisPivotValue = value;
      }
    }
    //slot in pivot
    elementU[put] = elementU[startColumn];
    indexRowU[put] = indexRowU[startColumn];
    if ( positionLargest == startColumn ) {
      positionLargest = put;	//follow if was largest
    }
    put++;
    elementU[startColumn] = thisPivotValue;
    indexRowU[startColumn] = pivotRow;
    //clean up counts
    startColumn++;
    numberInColumn[iColumn] = put - startColumn;
    numberInColumnPlus_[iColumn]++;
    startColumnU[iColumn]++;
    //how much space have we got
    int next = nextColumn_[iColumn];
    CoinBigIndex space;

    space = startColumnU[next] - put - numberInColumnPlus_[next];
    //assume no zero elements
    if ( numberInPivotColumn > space ) {
      //getColumnSpace also moves fixed part
      if ( !getColumnSpace ( iColumn, numberInPivotColumn ) ) {
	return false;
      }
      //redo starts
      positionLargest = positionLargest + startColumnU[iColumn] - startColumn;
      startColumn = startColumnU[iColumn];
      put = startColumn + numberInColumn[iColumn];
    }
    double tolerance = zeroTolerance_;

    for ( j = 0; j < numberInPivotColumn; j++ ) {
      value = work[j] - thisPivotValue * multipliersL[j];
      double absValue = fabs ( value );

      if ( absValue > tolerance ) {
	work[j] = 0.0;
	elementU[put] = value;
	indexRowU[put] = indexL[j];
	if ( absValue > largest ) {
	  largest = absValue;
	  positionLargest = put;
	}
	put++;
      } else {
	work[j] = 0.0;
	added--;
	int word = j >> COINFACTORIZATION_SHIFT_PER_INT;
	int bit = j & COINFACTORIZATION_MASK_PER_INT;

	if ( temp2[word] & ( 1 << bit ) ) {
	  //take out of row list
	  iRow = indexL[j];
	  CoinBigIndex start = startRowU[iRow];
	  CoinBigIndex end = start + numberInRow[iRow];
	  CoinBigIndex where = start;

	  while ( indexColumnU[where] != iColumn ) {
	    where++;
	  }			/* endwhile */
#if DEBUG_COIN
	  if ( where >= end ) {
	    abort (  );
	  }
#endif
	  indexColumnU[where] = indexColumnU[end - 1];
	  numberInRow[iRow]--;
	} else {
	  //make sure won't be added
	  int word = j >> COINFACTORIZATION_SHIFT_PER_INT;
	  int bit = j & COINFACTORIZATION_MASK_PER_INT;

	  temp2[word] = temp2[word] | ( 1 << bit );	//say already in counts
	}
      }
    }
    numberInColumn[iColumn] = put - startColumn;
    //move largest
    if ( positionLargest >= 0 ) {
      value = elementU[positionLargest];
      iRow = indexRowU[positionLargest];
      elementU[positionLargest] = elementU[startColumn];
      indexRowU[positionLargest] = indexRowU[startColumn];
      elementU[startColumn] = value;
      indexRowU[startColumn] = iRow;
    }
    //linked list for column
    if ( nextCount_[iColumn + numberRows_] != -2 ) {
      //modify linked list
      deleteLink ( iColumn + numberRows_ );
      addLink ( iColumn + numberRows_, numberInColumn[iColumn] );
    }
    temp2 += increment2;
  }
  //get space for row list
  unsigned int *putBase = workArea2;
  int bigLoops = numberInPivotColumn >> COINFACTORIZATION_SHIFT_PER_INT;
  int i = 0;

  // do linked lists and update counts
  while ( bigLoops ) {
    bigLoops--;
    int bit;
    for ( bit = 0; bit < COINFACTORIZATION_BITS_PER_INT; i++, bit++ ) {
      unsigned int *putThis = putBase;
      int iRow = indexL[i];

      //get space
      int number = 0;
      int jColumn;

      for ( jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
	unsigned int test = *putThis;

	putThis += increment2;
	test = 1 - ( ( test >> bit ) & 1 );
	number += test;
      }
      int next = nextRow[iRow];
      CoinBigIndex space;

      space = startRowU[next] - startRowU[iRow];
      number += numberInRow[iRow];
      if ( space < number ) {
	if ( !getRowSpace ( iRow, number ) ) {
	  return false;
	}
      }
      // now do
      putThis = putBase;
      next = nextRow[iRow];
      number = numberInRow[iRow];
      CoinBigIndex end = startRowU[iRow] + number;
      int saveIndex = indexColumnU[startRowU[next]];

      //add in
      for ( jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
	unsigned int test = *putThis;

	putThis += increment2;
	test = 1 - ( ( test >> bit ) & 1 );
	indexColumnU[end] = saveColumn[jColumn];
	end += test;
      }
      //put back next one in case zapped
      indexColumnU[startRowU[next]] = saveIndex;
      markRow[iRow] = -1;
      number = end - startRowU[iRow];
      numberInRow[iRow] = number;
      deleteLink ( iRow );
      addLink ( iRow, number );
    }
    putBase++;
  }				/* endwhile */
  int bit;

  for ( bit = 0; i < numberInPivotColumn; i++, bit++ ) {
    unsigned int *putThis = putBase;
    int iRow = indexL[i];

    //get space
    int number = 0;
    int jColumn;

    for ( jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
      unsigned int test = *putThis;

      putThis += increment2;
      test = 1 - ( ( test >> bit ) & 1 );
      number += test;
    }
    int next = nextRow[iRow];
    CoinBigIndex space;

    space = startRowU[next] - startRowU[iRow];
    number += numberInRow[iRow];
    if ( space < number ) {
      if ( !getRowSpace ( iRow, number ) ) {
	return false;
      }
    }
    // now do
    putThis = putBase;
    next = nextRow[iRow];
    number = numberInRow[iRow];
    CoinBigIndex end = startRowU[iRow] + number;
    int saveIndex;

    saveIndex = indexColumnU[startRowU[next]];

    //add in
    for ( jColumn = 0; jColumn < numberInPivotRow; jColumn++ ) {
      unsigned int test = *putThis;

      putThis += increment2;
      test = 1 - ( ( test >> bit ) & 1 );

      indexColumnU[end] = saveColumn[jColumn];
      end += test;
    }
    indexColumnU[startRowU[next]] = saveIndex;
    markRow[iRow] = -1;
    number = end - startRowU[iRow];
    numberInRow[iRow] = number;
    deleteLink ( iRow );
    addLink ( iRow, number );
  }
  markRow[pivotRow] = -1;
  //modify linked list for pivots
  deleteLink ( pivotRow );
  deleteLink ( pivotColumn + numberRows_ );
  totalElements_ += added;
  return true;
}

  /********************************* END LARGE TEMPLATE ********/
  //@}
////////////////// data //////////////////
protected:

  /**@name data */
  //@{
  /// Pivot tolerance
  double pivotTolerance_;
  /// Zero tolerance
  double zeroTolerance_;
  /// Whether slack value is  +1 or -1
  double slackValue_;
  /// How much to multiply areas by
  double areaFactor_;
  /// Relax check on accuracy in replaceColumn
  double relaxCheck_;
  /// Number of Rows in factorization
  int numberRows_;
  /// Number of Rows after iterating
  int numberRowsExtra_;
  /// Maximum number of Rows after iterating
  int maximumRowsExtra_;
  /// Number of Columns in factorization
  int numberColumns_;
  /// Number of Columns after iterating
  int numberColumnsExtra_;
  /// Maximum number of Columns after iterating
  int maximumColumnsExtra_;
  /// Number factorized in U (not row singletons)
  int numberGoodU_;
  /// Number factorized in L
  int numberGoodL_;
  /// Maximum number of pivots before factorization
  int maximumPivots_;
  /// Number pivots since last factorization
  int numberPivots_;
  /// Number of elements in U (to go)
  ///       or while iterating total overall
  CoinBigIndex totalElements_;
  /// Number of elements after factorization
  CoinBigIndex factorElements_;
  /// Pivot order for each Column
  int *pivotColumn_;
  /// Permutation vector for pivot row order
  int *permute_;
  /// DePermutation vector for pivot row order
  int *permuteBack_;
  /// Inverse Pivot order for each Column
  int *pivotColumnBack_;
  /// Status of factorization
  int status_;

  /** 0 - no increasing rows - no permutations,
   1 - no increasing rows but permutations 
   2 - increasing rows 
     - taken out as always 2 */
  //int increasingRows_;

  /// Detail in messages
  int messageLevel_;

  /// Number of trials before rejection
  int numberTrials_;
  /// Start of each Row as pointer
  CoinBigIndex *startRowU_;

  /// Number in each Row
  int *numberInRow_;

  /// Number in each Column
  int *numberInColumn_;

  /// Number in each Column including pivoted
  int *numberInColumnPlus_;

  /** First Row/Column with count of k,
      can tell which by offset - Rows then Columns */
  int *firstCount_;

  /// Next Row/Column with count
  int *nextCount_;

  /// Previous Row/Column with count
  int *lastCount_;

  /// Next Column in memory order
  int *nextColumn_;

  /// Previous Column in memory order
  int *lastColumn_;

  /// Next Row in memory order
  int *nextRow_;

  /// Previous Row in memory order
  int *lastRow_;

  /// Columns left to do in a single pivot
  int *saveColumn_;

  /// Marks rows to be updated
  int *markRow_;

  /// Larger of row and column size
  int biggerDimension_;

  /// Base address for U (may change)
  int *indexColumnU_;

  /// Pivots for L
  int *pivotRowL_;

  /// Inverses of pivot values
  double *pivotRegion_;

  /// Number of slacks at beginning of U
  int numberSlacks_;

  /// Number in U
  int numberU_;

  /// Maximum space used in U
  CoinBigIndex maximumU_;

  /// Base of U is always 0
  //int baseU_;

  /// Length of U
  CoinBigIndex lengthU_;

  /// Length of area reserved for U
  CoinBigIndex lengthAreaU_;

/// Elements of U
  double *elementU_;

/// Row indices of U
  int *indexRowU_;

/// Start of each column in U
  CoinBigIndex *startColumnU_;

/// Converts rows to columns in U 
  CoinBigIndex *convertRowToColumnU_;

  /// Number in L
  int numberL_;

/// Base of L
  int baseL_;

  /// Length of L
  CoinBigIndex lengthL_;

  /// Length of area reserved for L
  CoinBigIndex lengthAreaL_;

  /// Elements of L
  double *elementL_;

  /// Row indices of L
  int *indexRowL_;

  /// Start of each column in L
  CoinBigIndex *startColumnL_;

  /// Number in R
  int numberR_;

  /// Length of R stuff
  CoinBigIndex lengthR_;

  /// length of area reserved for R
  CoinBigIndex lengthAreaR_;

  /// Elements of R
  double *elementR_;

  /// Row indices for R
  int *indexRowR_;

  /// Start of columns for R
  CoinBigIndex *startColumnR_;

  /// Dense area
  double  * denseArea_;

  /// Dense permutation
  int * densePermute_;

  /// Number of dense rows
  int numberDense_;

  /// Dense threshold
  int denseThreshold_;

  /// Number of compressions done
  CoinBigIndex numberCompressions_;

  /// true if Forrest Tomlin update, false if PFI 
  bool doForrestTomlin_;

  /// For statistics 
  mutable bool collectStatistics_;

  /// Below are all to collect
  mutable double ftranCountInput_;
  mutable double ftranCountAfterL_;
  mutable double ftranCountAfterR_;
  mutable double ftranCountAfterU_;
  mutable double btranCountInput_;
  mutable double btranCountAfterU_;
  mutable double btranCountAfterR_;
  mutable double btranCountAfterL_;

  /// We can roll over factorizations
  mutable int numberFtranCounts_;
  mutable int numberBtranCounts_;

  /// While these are average ratios collected over last period
  double ftranAverageAfterL_;
  double ftranAverageAfterR_;
  double ftranAverageAfterU_;
  double btranAverageAfterU_;
  double btranAverageAfterR_;
  double btranAverageAfterL_;

  /// Below this use sparse technology - if 0 then no L row copy
  int sparseThreshold_;

  /// And one for "sparsish"
  int sparseThreshold2_;

  /// Start of each row in L
  CoinBigIndex * startRowL_;

  /// Index of column in row for L
  int * indexColumnL_;

  /// Elements in L (row copy)
  double * elementByRowL_;

  /// Sparse regions
  mutable int * sparse_;
  /** L to U bias
      0 - U bias, 1 - some U bias, 2 some L bias, 3 L bias
  */
  int biasLU_;
  //@}
};
// Dense coding
#ifdef COIN_HAS_LAPACK
#define DENSE_CODE 1
/* Type of Fortran integer translated into C */
#ifndef ipfint
//typedef ipfint FORTRAN_INTEGER_TYPE ;
typedef int ipfint;
typedef const int cipfint;
#endif
#endif
#endif
