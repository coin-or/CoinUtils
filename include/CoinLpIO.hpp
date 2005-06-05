// Last edit: 3/7/05
//
// Name:     CoinLpIO.hpp; Support for Lp files
// Author:   Francois Margot
//           Tepper School of Business
//           Carnegie Mellon University, Pittsburgh, PA 15213
//           email: fmargot@andrew.cmu.edu
// Date:     12/28/03
//-----------------------------------------------------------------------------
// Copyright (C) 2003, Francois Margot, International Business Machines
// Corporation and others.  All Rights Reserved.


class CoinPackedMatrix;

typedef int COINColumnIndex;

  /** Class to read and write Lp files 

 Lp file format: 

 Min<BR>
  obj: x0 + x1 + 3 x2 - 4.5 xyr <BR>
 s.t. <BR>
 cons1: x0 - x2 - 2.3 x4 <= 4.2 <BR>
 c2: x1 + x2 >= 1 <BR>
 cc: x1 + x2 + xyr = 2 <BR>
 Bounds <BR>
 0 <= x1 <= 3 <BR>
 1 >= x2 <BR>
 x3 = 1 <BR>
 xyr free <BR>
 Integers <BR>
 x0 <BR>
 Generals <BR>
 x1 xyr <BR>
 Binaries <BR>
 x2 <BR>
 End

Notes: <UL>
 <LI> Bounds, Integers, Generals, Binaries sections are optional.
 <LI> Generals and Integers are synonymous.
 <LI> Bounds section (if any) must come before Integers, Generals, and 
      Binaries sections.
 <LI> Constraint names must be followed by ':' without blank space.
      (Constraint names are optional.)
 <LI> Variable names can not start with a number or '<', '>', '='; 
      they must be followed by a blank space. 
 <LI> Keywords may have their first letter in caps or lower case and
      be in plural or singular form.
 <LI> Max, Maximize, Minimize are also allowed for the objective sense.
 <LI> "S.T." or "ST.", "ST", "Subject To" or "subject to" are also 
      allowed. 
 <LI> Default bounds are 0 for lower bound and +infinity for upper bound.
 <LI> Free variables get lower bound -infinity and upper bound +infinity.
 <LI> If a variable appears in the Bounds section more than once,
      the last bound is the one taken into account. The bounds for a
      binary variable are set to 0/1 only if this bound is stronger than 
      the bound obtained from the Bounds section. 
</UL>
*/
class CoinLpIO {

public:

  /**@name Constructor and Destructor */
  //@{
  /// Default Constructor
  CoinLpIO(); 
  
  /// Destructor 
  ~CoinLpIO();

  /// Free all memory (except hash tables)
  void freeAll();
  //@}

    /** A quick inlined function to convert from lb/ub style constraint
	definition to sense/rhs/range style */
    inline void
    convertBoundToSense(const double lower, const double upper,
			char& sense, double& right, double& range) const;

  /**@name Queries */
  //@{

  /// Get the problem name
  const char * getProblemName() const;

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

  /// Get pointer to array[getNumRows()] of row lower bounds
  const double * getRowLower() const;
  
  /// Get pointer to array[getNumRows()] of row upper bounds
  const double * getRowUpper() const;
      /** Get pointer to array[getNumRows()] of constraint senses.
	<ul>
	<li>'L': <= constraint
	<li>'E': =  constraint
	<li>'G': >= constraint
	<li>'R': ranged constraint
	<li>'N': free constraint
	</ul>
      */
  const char * getRowSense() const;
  
  /** Get pointer to array[getNumRows()] of constraint right-hand sides.
      
  Given constraints with upper (rowupper) and/or lower (rowlower) bounds,
  the constraint right-hand side (rhs) is set as
  <ul>
  <li> if rowsense()[i] == 'L' then rhs()[i] == rowupper()[i]
  <li> if rowsense()[i] == 'G' then rhs()[i] == rowlower()[i]
  <li> if rowsense()[i] == 'R' then rhs()[i] == rowupper()[i]
  <li> if rowsense()[i] == 'N' then rhs()[i] == 0.0
	</ul>
  */
  const double * getRightHandSide() const;
  
  /** Get pointer to array[getNumRows()] of row ranges.
      
  Given constraints with upper (rowupper) and/or lower (rowlower) bounds, 
  the constraint range (rowrange) is set as
  <ul>
  <li> if rowsense()[i] == 'R' then
  rowrange()[i] == rowupper()[i] - rowlower()[i]
  <li> if rowsense()[i] != 'R' then
  rowrange()[i] is 0.0
  </ul>
  Put another way, only range constraints have a nontrivial value for
  rowrange.
  */
  const double * getRowRange() const;

