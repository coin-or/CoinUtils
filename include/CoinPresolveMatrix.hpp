// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
#ifndef CoinPresolveMatrix_H
#define CoinPresolveMatrix_H

#include "CoinPragma.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinMessage.hpp"

#include <cmath>
#include <cassert>
#include <cfloat>
#include <cassert>

/*
  Define these for debugging information and consistency checks.
  Both must be given a value --- they're used in both the #ifdef and
  #if forms, although the value is never used.

  #define	DEBUG_PRESOLVE 1
  #define CHECK_CONSISTENCY 1
*/


#if defined(_MSC_VER)
// Avoid MS Compiler problem in recognizing type to delete
// by casting to type.
#define deleteAction(array,type) delete [] ((type) array)
#else
#define deleteAction(array,type) delete [] array
#endif

/*! \todo Figure out a way to remove the SolverInterface class declarations
	  in CoinPresolveMatrix.hpp.

  The need to include these class declarations here strikes me (lh) as
  violating the modularity of COIN.

  The problem is in the constructors --- it's convenient to take a SI* as one
  of the parameters, and that means we need a declaration for the SI class.
  (See the declarations for the CoinPrePostsolveMatrix and CoinPresolveMatrix
  constructors for examples.

  Virtual constructors, perhaps? Or a generic constructor that creates an
  empty CoinPresolveMatrix, and a pure virtual load function that would be
  overridden in derived classes?
*/
  class ClpSimplex;
  class OsiSolverInterface;

/*! \brief Zero tolerance

  OSL had a fixed zero tolerance; we still use that here.
*/
const double ZTOLDP      = 1e-12;

double *presolve_duparray(const double *d, int n, int n2);
double *presolve_duparray(const double *d, int n);
int *presolve_duparray(const int *d, int n, int n2);
int *presolve_duparray(const int *d, int n);
char *presolve_duparray(const char *d, int n, int n2);
char *presolve_duparray(const char *d, int n);
// This one saves in one go to save [] memory
double * presolve_duparray(const double * element, const int * index,
			   int length, int offset);

void presolve_delete_from_row(int row, int col /* thing to delete */,
		     const CoinBigIndex *mrstrt,
		     int *hinrow, int *hcol, double *dels);

/*! \relates CoinPrePostsolveMatrix
    \brief Find position of entry in packed or loosely packed matrix

    The routine locates the position \p k in \p hrow for the specified \p row.
    It will abort if the entry does not exist.

    To use as presolve_find_col: presolve_find_row(col,krs,kre,hcol).
*/
int presolve_find_row(int row, CoinBigIndex kcs, CoinBigIndex kce, const int *hrow);
/*! \relates CoinPrePostsolveMatrix
    \brief Find position of entry in packed or loosely packed matrix

    The routine locates the position k in hrow for the specified row. A return
    value of \p kce means the entry does not exist.

    To use as presolve_find_col1: presolve_find_row1(col,krs,kre,hcol).
*/
int presolve_find_row1(int row, CoinBigIndex kcs, CoinBigIndex kce, const int *hrow);


#ifdef	DEBUG_PRESOLVE
inline void	DIE(const char *s)	{ std::cout<<s; abort(); }
#else
  inline void	DIE(const char *s)	{}
#endif

#if	DEBUG_PRESOLVE
#define	PRESOLVE_STMT(s)	s
#define PRESOLVEASSERT(x)	((x) ? 1 : ((std::cerr<< "FAILED ASSERTION at line "<< __LINE__ << ":  " #x "\n"), abort(), 0))
#else
#define PRESOLVEASSERT(x) assert(x)
#define	PRESOLVE_STMT(s)
#endif

struct dropped_zero {
  int row;
  int col;
};

inline int ALIGN(int n, int m)	{ return (((n + m - 1) / m) * m); }
inline int ALIGN_DOUBLE(int n)	{ return ALIGN(n,sizeof(double)); }

// Plus infinity
#ifndef COIN_DBL_MAX
#define COIN_DBL_MAX DBL_MAX
#endif
#define PRESOLVE_INF COIN_DBL_MAX

class CoinPostsolveMatrix;

// Note 77
// "Members and bases are constructed in ordre of declation
//  in the class and destroyed in the reverse order."  C++PL 3d Ed. p. 307
//
// That's why I put integer members (such as ncols) before the array members;
// I like to use those integer values during initialization.
// NOT ANYMORE

