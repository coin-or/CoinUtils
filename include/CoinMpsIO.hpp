// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
#ifndef CoinMpsIO_H
#define CoinMpsIO_H

#include "CoinPackedMatrix.hpp"
#include "CoinMessageHandler.hpp"
//#############################################################################

/** MPS IO Interface

    This can be used to read in mps files without a solver.
    After reading the file this contains all relevant data which
    may be more than the CoinSolverInterface allows for.  Items may be
    deleted to allow for flexibility of data storage.

    This design makes it look very like a dummy solver as the same
    conventions are used.
*/

class CoinMpsIO {
   friend void CoinMpsIOUnitTest(const std::string & mpsDir);

public:
  /**@name Problem information methods 

   These methods return
   information about the problem referred to by the current object.
   Querying a problem that has no data associated with it result in
   zeros for the number of rows and columns, and NULL pointers from
   the methods that return vectors.

   Const pointers returned from any data-query method are always valid
  */
  //@{
    /// Get number of columns
    int getNumCols() const;

    /// Get number of rows
    int getNumRows() const;

    /// Get number of nonzero elements
    int getNumElements() const;

    /// Get pointer to array[getNumCols()] of column lower bounds
    const double * getColLower() const;

    /// Get pointer to array[getNumCols()] of column upper bounds
    const double * getColUpper() const;

    /** Get pointer to array[getNumRows()] of row constraint senses.
	<ul>
	<li>'L': <= constraint
	<li>'E': =  constraint
	<li>'G': >= constraint
	<li>'R': ranged constraint
	<li>'N': free constraint
	</ul>
    */
    const char * getRowSense() const;

    /** Get pointer to array[getNumRows()] of rows right-hand sides
	<ul>
	  <li> if rowsense()[i] == 'L' then rhs()[i] == rowupper()[i]
	  <li> if rowsense()[i] == 'G' then rhs()[i] == rowlower()[i]
	  <li> if rowsense()[i] == 'R' then rhs()[i] == rowupper()[i]
	  <li> if rowsense()[i] == 'N' then rhs()[i] == 0.0
	</ul>
    */
    const double * getRightHandSide() const;

    /** Get pointer to array[getNumRows()] of row ranges.
	<ul>
          <li> if rowsense()[i] == 'R' then
                  rowrange()[i] == rowupper()[i] - rowlower()[i]
          <li> if rowsense()[i] != 'R' then
                  rowrange()[i] is 0.0
        </ul>
    */
    const double * getRowRange() const;

    /// Get pointer to array[getNumRows()] of row lower bounds
    const double * getRowLower() const;

    /// Get pointer to array[getNumRows()] of row upper bounds
    const double * getRowUpper() const;

    /// Get pointer to array[getNumCols()] of objective function coefficients
    const double * getObjCoefficients() const;

    /// Get pointer to row-wise copy of matrix
    const CoinPackedMatrix * getMatrixByRow() const;

    /// Get pointer to column-wise copy of matrix
    const CoinPackedMatrix * getMatrixByCol() const;
  
   /// Set the data
   void setMpsData(const CoinPackedMatrix& m, const double infinity,
		   const double* collb, const double* colub,
		   const double* obj, const char* integrality,
		   const double* rowlb, const double* rowub,
		   char const * const * const colnames,
		   char const * const * const rownames);
   void setMpsData(const CoinPackedMatrix& m, const double infinity,
		   const double* collb, const double* colub,
		   const double* obj, const char* integrality,
		   const char* rowsen, const double* rowrhs,
		   const double* rowrng,
		   char const * const * const colnames,
		   char const * const * const rownames);

    /// Sets infinity!
    void setInfinity(double value);
    /// Gets infinity
    double getInfinity() const;
    /// Return true if column is continuous
    bool isContinuous(int colNumber) const;

    /** Return true if column is integer.
        Note: This function returns true if the the column
        is binary or a general integer.
    */
    bool isInteger(int columnNumber) const;


  /**@name Methods to input/output a problem (returns number of errors) 
    -1 if file not opened */
  //@{
    /** Read an mps file from the given filename.
	Note: if the MpsIO class was compiled with support for libz and/or
	libbz then reapMps will try to append .gz (.bz2 (TODO)) to the
	filename and open it as a compressed file if the original file cannot
	be opened. */
    int readMps(const char *filename, const char *extension = "mps");
    /// Read an mps file from previously given filename (or stdin)
    int readMps();
    /** write out the problem into a file with the given filename.
	- \c compression can be set to three values to indicate what kind
	  of file should be written
	  - 0: plain text (default)
	  - 1: gzip compressed (.gz is appended to \c filename) (TODO)
	  - 2: bzip2 compressed (.bz2 is appended to \c filename) (TODO)
	  .
	  If the library was not compiled with the requested compression then
	  writeMps falls back to writing a plain text file.
	- \c formatType specifies the precision to be written into the MPS
	file
	  - 0: normal precision (default)
	  - 1: extra accuracy
	  - 2: IEEE hex (to be implemented)
	  .
	- \c numberAcross specifies whether 1 or 2 values should be specied on
	every data line in the MPS file (default is 2)
	.
    */
    int writeMps(const char *filename, int compression = 0,
		 int formatType = 0, int numberAcross = 2) const;
    /// Set file name
    void setFileName(const char * name);
    /// Get file name
    const char * getFileName() const;
    /// Test if current file exists and readable
    const bool fileReadable() const;
    // Could later allow for file pointers and positioning
    /// Sets default upper bound for integer variables
    void setDefaultBound(int value);
    /// gets default upper bound for integer variables
    int getDefaultBound() const;
  //@}

