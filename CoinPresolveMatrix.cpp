// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

//#define	CHECK_CONSISTENCY	1

#include <stdio.h>

#include <assert.h>
#include <iostream>

#include "CoinHelperFunctions.hpp"

#include "CoinPresolveMatrix.hpp"
void CoinPresolveAction::throwCoinError(const char *error,
				       const char *ps_routine)
{
  throw CoinError(error, ps_routine, "CoinPresolve");
}


  class presolvehlink;

void presolve_make_memlists(CoinBigIndex *starts, int *lengths,
		       presolvehlink *link,
		       int n);

/*static*/ void presolve_prefix(const int *starts, int *diffs, int n, int limit);

/*static*/ void presolve_prefix(const CoinBigIndex *starts, int *diffs, int n, int limit)
{
  int i;

  for (i=0; i<n; i++) {
    diffs[i] = starts[i+1] - starts[i];
  }
}

// afterward, link[i].pre is the largest index less than i st lengths[i]!=0
//  (or -1 if all such lengths are 0).
// link[i].suc is the opposite.
// That is, it is the same thing as setting link[i].pre = i-1 and link[i].suc = i+1
// and then deleting all the empty elements.
// This list is maintained together with hrow/hcol so that as we relocate
// columns or rows, we can quickly determine what column/row precedes a given
// column/row in the memory region hrow/hcol.
// Note that n itself has a pre and a suc.
void presolve_make_memlists(CoinBigIndex *starts, int *lengths, presolvehlink *link, int n)
{
  int i;
  int pre = NO_LINK;
  
  for (i=0; i<n; i++) {
    if (lengths[i]) {
      link[i].pre = pre;
      if (pre != NO_LINK)
	link[pre].suc = i;
      pre = i;
    }
    else {
      link[i].pre = NO_LINK;
      link[i].suc = NO_LINK;
    }
  }
  if (pre != NO_LINK)
    link[pre].suc = n;

  // (1) Arbitrarily place the last non-empty entry in link[n].pre
  link[n].pre = pre;

  link[n].suc = NO_LINK;
}

// This one saves in one go to save [] memory
double * presolve_duparray(const double * element, const int * index,
			   int length, int offset)
{
  int n;
  if (sizeof(double)==2*sizeof(int)) 
    n = (3*length+1)>>1;
  else
    n = 2*length;
  double * dArray = new double [n];
  int * iArray = (int *) (dArray+length);
  memcpy(dArray,element+offset,length*sizeof(double));
  memcpy(iArray,index+offset,length*sizeof(int));
  return dArray;
}


double *presolve_duparray(const double *d, int n, int n2)
{
  double *d1 = new double[n2];
  memcpy(d1, d, n*sizeof(double));
  return (d1);
}
double *presolve_duparray(const double *d, int n)
{
  return presolve_duparray(d, n, n);
}

int *presolve_duparray(const int *d, int n, int n2)
{
  int *d1 = new int[n2];
  memcpy(d1, d, n*sizeof(int));
  return (d1);
}
int *presolve_duparray(const int *d, int n)
{
  return presolve_duparray(d, n, n);
}

char *presolve_duparray(const char *d, int n, int n2)
{
  char *d1 = new char[n2];
  memcpy(d1, d, n*sizeof(char));
  return (d1);
}
char *presolve_duparray(const char *d, int n)
{
  return presolve_duparray(d, n, n);
}

#if 0
double *presolve_duparray(const double *d, int n, char **end_mmapp)
{
  double *d1 = (double*)*end_mmapp;
  memcpy(d1, d, n*sizeof(double));
  *end_mmapp += ALIGN_DOUBLE(n*sizeof(double));
  return (d1);
}
int *presolve_duparray(const int *d, int n, char **end_mmapp)
{
  int *d1 = (int*)*end_mmapp;
  memcpy(d1, d, n*sizeof(int));
  *end_mmapp += ALIGN_DOUBLE(n*sizeof(int));
  return (d1);
}

double *presolve_duparray(const double *d, int n, int n2, char **end_mmapp)
{
  double *d1 = (double*)*end_mmapp;
  memcpy(d1, d, n*sizeof(double));
  *end_mmapp += ALIGN_DOUBLE(n2*sizeof(double));
  return (d1);
}
int *presolve_duparray(const int *d, int n, int n2, char **end_mmapp)
{
  int *d1 = (int*)*end_mmapp;
  memcpy(d1, d, n*sizeof(int));
  *end_mmapp += ALIGN_DOUBLE(n2*sizeof(int));
  return (d1);
}
#endif