/*! \class CoinPresolveAction
    \brief Abstract base class of all presolve routines.

  The details will make more sense after a quick overview of the grand plan:
  A presolve object is handed a problem object, which it is expected to
  modify in some useful way.  Assuming that it succeeds, the presolve object
  should create a postsolve object, <i>i.e.</i>, an object that contains
  instructions for backing out the presolve transform to recover the original
  problem. These postsolve objects are accumlated in a linked list, with each
  successive presolve action adding its postsolve action to the head of the
  list. The end result of all this is a presolved problem object, and a list
  of postsolve objects. The presolved problem object is then handed to a
  solver for optimization, and the problem object augmented with the
  results.  The list of postsolve objects is then traversed. Each of them
  (un)modifies the problem object, with the end result being the original
  problem, augmented with solution information.

  The problem object representation is CoinPrePostsolveMatrix and subclasses.
  Check there for details. The \c CoinPresolveAction class and subclasses
  represent the presolve and postsolve objects.

  In spite of the name, the only information held in a \c CoinPresolveAction
  object is the information needed to postsolve (<i>i.e.</i>, the information
  needed to back out the presolve transformation). This information is not
  expected to change, so the fields are all \c const.

  A subclass of \c CoinPresolveAction, implementing a specific pre/postsolve
  action, is expected to declare a static function that attempts to perform a
  presolve transformation. This function will be handed a CoinPresolveMatrix
  to transform, and a pointer to the head of the list of postsolve objects.
  If the transform is successful, the function will create a new
  \c CoinPresolveAction object, link it at the head of the list of postsolve
  objects, and return a pointer to the postsolve object it has just created.
  Otherwise, it should return 0. It is expected that these static functions
  will be the only things that can create new \c CoinPresolveAction objects;
  this is expressed by making each subclass' constructor(s) private.

  Every subclass must also define a \c postsolve method.
  This function will be handed a CoinPostsolveMatrix to transform.

  It is the client's responsibility to implement presolve and postsolve driver
  routines. See OsiPresolve for examples.

  \note Since the only fields in a \c CoinPresolveAction are \c const, anything
	one can do with a variable declared \c CoinPresolveAction* can also be
	done with a variable declared \c const \c CoinPresolveAction* It is
	expected that all derived subclasses of \c CoinPresolveAction also have
	this property.
*/
class CoinPresolveAction {
 public:
  /*! \brief Stub routine to throw exceptions.
  
   Exceptions are inefficient, particularly with g++.  Even with xlC, the
   use of exceptions adds a long prologue to a routine.  Therefore, rather
   than use throw directly in the routine, I use it in a stub routine.
  */
  static void throwCoinError(const char *error, const char *ps_routine);

  /*! \brief The next presolve transformation
  
    Set at object construction.
  */
  const CoinPresolveAction *next;
  
  /*! \brief Construct a postsolve object and add it to the transformation list.
  
    This is an `add to head' operation. This object will point to the
    one passed as the parameter.
  */
  CoinPresolveAction(const CoinPresolveAction *next) : next(next) {}

  /*! \brief A name for debug printing.

    It is expected that the name is not stored in the transform itself.
  */
  virtual const char *name() const = 0;

  /*! \brief Apply the postsolve transformation for this particular
	     presolve action.
  */
  virtual void postsolve(CoinPostsolveMatrix *prob) const = 0;

  /*! \brief Virtual destructor. */
  virtual ~CoinPresolveAction() {}
};

/*! \class CoinPrePostsolveMatrix
    \brief Collects all the information about the problem that is needed
	   in both presolve and postsolve.
    
    In a bit more detail, a column-major representation of the constraint
    matrix, plus row and column solutions and status. There's also a set of
    arrays holding the original row and column numbers.

    The matrix representation is the common column-major format: a pair of
    arrays, with positional correspondence, to hold coefficients and row
    indices. A second pair of arrays with positional correspondence specify
    the start of each column (in the first pair of arrays) and the length of
    each column.
*/
class CoinPrePostsolveMatrix {
 public:
  CoinPrePostsolveMatrix(const ClpSimplex * si,
			int ncols_,
			int nrows_,
			CoinBigIndex nelems_);
  CoinPrePostsolveMatrix(const OsiSolverInterface * si,
			int ncols_,
			int nrows_,
			CoinBigIndex nelems_);