  /// Get pointer to array[getNumCols()] of objective function coefficients
  const double * getObjCoefficients() const;
  
  /// Get pointer to row-wise copy of the coefficient matrix
  const CoinPackedMatrix * getMatrixByRow() const;

  /// Get pointer to column-wise copy of the coefficient matrix
  const CoinPackedMatrix * getMatrixByCol() const;

  /// Get objective function name
  const char * getObjName() const;
  
  /// Get pointer to array[getNumRows()] of row names
  char ** const getRowNames() const;
  
  /// Get pointer to array[getNumCols()] of column names
  char ** const getColNames() const;
  
  /// Return the row name for the specified index.
  /// Return 0 if the index is out of range.
  const char * rowName(int index) const;

  /// Return the column name for the specified index.
  /// Return 0 if the index is out of range.
  const char * columnName(int index) const;

  /// Return the index for the specified row name.
  /// Return -1 if the name is not found.
  int rowIndex(const char * name) const;

  /// Return the index for the specified column name.
  /// Return -1 if the name is not found.
  int columnIndex(const char * name) const;

    /** Returns the (constant) objective offset
    
	This is the RHS entry for the objective row
    */
    double objectiveOffset() const;

    /// Set objective offset
    inline void setObjectiveOffset(double value)
    { objectiveOffset_ = value;};

    /** Return true if a column is an integer variable

        Note: This function returns true if the the column
        is a binary or general integer variable.
    */
    bool isInteger(int columnNumber) const;
  
  /// Get characteristic vector of integer variables
  const char * integerColumns() const;
  //@}
  
  /**@name Parameters */
  //@{
  /// Get infinity
  double getInfinity() const;

  /// Set infinity. Any number larger is considered infinity.
  /// Default: DBL_MAX
  void setInfinity(const double);

  /// Get epsilon
  double getEpsilon() const;

  /// Set epsilon.
  /// Default: 1e-5.
  void setEpsilon(const double);

  /// Get numberAcross, the number of monomials to be printed per line
  int getNumberAcross() const;

  /// Set numberAcross.
  /// Default: 10.
  void setNumberAcross(const int);

  /// Get decimals, the number of digits to write after the decimal point
  int getDecimals() const;

  /// Set decimals.
  /// Default: 5
  void setDecimals(const int);
  //@}

  /**@name Public methods */
  //@{
  /// Set the data of the object.
  /// Set it from the coefficient matrix m, the lower bounds
  ///  collb,  the upper bounds colub, objective function obj_coeff, 
  /// integrality vector integrality, lower/upper bounds on the constraints.
  void setLpDataWithoutRowAndColNames(
			      const CoinPackedMatrix& m,
			      const double* collb, const double* colub,
			      const double* obj_coeff, 
			      const char* integrality,
			      const double* rowlb, const double* rowub);

  /// Set the row and column names
  void setLpDataRowAndColNames(char const * const * const rownames,
			       char const * const * const colnames);

  /// Write the data in Lp format in the file with name filename.
  /// Coefficients with value less than epsilon away from an integer value
  /// are written as integers..
  /// Write at most numberAcross coefficients on a line.
  /// Write non integer numbers with decimals digits after the decimal point.
  int writeLp(const char *filename, 
	      double epsilon, 
	      int numberAcross,
	      int decimals);

  /// Write the data in Lp format in the file with name filename.
  int writeLp(const char *filename) const;

  /// Read the data in Lp format from the file with name filename, using
  /// the given value for epsilon.
  void readLp(const char *filename, const double epsilon);

  /// Read the data in Lp format from the file with name filename.
  void readLp(const char *filename);

  /// Dump the data. Low level method for debugging.
  void print() const;
  //@}

protected:
  /// Problem name
  char * problemName_;

  /// Number of rows
  int numberRows_;
  
  /// Number of columns
  int numberColumns_;
  
  /// Number of elements
  int numberElements_;
  
  /// Pointer to column-wise copy of problem matrix coefficients.
  mutable CoinPackedMatrix *matrixByColumn_;  
  
  /// Pointer to row-wise copy of problem matrix coefficients.
  CoinPackedMatrix *matrixByRow_;  
  
  /// Pointer to dense vector of row lower bounds
  double * rowlower_;
  
  /// Pointer to dense vector of row upper bounds
  double * rowupper_;
  
  /// Pointer to dense vector of column lower bounds
  double * collower_;
  
  /// Pointer to dense vector of column upper bounds
  double * colupper_;
  
  /// Pointer to dense vector of row rhs
  mutable double * rhs_;
  
  /** Pointer to dense vector of slack variable upper bounds for ranged 
      constraints (undefined for non-ranged constraints)
  */
  mutable double  *rowrange_;