int presolve_find_row(int row, CoinBigIndex kcs, CoinBigIndex kce, const int *hrow)
{
  CoinBigIndex k;
  for (k=kcs; k<kce; k++)
    if (hrow[k] == row)
      return (k);
  DIE("FIND_ROW");
  return (0);
}

int presolve_find_row1(int row, CoinBigIndex kcs, CoinBigIndex kce, const int *hrow)
{
  CoinBigIndex k;
  for (k=kcs; k<kce; k++)
    if (hrow[k] == row)
      return (k);
  return (kce);
}

int presolve_find_row2(int irow, CoinBigIndex ks, int nc, const int *hrow, const int *link)
{
  for (int i=0; i<nc; ++i) {
    if (hrow[ks] == irow)
      return (ks);
    ks = link[ks];
  }
  abort();
  return -1;
}

int presolve_find_row3(int irow, CoinBigIndex ks, int nc, const int *hrow, const int *link)
{
  for (int i=0; i<nc; ++i) {
    if (hrow[ks] == irow)
      return (ks);
    ks = link[ks];
  }
  return (-1);
}


// delete the entry for col from row
// this keeps the row loosely packed
// if we didn't want to maintain that property, we could just decrement hinrow[row].
void presolve_delete_from_row(int row, int col /* thing to delete */,
		     const CoinBigIndex *mrstrt,
		     int *hinrow, int *hcol, double *dels)
{
  CoinBigIndex krs = mrstrt[row];
  CoinBigIndex kre = krs + hinrow[row];

  CoinBigIndex kcol = presolve_find_row(col, krs, kre, hcol);

  swap(hcol[kcol], hcol[kre-1]);
  swap(dels[kcol], dels[kre-1]);
  hinrow[row]--;
}


void presolve_delete_from_row2(int row, int col /* thing to delete */,
		      CoinBigIndex *mrstrt,
		     int *hinrow, int *hcol, double *dels, int *link, 
			       CoinBigIndex *free_listp)
{
  CoinBigIndex k = mrstrt[row];

  if (hcol[k] == col) {
    mrstrt[row] = link[k];
    link[k] = *free_listp;
    *free_listp = k;
    check_free_list(k);
    hinrow[row]--;
  } else {
    int n = hinrow[row] - 1;
    CoinBigIndex k0 = k;
    k = link[k];
    for (int i=0; i<n; ++i) {
      if (hcol[k] == col) {
	link[k0] = link[k];
	link[k] = *free_listp;
	*free_listp = k;
	check_free_list(k);
	hinrow[row]--;
	return;
      }
      k0 = k;
      k = link[k];
    }
    abort();
  }
}



CoinPrePostsolveMatrix::~CoinPrePostsolveMatrix()
{
  delete[]mcstrt_;
  delete[]hrow_;
  delete[]colels_;
  delete[]hincol_;

  delete[]cost_;
  delete[]clo_;
  delete[]cup_;
  delete[]rlo_;
  delete[]rup_;
  delete[]originalColumn_;
  delete[]rowduals_;

  delete[]rcosts_;
}

// Sets status (non -basic ) using value
void 
CoinPrePostsolveMatrix::setRowStatusUsingValue(int iRow)
{
  double value = acts_[iRow];
  double lower = rlo_[iRow];
  double upper = rup_[iRow];
  if (lower<-1.0e20&&upper>1.0e20) {
    setRowStatus(iRow,isFree);
  } else if (fabs(lower-value)<=ztolzb_) {
    setRowStatus(iRow,atLowerBound);
  } else if (fabs(upper-value)<=ztolzb_) {
    setRowStatus(iRow,atUpperBound);
  } else {
    setRowStatus(iRow,superBasic);
  }
}
void 
CoinPrePostsolveMatrix::setColumnStatusUsingValue(int iColumn)
{
  double value = sol_[iColumn];
  double lower = clo_[iColumn];
  double upper = cup_[iColumn];
  if (lower<-1.0e20&&upper>1.0e20) {
    setColumnStatus(iColumn,isFree);
  } else if (fabs(lower-value)<=ztolzb_) {
    setColumnStatus(iColumn,atLowerBound);
  } else if (fabs(upper-value)<=ztolzb_) {
    setColumnStatus(iColumn,atUpperBound);
  } else {
    setColumnStatus(iColumn,superBasic);
  }
}