  ~CoinPrePostsolveMatrix();

  /*! \brief Enum for status of various sorts
  
    Matches CoinWarmStartBasis and adds superBasic
  */
  enum Status {
    isFree = 0x00,
    basic = 0x01,
    atUpperBound = 0x02,
    atLowerBound = 0x03,
    superBasic = 0x04
  };
  /// Vector of primal variable values
  double *sol_;
  /// Vector of dual variable values
  double *rowduals_;
  /*! \brief Vector of constraint left-hand-side values
  
    Produced by evaluating constraints according to #sol_
  */
  double *acts_;

  double *rcosts_;
  /// Status of primal variables
  unsigned char *colstat_;
  /// Status of constraints
  unsigned char *rowstat_;


  /// Original objective offset
  double originalOffset_;
  /// Message handler
  CoinMessageHandler *  handler_; 
  /// Messages
  CoinMessages messages_; 

  inline CoinMessageHandler * messageHandler() const 
  { return handler_; }
  /// Return messages
  inline CoinMessages messages() const 
  { return messages_; }
  /*! \name Column-major representation
  
    Common column-major format: A pair of vectors with positional
    correspondence to hold coefficients and row indices, and a second pair
    of vectors giving the starting position and length of each column in
    the first pair.
  */
  //@{
  /// current number of columns
  int ncols_;
  /// original number of columns
  const int ncols0_;
  /// number of coefficients
  CoinBigIndex nelems_;
  /// Vector of column start positions in #hrow_, #colels_
  CoinBigIndex *mcstrt_;
  /// Vector of column lengths
  int *hincol_;
  /// Row indices (positional correspondence with #colels_)
  int *hrow_;
  /// Coefficients (positional correspondence with #hrow_)
  double *colels_;
  //@}

  /// Objective coefficients
  double *cost_;
  /// Column (primal variable) lower bounds
  double *clo_;
  /// Column (primal variable) upper bounds
  double *cup_;
  /// Row (constraint) lower bounds
  double *rlo_;
  /// Row (constraint) upper bounds
  double *rup_;

  /// Original column numbers
  int * originalColumn_;
  /// Original row numbers
  int * originalRow_;

  /// Primal feasibility tolerance
  const double ztolzb_;
  /// Dual feasibility tolerance
  const double ztoldj_;

  /// Maximization/minimization
  double maxmin_;

  /*! \name Status stuff

    Functions to work with the CoinPrePostsolveMatrix::Status enum and
    related vectors.
  */
  //@{
  
  /// Set row status (<i>i.e.</i>, status of artificial for this row)
  inline void setRowStatus(int sequence, Status status)
  {
    unsigned char & st_byte = rowstat_[sequence];
    st_byte &= ~7;
    st_byte |= status;
  };
  /// Get row status
  inline Status getRowStatus(int sequence) const
  {return static_cast<Status> (rowstat_[sequence]&7);};
  /// Check if artificial for this row is basic
  inline bool rowIsBasic(int sequence) const
  {return (static_cast<Status> (rowstat_[sequence]&7)==basic);};
  /// Set column status (<i>i.e.</i>, status of primal variable)
  inline void setColumnStatus(int sequence, Status status)
  {
    unsigned char & st_byte = colstat_[sequence];
    st_byte &= ~7;
    st_byte |= status;

#   ifdef DEBUG_PRESOLVE
    switch (status)
    { case isFree:
      { if (clo_[sequence] > -PRESOLVE_INF || cup_[sequence] < PRESOLVE_INF)
	{ std::cout << "Bad status: Var " << sequence
		    << " isFree, lb = " << clo_[sequence]
		    << ", ub = " << cup_[sequence] << std::endl ; }
	break ; }
      case basic:
      { break ; }
      case atUpperBound:
      { if (cup_[sequence] >= PRESOLVE_INF)
	{ std::cout << "Bad status: Var " << sequence
	            << " atUpperBound, lb = " << clo_[sequence]
	            << ", ub = " << cup_[sequence] << std::endl ; }
	break ; }
      case atLowerBound:
      { if (clo_[sequence] <= -PRESOLVE_INF)
	{ std::cout << "Bad status: Var " << sequence
	            << " atLowerBound, lb = " << clo_[sequence]
	            << ", ub = " << cup_[sequence] << std::endl ; }
	break ; }
      case superBasic:
      { if (clo_[sequence] <= -PRESOLVE_INF && cup_[sequence] >= PRESOLVE_INF)
	{ std::cout << "Bad status: Var " << sequence
	            << " superBasic, lb = " << clo_[sequence]
	            << ", ub = " << cup_[sequence] << std::endl ; }
	break ; }
      default:
      { assert(false) ;
	break ; } }
#   endif
  };
  /// Get column status
  inline Status getColumnStatus(int sequence) const
  {return static_cast<Status> (colstat_[sequence]&7);};
  /// Check if primal variable is basic
  inline bool columnIsBasic(int sequence) const
  {return (static_cast<Status> (colstat_[sequence]&7)==basic);};
  /*! \brief Sets status of artificial to the correct nonbasic status given
	     bounds and current value
  */
  void setRowStatusUsingValue(int iRow);
  /*! \brief Sets status of primal variable to the correct nonbasic status
	     given bounds and current value
  */
  void setColumnStatusUsingValue(int iColumn);
  //@}

};


