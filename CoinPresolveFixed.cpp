// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#include <stdio.h>
#include <math.h>

#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveFixed.hpp"

/* Begin routines associated with remove_fixed_action */

const char *remove_fixed_action::name() const
{
  return ("remove_fixed_action");
}

/*
 * invariant:  both reps are loosely packed.
 * coefficients of both reps remain consistent.
 *
 * Note that this concerns variables whose column bounds determine that
 * they are slack; this does NOT concern singleton row constraints that
 * determine that the relevant variable is slack.
 *
 * Invariant:  col and row rep are consistent
 */
/*
  In fact, this routine doesn't really remove the column, it just empties
  it. Really removing it involves repacking the matrix representation, which
  is seriously expensive.

  remove_fixed_action implicitly assumes that the value of the variable
  has already been forced within bounds. If this isn't true, the correction
  to acts will be wrong. See make_fixed_action if you need to force the
  value within bounds first.
*/
const remove_fixed_action*
  remove_fixed_action::presolve (CoinPresolveMatrix *prob,
				 int *fcols,
				 int nfcols,
				 const CoinPresolveAction *next)
{
  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hincol		= prob->hincol_;

  double *rowels	= prob->rowels_;
  int *hcol		= prob->hcol_;
  CoinBigIndex *mrstrt	= prob->mrstrt_;
  int *hinrow		= prob->hinrow_;
  //  int nrows		= prob->nrows_;

  double *clo	= prob->clo_;
  //  double *cup	= prob->cup_;
  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;
  double *acts	= prob->acts_;

  //  double *dcost	= prob->cost_;

  presolvehlink *clink = prob->clink_;

  action *actions 	= new  action[nfcols+1];
  // Scan columns to be removed and total up the number of coefficients.
  int size=0;
  int ckc;
  for (ckc = 0 ; ckc < nfcols ; ckc++) {
    int j = fcols[ckc];
    size += hincol[j];
  }
  // Allocate arrays to hold coefficients and associated row indices
  double * els_action = new double[size];
  int * rows_action = new int[size];
  actions[nfcols].start=size;
  size=0;
  
  /*
    Open a loop to excise each column a<j>. The first thing to do is load the
    action entry with the index j, the value of x<j>, and the number of
    entries in a<j>. After we walk the column and tweak the row-major
    representation, we'll simply claim this column is empty by setting
    hincol[j] = 0.
  */
  for (ckc = 0 ; ckc < nfcols ; ckc++) {
    int j = fcols[ckc];
    double sol = clo[j];
    CoinBigIndex kcs = mcstrt[j];
    CoinBigIndex kce = kcs + hincol[j];
    CoinBigIndex k;

    {
      action &f = actions[ckc];
      
      f.col = j;
      f.sol = sol;

      f.start = size;

    }
    // the bias is updated when the empty column is removed
    //prob->change_bias(sol * dcost[j]);
/*
  Now walk a<j>. For each row i with a coefficient a<ij>:
    * save the coefficient and row index,
    * substitute the value of x<j>, adjusting the row bounds and lhs value
      accordingly, then
    * delete a<ij> from the row-major representation.
    * Finally, mark the row as changed (reasonable) and mark each remaining
      column in the row as changed (why?).
*/
    for (k = kcs ; k < kce ; k++) {
      int row = hrow[k];
      double coeff = colels[k];
     
      els_action[size]=coeff;
      rows_action[size++]=row;

      // Avoid reducing finite infinity.
      if (-PRESOLVE_INF < rlo[row])
	rlo[row] -= sol*coeff;
      if (rup[row] < PRESOLVE_INF)
	rup[row] -= sol*coeff;
      acts[row] -= sol*coeff;

      presolve_delete_from_row(row,j,mrstrt,hinrow,hcol,rowels);

      // mark unless already marked
      if (!prob->rowChanged(row)) {
	prob->addRow(row);
	CoinBigIndex krs = mrstrt[row];
	CoinBigIndex kre = krs + hinrow[row];
	for (CoinBigIndex k=krs; k<kre; k++) {
	  int jcol = hcol[k];
	  prob->addCol(jcol);
	}
      }
/*
  Remove the column's link from the linked list of columns, and declare
  it empty in the column-major representation.
*/
      PRESOLVE_REMOVE_LINK(clink, j);
      hincol[j] = 0;
    }
  }
#if	PRESOLVE_SUMMARY
  printf("NFIXED:  %d\n", nfcols);
#endif

/*
  No idea what this first bit might be used for. Looks to be a historical
  artififact --- there is no matching routine that I can see.  -- lh --
*/
#if 0
  remove_fixed_action * nextAction =  new 
    remove_fixed_action(nfcols, actions, next);
  delete [] (void *) actions;
  return nextAction;
#else
/*
  Create the postsolve object, link it at the head of the list of postsolve
  objects, and return a pointer.
*/
  return (new remove_fixed_action(nfcols,actions,
				  els_action,rows_action,next));
#endif
}