  /// Pointer to dense vector of row senses
  mutable char * rowsense_;
  
  /// Pointer to dense vector of objective coefficients
  double * objective_;
  
  /// Constant offset for objective value
  double objectiveOffset_;
  
  /// Pointer to dense vector specifying if a variable is continuous
  /// (0) or integer (1).
  char * integerType_;
  
  /// Current file name
  char * fileName_;
  
  /// Value to use for infinity
  double infinity_;

  /// Value to use for epsilon
  double epsilon_;

  /// Number of monomials printed in a row
  int numberAcross_;

  /// Number of decimals printed for coefficients
  int decimals_;

  /// Objective function name
  char *objName_;

  /// Row names and column names (linked to Hash tables)
  char **names_[2];

  typedef struct {
    int index, next;
  } CoinHashLink;

  /// Maximum number of entries in a hash table section
  int maxHash_[2];

  /// Number of entries in a hash table section
  int numberHash_[2];

  /// Hash tables (two sections, 0 - row names, 1 - column names)
  mutable CoinHashLink *hash_[2];

  /// Build the hash table for the given names. The parameter number is
  /// the cardinality of names; section = 0 for row names, section = 1 for
  /// column names. Removes duplicate names. 
  void startHash(char const * const * const names, 
		 const COINColumnIndex number, 
		 int section);

  /// Delete hash storage
  void stopHash(int section);

  /// Return the index of the given name.
  /// section = 0 for row names, 
  /// section = 1 for column names. Return -1 if the name is not found.
  COINColumnIndex findHash(const char *name, int section) const;

  /// Insert thisName in the hash table if not present yet. Does nothing
  /// if the name is already in.
  void insertHash(const char *thisName, int section);

  /// Write a coefficient.
  /// print_1 = 0 : do not print the value 1.
  void out_coeff(FILE *fp, double v, int print_1) const;

  /// Locate the objective function. 
  /// Return 1 if successful, -1 otherwise.
  int find_obj(FILE *fp) const;

  /// Return an integer indicating if the keyword "subject to" or one
  /// of its variants has been read.
  /// Return 1 if buff is the keyword "s.t" or one of its variants.
  /// Return 2 if buff is the keyword "subject" or one of its variants.
  /// Return 0 otherwise.
  int is_subject_to(char *buff) const;

  /// Return 1 if the first character of buff is a number.
  /// Return 0 otherwise.
  int first_is_number(char *buff) const;

  /// Return 1 if buff is the keyword "free" or one of its variants.
  /// Return 0 otherwise.
  int is_free(char *buff) const;
  
  /// Return an integer indicating the inequality sense read.
  /// Return 0 if buff is '<='.
  /// Return 1 if buff is '='.
  /// Return 2 if buff is '>='.
  /// Return -1 otherwise.
  int is_sense(char *buff) const;

  /// Return an integer indicating if one of the keywords "Bounds", "Integers",
  /// "Generals", "Binaries", "End", or one
  /// of their variants has been read.
  /// Return 1 if buff is the keyword "Bounds" or one of its variants..
  /// Return 2 if buff is the keyword "Integers" or "Generals" or one of their 
  /// variants.
  /// Return 3 if buff is the keyword "Binaries" or one of its variants.
  /// Return 4 if buff is the keyword "End" or one of its variants.
  /// Return 0 otherwise.
  int is_keyword(char *buff) const;

  /// Read a monomial of the objective function..
  /// Return 1 if "subject to" or one of its variants has been read.
  int read_monom_obj(FILE *fp, double *coeff, char **name, int *cnt, 
		     char **obj_name) const;

  /// Read a monomial of a constraint.
  /// Return a positive number if the sense of the inequality has been 
  /// read (see method is_sense() for the return code).
  /// Return -1 otherwise.
  int read_monom_row(FILE *fp, char *start_str, double *coeff, char **name, 
		     int cnt_coeff) const;

  /// Reallocate vectors related to number of coefficients.
  void realloc_coeff(double **coeff, char ***colNames, int *maxcoeff) const;

  /// Reallocate vectors related to rows.
  void realloc_row(char ***rowNames, int **start, double **rhs, 
		   double **rowlow, double **rowup, int *maxrow) const;
    
  /// Reallocate vectors related to columns.
  void realloc_col(double **collow, double **colup, char **is_int,
		   int *maxcol) const;

  /// Read a constraint.
  void read_row(FILE *fp, char *buff, double **pcoeff, char ***pcolNames, 
		int *cnt_coeff, int *maxcoeff,
		     double *rhs, double *rowlow, double *rowup, 
		     int *cnt_row, double inf) const;

};