/*
 * Currently, the matrix is represented the same way an CoinPackedMatrix is.
 * Occasionally columns increase in size.
 * In order to check whether there is enough room for the column
 * where it sits, I wanted to know what the next column (in memory order)
 * in the representation was.
 * To do that, I maintain a linked list of columns; the "pre" and "suc"
 * fields give the previous and next columns, in memory order (that is,
 * the column whose mcstrt entry is next smaller or larger).
 * The same thing for the row representation.
 *
 * This is all likely to change, but I'm leaving it as it is for now.
 */
//  static const int	NO_LINK	= -66666666;
#define NO_LINK -66666666

class presolvehlink {
public:
  int pre, suc;
};
  
static inline void PRESOLVE_REMOVE_LINK(presolvehlink *link, int i)
{ 
  int ipre = link[i].pre;
  int isuc = link[i].suc;
  if (ipre >= 0) {
    link[ipre].suc = isuc;
  }
  if (isuc >= 0) {
    link[isuc].pre = ipre;
  }
  link[i].pre = NO_LINK, link[i].suc = NO_LINK;
}

// inserts i after pos
static inline void PRESOLVE_INSERT_LINK(presolvehlink *link, int i, int pos)
{
  int isuc = link[pos].suc;
  link[pos].suc = i;
  link[i].pre = pos;
  if (isuc >= 0) {
    link[isuc].pre = i;
  }
  link[i].suc = isuc;
}

// rename i to j
// that is, position j should be unused, and i will take its place
// should be equivalent to:
//   int pre = link[i].pre;
//   PRESOLVE_REMOVE_LINK(link, i);
//   PRESOLVE_INSERT_LINK(link, j, pre);
// if pre is not NO_LINK (otherwise -- ??)
static inline void PRESOLVE_MOVE_LINK(presolvehlink *link, int i, int j)
{ 
  int ipre = link[i].pre;
  int isuc = link[i].suc;
  if (ipre >= 0) {
    link[ipre].suc = j;
  }
  if (isuc >= 0) {
    link[isuc].pre = j;
  }
  link[i].pre = NO_LINK, link[i].suc = NO_LINK;
}



// this really should never happen.
// it will if there isn't enough space to postsolve the matrix.
// see the note below.
static inline void check_free_list(int free_list)
{
  assert (free_list>=0);
  //if (free_list < 0) {
  //printf("RAN OUT OF LINKS!!\n");
  //abort();
  //}
}


/*! \class CoinPresolveMatrix
   \brief Augments CoinPrePostsolveMatrix with information about the problem
	  that is only needed during presolve.

  This class adds a row-major matrix representation and linked lists that allow
  for easy manipulation of the matrix when applying presolve transforms.
*/

class CoinPresolveMatrix : public CoinPrePostsolveMatrix {
 public:

  CoinPresolveMatrix(int ncols0,
		    double maxmin,
		    // end prepost members

		    ClpSimplex * si,

		    // rowrep
		    int nrows,
		    CoinBigIndex nelems,
		 bool doStatus,
		 double nonLinearVariable);


  void update_model(ClpSimplex * si,
			    int nrows0,
			    int ncols0,
			    CoinBigIndex nelems0);
  CoinPresolveMatrix(int ncols0,
		    double maxmin,
		    // end prepost members

		    OsiSolverInterface * si,

		    // rowrep
		    int nrows,
		    CoinBigIndex nelems,
		 bool doStatus,
		 double nonLinearVariable);