remove_fixed_action::remove_fixed_action(int nactions,
					 action *actions,
					 double * els_action,
					 int * rows_action,
					 const CoinPresolveAction *next) :
  CoinPresolveAction(next),
  colrows_(rows_action),
  colels_(els_action),
  nactions_(nactions),
  actions_(actions)
{
}

remove_fixed_action::~remove_fixed_action()
{
  deleteAction(actions_,action*);
  delete [] colels_;
  delete [] colrows_;
}

/*
 * Say we determined that cup - clo <= ztolzb, so we fixed sol at clo.
 * This involved subtracting clo*coeff from ub/lb for each row the
 * variable occurred in.
 * Now when we put the variable back in, by construction the variable
 * is within tolerance, the non-slacks are unchanged, and the 
 * distances of the affected slacks from their bounds should remain
 * unchanged (ignoring roundoff errors).
 * It may be that by adding the term back in, the affected constraints
 * now aren't as accurate due to round-off errors; this could happen
 * if only one summand and the slack in the original formulation were large
 * (and naturally had opposite signs), and the new term in the constraint
 * is about the size of the old slack, so the new slack becomes about 0.
 * It may be that there is catastrophic cancellation in the summation,
 * so it might not compute to 0.
 */
void remove_fixed_action::postsolve(CoinPostsolveMatrix *prob) const
{
  action * actions	= actions_;
  const int nactions	= nactions_;

  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hincol		= prob->hincol_;
  int *link		= prob->link_;
  //  int ncols		= prob->ncols_;
  CoinBigIndex free_list = prob->free_list_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;
  double *rlo	= prob->rlo_;
  double *rup	= prob->rup_;

  double *sol	= prob->sol_;
  double *dcost	= prob->cost_;
  double *rcosts	= prob->rcosts_;

  double *acts	= prob->acts_;
  double *rowduals = prob->rowduals_;

  unsigned char *colstat	= prob->colstat_;
  //  unsigned char *rowstat	= prob->rowstat_;

  const double maxmin	= prob->maxmin_;

  char *cdone	= prob->cdone_;
  //  char *rdone	= prob->rdone_;
  double * els_action = colels_;
  int * rows_action = colrows_;
  int end = actions[nactions].start;

  for (const action *f = &actions[nactions-1]; actions<=f; f--) {
    int icol = f->col;
    const double thesol = f->sol;

    cdone[icol] = FIXED_VARIABLE;

    sol[icol] = thesol;
    clo[icol] = thesol;
    cup[icol] = thesol;

    int cs = -11111;
    int start = f->start;
    double dj = maxmin * dcost[icol];
    
    for (int i=start; i<end; ++i) {
      int row = rows_action[i];
      double coeff =els_action[i];
      
      // pop free_list
      CoinBigIndex k = free_list;
      free_list = link[free_list];
      
      check_free_list(free_list);
      
      // restore
      hrow[k] = row;
      colels[k] = coeff;
      link[k] = cs;
      cs = k;
      
      if (-PRESOLVE_INF < rlo[row])
	rlo[row] += coeff * thesol;
      if (rup[row] < PRESOLVE_INF)
	rup[row] += coeff * thesol;
      acts[row] += coeff * thesol;
      
      dj -= rowduals[row] * coeff;
    }
    mcstrt[icol] = cs;
    
    rcosts[icol] = dj;
    hincol[icol] = end-start;
    end=start;

    /* Original comment:
     * the bounds in the reduced problem were tightened.
     * that means that this variable may not have been basic
     * because it didn't have to be,
     * but now it may have to.
     * no - the bounds aren't changed by this operation
     */
/*
  We've reintroduced the variable, but it's still fixed (equal bounds).
  Pick the nonbasic status that agrees with the reduced cost. Later, if
  postsolve unfixes the variable, we'll need to confirm that this status is
  still viable. We live in a minimisation world here.
*/
    if (colstat)
    { if (dj < 0)
	prob->setColumnStatus(icol,CoinPrePostsolveMatrix::atUpperBound);
      else
	prob->setColumnStatus(icol,CoinPrePostsolveMatrix::atLowerBound); }
	
  }

  prob->free_list_ = free_list;
}

