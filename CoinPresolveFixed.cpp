// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#include <stdio.h>
#include <math.h>

#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveFixed.hpp"

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
const remove_fixed_action *remove_fixed_action::presolve(CoinPresolveMatrix *prob,
						     int *fcols,
						     int nfcols,
						     const CoinPresolveAction *next)
{
  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt		= prob->mcstrt_;
  int *hincol		= prob->hincol_;

  double *rowels	= prob->rowels_;
  int *hcol		= prob->hcol_;
  CoinBigIndex *mrstrt		= prob->mrstrt_;
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
  // Find size of arrays
  int size=0;
  int ckc;
  for ( ckc=0; ckc<nfcols; ckc++) {
    int j = fcols[ckc];
    size += hincol[j];
  }
  double * els_action = new double[size];
  int * rows_action = new int[size];
  actions[nfcols].start=size;
  size=0;
  
  for ( ckc=0; ckc<nfcols; ckc++) {
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

    for (k=kcs; k<kce; k++) {
      int row = hrow[k];
      double coeff = colels[k];
      // save
      els_action[size]=coeff;
      rows_action[size++]=row;

      rlo[row] -= sol * coeff;
      rup[row] -= sol * coeff;
      acts[row] -= sol * coeff;

      // remove this column from all rows it occurs in in the row rep
      presolve_delete_from_row(row, j, mrstrt, hinrow, hcol, rowels);

      // mark
      prob->addRow(row);
      CoinBigIndex krs = mrstrt[row];
      CoinBigIndex kre = krs + hinrow[row];
      for (CoinBigIndex k=krs; k<kre; k++) {
	int jcol = hcol[k];
	prob->addCol(jcol);
      }
      // remove the column entirely from the col rep
      PRESOLVE_REMOVE_LINK(clink, j);
      hincol[j] = 0;
    }
  }
#if	PRESOLVE_SUMMARY
  printf("NFIXED:  %d\n", nfcols);
#endif

#if 0
  remove_fixed_action * nextAction =  new 
    remove_fixed_action(nfcols, actions, next);
  delete [] (void *) actions;
  return nextAction;
#else
  return (new remove_fixed_action(nfcols, actions, els_action,rows_action,next));
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
  action * actions = actions_;
  const int nactions	= nactions_;

  double *colels	= prob->colels_;
  int *hrow		= prob->hrow_;
  CoinBigIndex *mcstrt		= prob->mcstrt_;
  int *hincol		= prob->hincol_;
  int *link		= prob->link_;
  //  int ncols		= prob->ncols_;
  CoinBigIndex free_list		= prob->free_list_;

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

    /* the bounds in the reduced problem were tightened.
     * that means that this variable may not have been basic
     * because it didn't have to be,
     * but now it may have to.
     * no - the bounds aren't changed by this operation.
     */
    if (colstat)
      prob->setColumnStatus(icol,CoinPrePostsolveMatrix::atUpperBound);
  }

  prob->free_list_ = free_list;
}


const CoinPresolveAction *remove_fixed(CoinPresolveMatrix *prob,
				    const CoinPresolveAction *next)
{
  int ncols	= prob->ncols_;
  int *fcols	= new int[ncols];
  int nfcols	= 0;

  int *hincol		= prob->hincol_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;

  for (int i=0; i<ncols; i++)
    if (hincol[i] > 0 && clo[i] == cup[i]&&!prob->colProhibited2(i))
      fcols[nfcols++] = i;

  next = remove_fixed_action::presolve(prob, fcols, nfcols, next);
  delete[]fcols;
  return (next);
}



const char *make_fixed_action::name() const
{
  return ("make_fixed_action");
}

const CoinPresolveAction *make_fixed_action::presolve(CoinPresolveMatrix *prob,
						     int *fcols,
						     int nfcols,
						   bool fix_to_lower,
						   const CoinPresolveAction *next)
{
  double *clo	= prob->clo_;
  double *cup	= prob->cup_;
  double *csol = prob->sol_;

  double *colels	= prob->colels_;
  int *hrow	= prob->hrow_;
  CoinBigIndex *mcstrt	= prob->mcstrt_;
  int *hincol	= prob->hincol_;

  double *acts	= prob->acts_;


  action *actions 	= new  action[nfcols];

  for (int ckc=0; ckc<nfcols; ckc++) {
    int j = fcols[ckc];
    double movement;

    action &f = actions[ckc];
      
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
      for (k=mcstrt[j];k<mcstrt[j]+hincol[j];k++) {
	int row = hrow[k];
	acts[row] += movement*colels[k];
      }
    }
  }

  // this is unusual in that the make_fixed_action transform
  // contains within it a remove_fixed_action transform
  // bad idea?
  return (new make_fixed_action(nfcols, actions, fix_to_lower,
				remove_fixed_action::presolve(prob,
							      fcols, nfcols,
							      0),
				next));
}

void make_fixed_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_;
  //  const int nactions	= nactions_;
  const bool fix_to_lower	= fix_to_lower_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;

  faction_->postsolve(prob);

  for (int cnt = faction_->nactions_-1; cnt>=0; cnt--) {
    const action *f = &actions[cnt];
    const remove_fixed_action::action *f1 = &faction_->actions_[cnt];

    int icol = f1->col;
    if (fix_to_lower) {
      cup[icol] = f->bound;
    } else {
      clo[icol] = f->bound;
    }
  }
}


const CoinPresolveAction *make_fixed(CoinPresolveMatrix *prob,
				  const CoinPresolveAction *next)
{
  int ncols	= prob->ncols_;
  int *fcols	= new int[ncols];
  int nfcols	= 0;

  int *hincol		= prob->hincol_;

  double *clo	= prob->clo_;
  double *cup	= prob->cup_;

  for (int i=0; i<ncols; i++)
    if (hincol[i] > 0 && fabs(cup[i] - clo[i]) < ZTOLDP&&!prob->colProhibited2(i)) 
      fcols[nfcols++] = i;

  // Always make action as first to be done
  next = make_fixed_action::presolve(prob, fcols, nfcols,
				     true, // arbitrary
				     next);
  delete[]fcols;
  return (next);
}