  void update_model(OsiSolverInterface * si,
			    int nrows0,
			    int ncols0,
			    CoinBigIndex nelems0);

  ~CoinPresolveMatrix();
  // Crude linked lists, modelled after the linked lists used in OSL factorization.
  presolvehlink *clink_;
  presolvehlink *rlink_;

  double dobias_;

  /*! \name Row-major representation

    Common row-major format: A pair of vectors with positional
    correspondence to hold coefficients and column indices, and a second pair
    of vectors giving the starting position and length of each row in
    the first pair.
  */
  //@{
  /// current number of rows
  int nrows_;
  /// Vector of row start positions in #hcol, #rowels_
  CoinBigIndex *mrstrt_;
  /// Vector of row lengths
  int *hinrow_;
  /// Coefficients (positional correspondence with #hcol_)
  double *rowels_;
  /// Column indices (positional correspondence with #rowels_)
  int *hcol_;
  //@}

  /// tracks integrality of columns (1 for integer, 0 for continuous)
  char *integerType_;
  /// bounds can be moved by this to stay feasible
  double feasibilityTolerance_;
  /// Output status 0=feasible, 1 infeasible, 2 unbounded
  int status_;
  // Should use templates ?
  // Rows
  // Bits to say if row changed
  // Now char so can use to find duplicates
  /*! \name Row data */
  //@{
  /*! \brief Row status information

  Coded using the following bits:
    <ul>
      <li> 0x01: Row has changed
      <li> 0x02: preprocessing prohibited
      <li> 0x04: Row has been used
    </ul>
  */
  unsigned char * rowChanged_;
  /// Input list of rows to process
  int * rowsToDo_;
  /// Length of #rowsToDo_
  int numberRowsToDo_;
  /// Output list of rows to process
  int * nextRowsToDo_;
  /// Length of #nextRowsToDo_
  int numberNextRowsToDo_;
  //@}

  /// Flag to say if any rows or columns are marked as prohibited
  bool anyProhibited_;
  /// Check if there are any prohibited rows or columns 
  inline bool anyProhibited() const
  { return anyProhibited_;};


  /*! \name Row functions */
  //@{
  /// Has row been changed?
  inline bool rowChanged(int i) const {
    return (rowChanged_[i]&1)!=0;
  }
  /// Mark row as changed
  inline void setRowChanged(int i) {
    rowChanged_[i] |= 1;
  }
  /// Mark row as changed and add to list of rows to process next
  inline void addRow(int i) {
    if ((rowChanged_[i]&1)==0) {
      rowChanged_[i] |= 1;
      nextRowsToDo_[numberNextRowsToDo_++] = i;
    }
  }
  /// Mark row as not changed
  inline void unsetRowChanged(int i) {
    rowChanged_[i]  &= ~1;;
  }
  /// Test if row is eligible for preprocessing
  inline bool rowProhibited(int i) const {
    return (rowChanged_[i]&2)!=0;
  }
  /*! \brief Test if row is eligible for preprocessing

    The difference between this method and #rowProhibited() is that this
    method first tests #anyProhibited_ before examining the specific entry
    for the specified row.
  */
  inline bool rowProhibited2(int i) const {
    if (!anyProhibited_)
      return false;
    else
      return (rowChanged_[i]&2)!=0;
  }
  /// Mark row as ineligible for preprocessing
  inline void setRowProhibited(int i) {
    rowChanged_[i] |= 2;
  }
  // This is for doing faster lookups to see where two rows have entries
  // in common
  /// Test if row is marked as used
  inline bool rowUsed(int i) const {
    return (rowChanged_[i]&4)!=0;
  }
  /// Mark row as used
  inline void setRowUsed(int i) {
    rowChanged_[i] |= 4;
  }
  /// Mark row as unused
  inline void unsetRowUsed(int i) {
    rowChanged_[i]  &= ~4;;
  }
  //@}