/*
  Scan the problem for variables that are already fixed, and remove them.
  There's an implicit assumption that the value of the variable is already
  within bounds. If you want to protect against this possibility, you want to
  use make_fixed.
*/
const CoinPresolveAction *remove_fixed (CoinPresolveMatrix *prob,
					const CoinPresolveAction *next)
{
  int ncols	= prob->ncols_;
  int *fcols	= new int[ncols];
  int nfcols	= 0;

  int *hincol		= prob->hincol_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;

  for (int i = 0 ; i < ncols ; i++)
    if (hincol[i] > 0 && clo[i] == cup[i]&&!prob->colProhibited2(i))
      fcols[nfcols++] = i;

  next = remove_fixed_action::presolve(prob, fcols, nfcols, next);
  delete[]fcols;
  return (next);
}

/* End routines associated with remove_fixed_action */

/* Begin routines associated with make_fixed_action */

const char *make_fixed_action::name() const
{
  return ("make_fixed_action");
}


/*
  This routine does the actual job of fixing one or more variables. The set of
  indices to be fixed is specified by nfcols and fcols. fix_to_lower specifies
  the bound where the variable(s) should be fixed. The other bound is
  preserved as part of the action and the bounds are set equal. Note that you
  don't get to specify the bound on a per-variable basis.

  make_fixed_action will adjust the the row activity to compensate for forcing
  the variable within bounds. If bounds are already equal, and the variable is
  within bounds, you should consider remove_fixed_action.
*/
const CoinPresolveAction*
make_fixed_action::presolve (CoinPresolveMatrix *prob,
			      int *fcols, int nfcols,
			      bool fix_to_lower,
			      const CoinPresolveAction *next)

{ double *clo	= prob->clo_;
  double *cup	= prob->cup_;
  double *csol	= prob->sol_;

  double *colels = prob->colels_;
  int *hrow	= prob->hrow_;
  CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hincol	= prob->hincol_;

  double *acts	= prob->acts_;

  action *actions = new action[nfcols];

/*
  Scan the set of indices specifying variables to be fixed. For each variable,
  stash the unused bound in the action and set the bounds equal. If the value
  of the variable changes, update the solution.
*/
  for (int ckc = 0 ; ckc < nfcols ; ckc++)
  { int j = fcols[ckc] ;
    double movement ;

    action &f = actions[ckc] ;

    f.col = j ;
    if (fix_to_lower) {
      f.bound = cup[j];
      cup[j] = clo[j];
      movement = clo[j]-csol[j];
      csol[j] = clo[j];
    } else {
      f.bound = clo[j];
      clo[j] = cup[j];
      movement = cup[j]-csol[j];
      csol[j] = cup[j];
    }
    if (movement) {
      CoinBigIndex k;
      for (k = mcstrt[j] ; k < mcstrt[j]+hincol[j] ; k++) {
	int row = hrow[k];
	acts[row] += movement*colels[k];
      }
    }
  }
/*
  Original comment:
  This is unusual in that the make_fixed_action transform
  contains within it a remove_fixed_action transform
  bad idea?

  Explanatory comment:
  Now that we've adjusted the bounds, time to create the postsolve action
  that will restore the original bounds. But wait! We're not done. By calling
  remove_fixed_action::presolve, we will remove these variables from the
  model, caching the postsolve transform that will restore them inside the
  postsolve transform for fixing the bounds.
*/
  return (new make_fixed_action(nfcols, actions, fix_to_lower,
				remove_fixed_action::presolve(prob,
							      fcols, nfcols,
							      0),
				next));
}