  /**@name Methods to output a problem (returns number of errors) 
    -1 if file not opened */
  //@{

  /**@name Constructors and destructors */
  //@{
    /// Default Constructor
    CoinMpsIO(); 
      
    /// Copy constructor 
    CoinMpsIO (const CoinMpsIO &);
  
    /// Assignment operator 
    CoinMpsIO & operator=(const CoinMpsIO& rhs);
  
    /// Destructor 
    ~CoinMpsIO ();
  //@}

  //@{
    /// array saying if each variable integer
    /// at present just zero/non-zero may be extended
    const char * integerColumns() const;
    /// names - returns NULL if out of range
    const char * rowName(int index) const;
    const char * columnName(int index) const;
  /** names - returns -1 if name not found
      at present rowIndex will return numberRows for objective
      and > numberRows for dropped free rows */
    int rowIndex(const char * name) const;
    int columnIndex(const char * name) const;
    /** objective offset - this is RHS entry for objective row */
    double objectiveOffset() const;
  //@}

  /**@name Message handling */
  //@{
  /// Pass in Message handler (not deleted at end)
  void passInMessageHandler(CoinMessageHandler * handler);
  /// Set language
  void newLanguage(CoinMessages::Language language);
  void setLanguage(CoinMessages::Language language)
  {newLanguage(language);};
  /// Return handler
  CoinMessageHandler * messageHandler() const
  {return handler_;};
  /// Return messages
  CoinMessages messages() 
  {return messages_;};
  //@}

  /**@name Name type methods */
  //@{
    /// Problem name
    const char * getProblemName() const;
    /// Objective name
    const char * getObjectiveName() const;
    /// Rhs name
    const char * getRhsName() const;
    /// Range name
    const char * getRangeName() const;
    /// Bound name
    const char * getBoundName() const;
  //@}


  /**@name release storage */
  //@{
    /** Release all information which can be re-calculated e.g. rowsense
	also any row copies OR hash tables for names */
    void releaseRedundantInformation();
    /// Release all row information (lower, upper)
    void releaseRowInformation();
    /// Release all column information (lower, upper, objective)
    void releaseColumnInformation();
    /// Release integer information
    void releaseIntegerInformation();
    /// Release row names
    void releaseRowNames();
    /// Release column names
    void releaseColumnNames();
    /// Release matrix information
    void releaseMatrixInformation();
  
  //@}

private:
  
  
  /**@name Private methods */
  //@{
  
    /// The real work of a copy constructor (used by copy and assignment)
    void gutsOfDestructor();
    void gutsOfCopy(const CoinMpsIO &);
  
    /// The real work of a destructor (used by copy and assignment)
    void freeAll();

    /** A quick inlined function to convert from lb/ub stryle constraint
	definition to sense/rhs/range style */
    inline void
    convertBoundToSense(const double lower, const double upper,
			char& sense, double& right, double& range) const;
    /** A quick inlined function to convert from sense/rhs/range stryle
	constraint definition to lb/ub style */
    inline void
    convertSenseToBound(const char sense, const double right,
			const double range,
			double& lower, double& upper) const;
  //@}

  
  // for hashing
  typedef struct {
    int index, next;
  } CoinHashLink;
  /**@name hash stuff */
  //@{
  /// Creates hash list for names (0 rows, 1 columns)
  void startHash ( char **names, const int number , int section );
  /// This one does it when names are already in
  void startHash ( int section ) const;
  /// Deletes hash storage
  void stopHash ( int section );
  /// Finds match using hash,  -1 not found
  int findHash ( const char *name , int section ) const;
  //@}

  /**@name Protected member data */
  //@{
    /**@name Cached information */
    //@{
      /// Pointer to dense vector of row sense indicators
      mutable char    *rowsense_;
  
      /// Pointer to dense vector of row right-hand side values
      mutable double  *rhs_;
  
     /** Pointer to dense vector of slack upper bounds for range 
         constraints (undefined for non-range rows)
     */
     mutable double  *rowrange_;
  
     /// Pointer to row-wise copy of problem matrix coefficients.
     mutable CoinPackedMatrix *matrixByRow_;  
     /// Pointer to column-wise copy of problem matrix coefficients.
     CoinPackedMatrix *matrixByColumn_;  
     double * rowlower_;
     double * rowupper_;
     double * collower_;
     double * colupper_;
     double * objective_;
     char * integerType_;
     char * fileName_;
     /// Number in hash table
     int numberHash_[2];
     /// Hash tables
     mutable CoinHashLink *hash_[2];
     /// Names linked to hash (0 - row names, 1 column names)
     char **names_[2];
     int numberRows_;
     int numberColumns_;
     CoinBigIndex numberElements_;
     /// Upper bound when no bounds for integers
     int defaultBound_; 
     double infinity_;
     /// offset for objective function (i.e. rhs of OBJ row)
     double objectiveOffset_;
     /// information on problem
     char * problemName_;
     char * objectiveName_;
     char * rhsName_;
     char * rangeName_;
     char * boundName_;
  /// Message handler
  CoinMessageHandler * handler_;
  /// Flag to say if default handler (so delete)
  bool defaultHandler_;
  /// Messages
  CoinMessages messages_;
    //@}
  //@}

};

//#############################################################################
/** A function that tests the methods in the CoinMpsIO class. The
    only reason for it not to be a member method is that this way it doesn't
    have to be compiled into the library. And that's a gain, because the
    library should be compiled with optimization on, but this method should be
    compiled with debugging. Also, if this method is compiled with
    optimization, the compilation takes 10-15 minutes and the machine pages
    (has 256M core memory!)... */
void
CoinMpsIOUnitTest(const std::string & mpsDir);

#endif