  /*! \name Column functions */
  //@{
  /// Has column been changed?
  inline bool colChanged(int i) const {
    return (colChanged_[i]&1)!=0;
  }
  /// Mark column as changed.
  inline void setColChanged(int i) {
    colChanged_[i] |= 1;
  }
  /// Mark column as changed and add to list of columns to process next
  inline void addCol(int i) {
    if ((colChanged_[i]&1)==0) {
      colChanged_[i] |= 1;
      nextColsToDo_[numberNextColsToDo_++] = i;
    }
  }
  /// Mark column as not changed
  inline void unsetColChanged(int i) {
    colChanged_[i]  &= ~1;;
  }
  /// Test if column is eligible for preprocessing
  inline bool colProhibited(int i) const {
    return (colChanged_[i]&2)!=0;
  }
  /*! \brief Test if column is eligible for preprocessing

    The difference between this method and #colProhibited() is that this
    method first tests #anyProhibited_ before examining the specific entry
    for the specified column.
  */
  inline bool colProhibited2(int i) const {
    if (!anyProhibited_)
      return false;
    else
      return (colChanged_[i]&2)!=0;
  }
  /// Mark column as ineligible for preprocessing
  inline void setColProhibited(int i) {
    colChanged_[i] |= 2;
  }
  /*! \brief Test if column is marked as used
  
    This is for doing faster lookups to see where two columns have entries
    in common.
  */
  inline bool colUsed(int i) const {
    return (colChanged_[i]&4)!=0;
  }
  /// Mark column as used
  inline void setColUsed(int i) {
    colChanged_[i] |= 4;
  }
  /// Mark column as unused
  inline void unsetColUsed(int i) {
    colChanged_[i]  &= ~4;;
  }
  //@}

  void consistent(bool testvals = true);

  inline void change_bias(double change_amount);

  /*! \name Column data */
  //@{
  /*! \brief Column status information

  Coded using the following bits:
    <ul>
      <li> 0x01: column has changed
      <li> 0x02: preprocessing prohibited
      <li> 0x04: column has been used
    </ul>
  */
  unsigned char * colChanged_;

  /// Input list of columns to process
  int * colsToDo_;
  /// Length of #colsToDo_
  int numberColsToDo_;
  /// Output list of columns to process next
  int * nextColsToDo_;
  /// Length of #nextColsToDo_
  int numberNextColsToDo_;
  //@}

  // Pass number
  int pass_;

};

/*! \class CoinPostsolveMatrix
    \brief Augments CoinPrePostsolveMatrix with information about the problem
	   that is only needed during postsolve.

  The notable point is that the matrix representation is threaded. The
  representation is column-major and starts with the standard two pairs of
  arrays: one pair to hold the row indices and coefficients, the second pair
  to hold the column starting positions and lengths. But the row indices and
  coefficients for a column do not necessarily occupy a contiguous block in
  their respective arrays. Instead, a link array gives the position of the
  next (row index,coefficient) pair. If the row index and value of a
  coefficient a<p,j> occupy position kp in their arrays, then the position of
  the next coefficient a<q,j> is found as kq = link[kp].

  This threaded representation allows for efficient expansion of columns as
  rows are reintroduced during postsolve transformations. The basic packed
  structures are allocated to the expected size of the postsolved matrix,
  and as new coefficients are added, their location is simply added to the
  thread for the column.

  There is no provision to convert the threaded representation to a packed
  representation. In the context of postsolve, it's not required --- we already
  have a perfectly good packed copy of the original matrix.
*/
class CoinPostsolveMatrix : public CoinPrePostsolveMatrix {
 public:

  CoinPostsolveMatrix(ClpSimplex * si,

		   int ncols0,
		   int nrows0,
		   CoinBigIndex nelems0,
		     
		   double maxmin_,
		   // end prepost members

		   double *sol,
		   double *acts,

		   unsigned char *colstat,
		   unsigned char *rowstat);

  CoinPostsolveMatrix(OsiSolverInterface * si,

		   int ncols0,
		   int nrows0,
		   CoinBigIndex nelems0,
		     
		   double maxmin_,
		   // end prepost members

		   double *sol,
		   double *acts,

		   unsigned char *colstat,
		   unsigned char *rowstat);


  ~CoinPostsolveMatrix();

  /*! \name Column thread structures

    As mentioned in the class documentation, the entries for a given column
    do not necessarily occupy a contiguous block of space. The #link_ array
    is used to maintain the threading. There is one thread for each column,
    and a single thread for all free entries in #hrow_ and #colels_.
  */
  //@{