#if	DEBUG_PRESOLVE
static void matrix_bounds_ok(const double *lo, const double *up, int n)
{
  int i;
  for (i=0; i<n; i++) {
    PRESOLVEASSERT(lo[i] <= up[i]);
    PRESOLVEASSERT(lo[i] < PRESOLVE_INF);
    PRESOLVEASSERT(-PRESOLVE_INF < up[i]);
  }
}
#endif


CoinPresolveMatrix::~CoinPresolveMatrix()
{
  delete[]clink_;
  delete[]rlink_;
  
  delete[]mrstrt_;
  delete[]hinrow_;
  delete[]rowels_;
  delete[]hcol_;

  delete[]integerType_;
  delete[] rowChanged_;
  delete[] rowsToDo_;
  delete[] nextRowsToDo_;
  delete[] colChanged_;
  delete[] colsToDo_;
  delete[] nextColsToDo_;
  delete[] rowProhibited_;
  delete[] colProhibited_;
}

#if	CHECK_CONSISTENCY

// The matrix is represented redundantly in both row and column format,
// in what I call "loosely packed" format.
// "packed" format would be as in normal OSL:  a vector of column starts,
// together with two vectors that give the row indices and coefficients.
//
// This format is "loosely packed" because some of the elements may correspond
// to empty rows.  This is so that we can quickly delete rows without having
// to update the column rep and vice versa.
//
// Checks whether an entry is in the col rep iff it is also in the row rep,
// and also that their values are the same (if testvals is non-zero).
//
// Note that there may be entries in a row that correspond to empty columns
// and vice-versa.  --- HUH???
static void matrix_consistent(const CoinBigIndex *mrstrt, const int *hinrow, const int *hcol,
			      const CoinBigIndex *mcstrt, const int *hincol, const int *hrow,
			      const double *rowels,
			      const double *colels,
			      int nrows, int testvals,
			      const char *ROW, const char *COL)
{
  for (int irow=0; irow<nrows; irow++) {
    if (hinrow[irow] > 0) {
      CoinBigIndex krs = mrstrt[irow];
      CoinBigIndex kre = krs + hinrow[irow];

      for (CoinBigIndex k=krs; k<kre; k++) {
	int jcol = hcol[k];
	CoinBigIndex kcs = mcstrt[jcol];
	CoinBigIndex kce = kcs + hincol[jcol];

	CoinBigIndex kk = presolve_find_row1(irow, kcs, kce, hrow);
	if (kk == kce) {
	  printf("MATRIX INCONSISTENT:  can't find %s %d in %s %d\n",
		 ROW, irow, COL, jcol);
	  fflush(stdout);
	  abort();
	}
	if (testvals && colels[kk] != rowels[k]) {
	  printf("MATRIX INCONSISTENT:  value for %s %d and %s %d\n",
		 ROW, irow, COL, jcol);
	  fflush(stdout);
	  abort();
	}
      }
    }
  }
}
#endif


void CoinPresolveMatrix::consistent(bool testvals)
{
#if	CHECK_CONSISTENCY
  matrix_consistent(mrstrt_, hinrow_, hcol_,
		    mcstrt_, hincol_, hrow_,
		    rowels_, colels_,
		    nrows_, testvals,
		    "row", "col");
  matrix_consistent(mcstrt_, hincol_, hrow_,
		    mrstrt_, hinrow_, hcol_,
		    colels_, rowels_, 
		    ncols_, testvals,
		    "col", "row");
#endif
}











////////////////  POSTSOLVE


CoinPostsolveMatrix::~CoinPostsolveMatrix()
{
  delete[]link_;

  delete[]cdone_;
  delete[]rdone_;
}


void CoinPostsolveMatrix::check_nbasic()
{
  int nbasic = 0;

  int i;
  for (i=0; i<ncols_; i++)
    if (columnIsBasic(i))
      nbasic++;

  for (i=0; i<nrows_; i++)
    if (rowIsBasic(i))
      nbasic++;

  if (nbasic != nrows_) {
    printf("WRONG NUMBER NBASIC:  is:  %d  should be:  %d\n",
	   nbasic, nrows_);
    fflush(stdout);
  }
}