/*
  Recall that in presolve, make_fixed_action forced a bound to fix a variable,
  then called remove_fixed_action to empty the column. removed_fixed_action
  left a postsolve object hanging off faction_, and our first act here is to
  call r_f_a::postsolve to repopulate the columns. The m_f_a postsolve activity
  consists of relaxing one of the bounds and making sure that the status is
  still viable (we can potentially eliminate the bound here).
*/
void make_fixed_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_;
  const int nactions = nactions_;
  const bool fix_to_lower = fix_to_lower_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;
  double *sol	= prob->sol_ ;
  unsigned char *colstat = prob->colstat_;
/*
  Repopulate the columns.
*/
  assert(nactions == faction_->nactions_) ;
  faction_->postsolve(prob);
/*
  Walk the actions: restore each bound and check that the status is still
  appropriate. Given that we're unfixing a fixed variable, it's safe to assume
  that the unaffected bound is finite.
*/
  for (int cnt = nactions-1 ; cnt >= 0 ; cnt--)
  { const action *f = &actions[cnt];
    int icol = f->col;
    double xj = sol[icol] ;

    assert(faction_->actions_[cnt].col == icol) ;

    if (fix_to_lower)
    { double ub = f->bound ;
      cup[icol] = ub ;
      if (colstat)
      { if (ub >= PRESOLVE_INF || xj != ub)
	{ prob->setColumnStatus(icol,
				CoinPrePostsolveMatrix::atLowerBound) ; } } }
    else
    { double lb = f->bound ;
      clo[icol] = lb ;
      if (colstat)
      { if (lb <= -PRESOLVE_INF || xj != lb)
	{ prob->setColumnStatus(icol,
				CoinPrePostsolveMatrix::atUpperBound) ; } } } }

  return ; }

/*!
  Scan the columns and collect indices of columns that have upper and lower
  bounds within the zero tolerance of one another. Hand this list to
  make_fixed_action::presolve() to do the heavy lifting.

  make_fixed_action will compensate for variables which are infeasible, forcing
  them to feasibility and correcting the row activity, before invoking
  remove_fixed_action to remove the variable from the problem. If you're
  confident of feasibility, consider remove_fixed.
*/
const CoinPresolveAction *make_fixed (CoinPresolveMatrix *prob,
				      const CoinPresolveAction *next)
{
  int ncols	= prob->ncols_;
  int *fcols	= new int[ncols];
  int nfcols	= 0;

  int *hincol	= prob->hincol_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;

  for (int i = 0 ; i < ncols ; i++)
  { if (hincol[i] > 0 &&
	fabs(cup[i] - clo[i]) < ZTOLDP && !prob->colProhibited2(i)) 
    { fcols[nfcols++] = i ; } }

/*
  Call m_f_a::presolve to do the heavy lifting. This will create a new
  CoinPresolveAction, which will become the head of the list of
  CoinPresolveAction's currently pointed to by next.
*/
  next = make_fixed_action::presolve(prob,fcols,nfcols,true,next) ;

  delete[]fcols ;
  return (next) ; }