  /*! \brief First entry in free entries thread */
  CoinBigIndex free_list_;
  /// Allocated size of #link_
  int maxlink_;
  /*! \brief Thread array

    Within a thread, link_[k] points to the next entry in the thread.
  */
  int *link_;

  //@}

  // debug
  char *cdone_;
  char *rdone_;
  int nrows_;

  // needed for presolve_empty
  int nrows0_;


  // debugging
  void check_nbasic();

  ////virtual void postsolve(const CoinPresolveAction *paction);
};

/// Adjust objective function constant offset
void CoinPresolveMatrix::change_bias(double change_amount)
{
  dobias_ += change_amount;
#if DEBUG_PRESOLVE
  assert(fabs(change_amount)<1.0e50);
#endif
  if (change_amount)
    PRESOLVE_STMT(printf("changing bias by %g to %g\n",
		    change_amount, dobias_));  
}

// useful functions
inline void swap(int &x, int &y) { int temp = x; x = y; y = temp; }
inline void swap(double &x, double &y) { double temp = x; x = y; y = temp; }
inline void swap(long &x, long &y) { long temp = x; x = y; y = temp; }
#ifdef COIN_BIG_INDEX
inline void swap(long long &x, long long &y) { long long temp = x; x = y; y = temp; }
#endif

inline void swap(int *&x, int *&y) { int *temp = x; x = y; y = temp; }
inline void swap(double *&x, double *&y) { double *temp = x; x = y; y = temp; }
// This returns a non const array filled with input from scalar
// or actual array
template <class T> inline T*
copyOfArray( const T * array, const int size, T value)
{
  T * arrayNew = new T[size];
  if (array) {
    memcpy(arrayNew,array,size*sizeof(T));
  } else {
    int i;
    for (i=0;i<size;i++) 
      arrayNew[i] = value;
  }
  return arrayNew;
}

// This returns a non const array filled with actual array (or NULL)
template <class T> inline T*
copyOfArray( const T * array, const int size)
{
  if (array) {
    T * arrayNew = new T[size];
    memcpy(arrayNew,array,size*sizeof(T));
    return arrayNew;
  } else {
    return NULL;
  }
}

#define	PRESOLVEFINITE(n)	(-PRESOLVE_INF < (n) && (n) < PRESOLVE_INF)

void presolve_make_memlists(CoinBigIndex *starts, int *lengths,
		       presolvehlink *link,
		       int n);

/*! \relates CoinPostsolveMatrix
   \brief Find position of entry in threaded matrix

    The routine locates the position \p k in \p hrow for the specified \p row.
    It will abort if the entry does not exist.

    To use as presolve_find_col2: presolve_find_row2(icol,rs,nr,hcol,link).
*/
int presolve_find_row2(int irow, CoinBigIndex cs, int nc,
		       const int *hrow, const int *link);

/*! \relates CoinPostsolveMatrix
   \brief Find position of entry in threaded matrix

    The routine locates the position \p k in \p hrow for the specified \p row.
    It returns -1 if the entry is missing.

    To use as presolve_find_col3: presolve_find_row3(icol,rs,nr,hcol,link).
*/
int presolve_find_row3(int irow, CoinBigIndex ks, int nc,
		       const int *hrow, const int *link);

/*! \relates CoinPrePostsolveMatrix
   \brief Delete an entry from a packed/loose packed representation

   Removes a<ij> from \p hcol and \p dels, decrementing \p hinrow. Loose
   packing is maintained by swapping the last entry in the row into the
   position occupied by the deleted entry.

   To use as presolve_delete_from_col:
   presolve_delete_from_row(col,row,mcstrt,hincol,hrow,rowels)
*/
void presolve_delete_from_row(int row, int col /* thing to delete */,
		     const CoinBigIndex *mrstrt,
			      int *hinrow, int *hcol, double *dels);

/*! \relates CoinPostsolveMatrix
   \brief Delete an entry from a threaded representation

   Removes a<ij> from \p hcol and \p dels, decrementing \p hinrow and
   maintaining the thread for the row.

   To use as presolve_delete_from_col2:
   presolve_delete_from_row2(col,row,mcstrt,hincol,hrow,rowels)
*/
void presolve_delete_from_row2(int row, int col /* thing to delete */,
		      CoinBigIndex *mrstrt,
			       int *hinrow, int *hcol, double *dels, int *link, CoinBigIndex *free_listp);
#endif
