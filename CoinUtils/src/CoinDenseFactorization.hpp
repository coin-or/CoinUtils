// Copyright (C) 2008, International Business Machines
// Corporation and others.  All Rights Reserved.

/* 
   Authors
   
   John Forrest

 */
#ifndef CoinDenseFactorization_H
#define CoinDenseFactorization_H

#include <iostream>
#include <string>
#include <cassert>
#include "CoinFinite.hpp"
#include "CoinIndexedVector.hpp"
class CoinPackedMatrix;
/** This deals with Factorization and Updates
    This is a simple dense version so other people can write a better one

    I am assuming that 32 bits is enough for number of rows or columns, but CoinBigIndex
    may be redefined to get 64 bits.
 */



class CoinDenseFactorization {
   friend void CoinDenseFactorizationUnitTest( const std::string & mpsDir );

public:

  /**@name Constructors and destructor and copy */
  //@{
  /// Default constructor
  CoinDenseFactorization (  );
  /// Copy constructor 
  CoinDenseFactorization ( const CoinDenseFactorization &other);
  
  /// Destructor
  ~CoinDenseFactorization (  );
  /// = copy
  CoinDenseFactorization & operator = ( const CoinDenseFactorization & other );
  //@}

  /**@name Do factorization - public */
  //@{
  /// Gets space for a factorization
  void getAreas ( int numberRows,
		  int numberColumns,
		  CoinBigIndex maximumL,
		  CoinBigIndex maximumU );
  
  /// PreProcesses column ordered copy of basis
  void preProcess ( );
  /** Does most of factorization returning status
      0 - OK
      -99 - needs more memory
      -1 - singular - use numberGoodColumns and redo
  */
  int factor ( );
  /// Does post processing on valid factorization - putting variables on correct rows
  void postProcess(const int * sequence, int * pivotVariable);
  /// Makes a non-singular basis by replacing variables
  void makeNonSingular(int * sequence, int numberColumns);
  //@}

  /**@name general stuff such as status */
  //@{ 
  /// Returns status
  inline int status (  ) const {
    return status_;
  }
  /// Sets status
  inline void setStatus (  int value)
  {  status_=value;  }
  /// Returns number of pivots since factorization
  inline int pivots (  ) const {
    return numberPivots_;
  }
  /// Number of Rows
  inline int numberRows (  ) const {
    return numberRows_;
  }
  /// Total number of columns in factorization
  inline int numberColumns (  ) const {
    return numberColumns_;
  }
  /// Total number of elements in factorization
  inline int numberElements (  ) const {
    return numberRows_*(numberColumns_+numberPivots_);
  }
  /// Number of good columns in factorization
  inline int numberGoodColumns (  ) const {
    return numberGoodU_;
  }
  /// Allows change of pivot accuracy check 1.0 == none >1.0 relaxed
  inline void relaxAccuracyCheck(double value)
  { relaxCheck_ = value;}
  inline double getAccuracyCheck() const
  { return relaxCheck_;}
  /// Maximum number of pivots between factorizations
  inline int maximumPivots (  ) const {
    return maximumPivots_ ;
  }
  void maximumPivots (  int value );

  /// Pivot tolerance
  inline double pivotTolerance (  ) const {
    return pivotTolerance_ ;
  }
  void pivotTolerance (  double value );
  /// Zero tolerance
  inline double zeroTolerance (  ) const {
    return zeroTolerance_ ;
  }
  inline void zeroTolerance (  double value )
  { zeroTolerance_ = value;}
  /// Whether slack value is +1 or -1
  inline double slackValue (  ) const {
    return slackValue_ ;
  }
  void slackValue (  double value );
  /// Returns maximum absolute value in factorization
  double maximumCoefficient() const;
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
  inline int updateColumnFT ( CoinIndexedVector * regionSparse,
		       CoinIndexedVector * regionSparse2)
  { return updateColumn(regionSparse,regionSparse2);}
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
  //@}
  /// *** Below this user may not want to know about

  /**@name various uses of factorization
   which user may not want to know about (left over from my LP code) */
  //@{
  /// Get rid of all memory
  inline void clearArrays()
  { gutsOfDestructor();}
  /// Returns array to put basis elements in
  inline double * elements() const
  { return elements_;}
  /// Returns array to put basis indices in
  inline int * indices() const
  { return (int *) (elements_+numberRows_*numberRows_);}
  /// Returns array to put basis starts in
  inline CoinBigIndex * starts() const
  { return (CoinBigIndex *) pivotRow_;}
  /// Returns pivot row 
  inline int * pivotRow() const
  { return pivotRow_;}
  /// Returns permute in
  inline int * permute() const
  { return pivotRow_;}
  /// Returns permute back
  inline int * permuteBack() const
  { return pivotRow_+numberRows_;}
  /// Returns work area
  inline double * workArea() const
  { return workArea_;}
  /// Returns int work area
  inline int * intWorkArea() const
  { return (int *) workArea_;}
  //@}

  /// The real work of desstructor 
  void gutsOfDestructor();
  /// The real work of constructor
  void gutsOfInitialize();
  /// The real work of copy
  void gutsOfCopy(const CoinDenseFactorization &other);

  //@}
protected:
  /** Returns accuracy status of replaceColumn
      returns 0=OK, 1=Probably OK, 2=singular */
  int checkPivot(double saveFromU, double oldPivot) const;
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
  /// Relax check on accuracy in replaceColumn
  double relaxCheck_;
  /// Number of Rows in factorization
  int numberRows_;
  /// Number of Columns in factorization
  int numberColumns_;
  /// Maximum rows ever (i.e. use to copy arrays etc)
  int maximumRows_;
  /// Maximum length of iterating area
  CoinBigIndex maximumSpace_;
  /// Number factorized in U (not row singletons)
  int numberGoodU_;
  /// Maximum number of pivots before factorization
  int maximumPivots_;
  /// Number pivots since last factorization
  int numberPivots_;
  /// Number of elements after factorization
  CoinBigIndex factorElements_;
  /// Pivot row 
  int * pivotRow_;
  /// Status of factorization
  int status_;
  /** Elements of factorization and updates
      length is maxR*maxR+maxSpace
      will always be long enough so can have nR*nR ints in maxSpace 
  */
  double * elements_;
  /// Work area of numberRows_ 
  double * workArea_;
  //@}
};
#endif
