/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include <stdio.h>
#include <math.h>

#include "CoinPresolveMatrix.hpp"
#include "CoinPresolveEmpty.hpp"	// for DROP_COL/DROP_ROW
#include "CoinPresolveFixed.hpp"
#include "CoinPresolveSubst.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPresolveUseless.hpp"
#include "CoinPresolveForcing.hpp"
#include "CoinMessage.hpp"
#include "CoinFinite.hpp"

#if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
#include "CoinPresolvePsdebug.hpp"
#endif

/*
  Just spent some time fixing a bug in postsolve which was causing a loss of
  basic variables. Turns out that when there are many forcing constraints,
  they will have columns in common (no surprise). Postsolve was not taking
  this into account and the code to adjust status after changing bounds was a
  bit too simple-minded. You can see the problem in this trace:

  Entering forcing_constraint_action::postsolve, 3 constraints to process.
  Restoring constraint 61, 3 variables.
    x(87) NBLB cbar = 0, lb = 0, ub = 0 -> 1.79769e+308 -> NBLB.
    x(103) NBLB cbar = 0, lb = 0, ub = 0 -> 0 -> NBLB.
    x(106) NBUB cbar = -0.00547328, lb = 0, ub = 0 -> 0 -> NBUB.
  Restoring constraint 60, 4 variables.
    x(89) NBUB cbar = -5.55112e-17, lb = 0, ub = 0 -> 1.79769e+308 -> NBLB.
    x(92) NBUB cbar = -0.00548328, lb = 0, ub = 0 -> 0 -> NBUB.
    x(103) NBLB cbar = 0, lb = 0, ub = 0 -> 1.79769e+308 -> NBLB.
    x(106) NBUB cbar = -0.00547328, lb = 0, ub = 0 -> 0 -> NBUB.
  Restoring constraint 59, 2 variables.
    x(92) NBUB cbar = -0.00548328, lb = 0, ub = 0 -> 1.79769e+308 -> NBLB.
    x(106) NBUB cbar = -0.00547328, lb = 0, ub = 0 -> 1.79769e+308 -> NBLB.
    Adjusting row dual; x(92) NBLB -> B, y = 0.0 -> 0.00548328.
    Row status NBUB, lb = 0, ax = 0, ub = 1.79769e+308.
  Leaving forcing_constraint_action::postsolve.

  Look at x(106): in presolve, u(106) is initially forced to 0 when
  constraint 59 is processed. Subsequent bound `changes' when constraints
  60 and 61 are processed are really noops. In postsolve, there's no need to
  change the status until constraint 59 is processed.

  Notice that the status of x(106) is first forced to NBLB and then to basic
  when constraint 59 is processed. While working on this, I encountered
  situations where the variable then appeared when processing a later
  constraint, in which case it appeared that it needed to remain basic. But
  I'm thinking that this might be an artifact of getting it wrong for the
  `noop' case of the previous paragraph.

  The reason for leaving this note is to pose the following question: If
  there's no actual change in bound when a forcing constraint is processed,
  is it necessary to record the change in the postsolve object? Then there
  would be fewer bound change records to process and the postsolve code
  could return to its previous simplicity.

  -- lh, 111210 --
*/

namespace {

/*
  This just doesn't seem efficient, particularly when used to calculate row
  bounds. Lots of extra work.

  Consider replacing with CglTighten, once that code moves from experimental
  into trunk.  -- lh, 110523 --
*/
void implied_bounds (const double *els,
		     const double *clo, const double *cup,
		     const int *hcol,
		     CoinBigIndex krs, CoinBigIndex kre,
		     double *maxupp, double *maxdownp,
		     int jcol,
		     double rlo, double rup,
		     double *iclb, double *icub)
{
  if (rlo<=-PRESOLVE_INF&&rup>=PRESOLVE_INF) {
    *iclb = -PRESOLVE_INF;
    *icub =  PRESOLVE_INF;
    return;
  }
  bool posinf = false;
  bool neginf = false;
  double maxup = 0.0;
  double maxdown = 0.0;

  int jcolk = -1;

  // compute sum of all bounds except for jcol
  CoinBigIndex kk;
  for (kk=krs; kk<kre; kk++) {
    if (hcol[kk] == jcol)
      jcolk = kk;

    // swap jcol with hcol[kre-1];
    // that is, consider jcol last
    // this assumes that jcol occurs in this row
    CoinBigIndex k = (hcol[kk] == jcol
	     ? kre-1
	     : kk == kre-1
	     ? jcolk
	     : kk);

    PRESOLVEASSERT(k != -1);	// i.e. jcol had better be in the row

    int col = hcol[k];
    double coeff = els[k];
    double lb = clo[col];
    double ub = cup[col];

    // quick!  compute the implied col bounds before maxup/maxdown are changed
    if (kk == kre-1) {
      PRESOLVEASSERT(fabs(coeff) > ZTOLDP);

      double ilb = (rlo - maxup) / coeff;
      bool finite_ilb = (-PRESOLVE_INF < rlo && !posinf && PRESOLVEFINITE(maxup));

      double iub = (rup - maxdown) / coeff;
      bool finite_iub = ( rup < PRESOLVE_INF && !neginf && PRESOLVEFINITE(maxdown));

      if (coeff > 0.0) {
	*iclb = (finite_ilb ? ilb : -PRESOLVE_INF);
	*icub = (finite_iub ? iub :  PRESOLVE_INF);
      } else {
	*iclb = (finite_iub ? iub : -PRESOLVE_INF);
	*icub = (finite_ilb ? ilb :  PRESOLVE_INF);
      }
    }

    if (coeff > 0.0) {
      if (PRESOLVE_INF <= ub) {
	posinf = true;
	if (neginf)
	  break;	// pointless
      } else
	maxup += ub * coeff;

      if (lb <= -PRESOLVE_INF) {
	neginf = true;
	if (posinf)
	  break;	// pointless
      } else
	maxdown += lb * coeff;
    } else {
      if (PRESOLVE_INF <= ub) {
	neginf = true;
	if (posinf)
	  break;	// pointless
      } else
	maxdown += ub * coeff;

      if (lb <= -PRESOLVE_INF) {
	posinf = true;
	if (neginf)
	  break;	// pointless
      } else
	maxup += lb * coeff;
    }
  }

  // If we broke from the loop, then the bounds are infinite.
  // However, since we put the column whose implied bounds we want
  // to know at the end, and it doesn't matter if its own bounds
  // are infinite, don't worry about breaking at the last iteration.
  if (kk<kre-1) {
    *iclb = -PRESOLVE_INF;
    *icub =  PRESOLVE_INF;
  } else
    PRESOLVEASSERT(jcolk != -1);

  // store row bounds
  *maxupp   = (posinf) ?  PRESOLVE_INF : maxup;
  *maxdownp = (neginf) ? -PRESOLVE_INF : maxdown;
}

void implied_row_bounds(const double *els,
			       const double *clo, const double *cup,
			       const int *hcol,
			       CoinBigIndex krs, CoinBigIndex kre,
			       double *maxupp, double *maxdownp)
{
  int jcol = hcol[krs];
  bool posinf = false;
  bool neginf = false;
  double maxup = 0.0;
  double maxdown = 0.0;

  int jcolk = -1;

  // compute sum of all bounds except for jcol
  CoinBigIndex kk;
  for (kk=krs; kk<kre; kk++) {
    if (hcol[kk] == jcol)
      jcolk = kk;

    // swap jcol with hcol[kre-1];
    // that is, consider jcol last
    // this assumes that jcol occurs in this row
    CoinBigIndex k = (hcol[kk] == jcol
	     ? kre-1
	     : kk == kre-1
	     ? jcolk
	     : kk);

    PRESOLVEASSERT(k != -1);	// i.e. jcol had better be in the row

    int col = hcol[k];
    double coeff = els[k];
    double lb = clo[col];
    double ub = cup[col];

    if (coeff > 0.0) {
      if (PRESOLVE_INF <= ub) {
	posinf = true;
	if (neginf)
	  break;	// pointless
      } else
	maxup += ub * coeff;

      if (lb <= -PRESOLVE_INF) {
	neginf = true;
	if (posinf)
	  break;	// pointless
      } else
	maxdown += lb * coeff;
    } else {
      if (PRESOLVE_INF <= ub) {
	neginf = true;
	if (posinf)
	  break;	// pointless
      } else
	maxdown += ub * coeff;

      if (lb <= -PRESOLVE_INF) {
	posinf = true;
	if (neginf)
	  break;	// pointless
      } else
	maxup += lb * coeff;
    }
  }
  // store row bounds
  *maxupp   = (posinf) ?  PRESOLVE_INF : maxup;
  *maxdownp = (neginf) ? -PRESOLVE_INF : maxdown;
}

}	// end file-local namespace



const char *forcing_constraint_action::name() const
{
  return ("forcing_constraint_action");
}

/*
  It may be the case that the variable bounds are such that no matter what
  feasible value they take, the constraint cannot be violated; in this case,
  we obviously don't need to take it into account, and we just drop it as
  a USELESS constraint.

  On the other hand, it may be that the only way to satisfy a constraint
  is to jam all the constraint variables to one of their bounds; in this
  case, these variables are essentially fixed, which we do with a FORCING
  constraint.  For now, this just tightens the bounds; subsequently the
  fixed variables will be removed, then the row will be dropped.

  Since both operations use similar information (the implied row bounds),
  we combine them into one presolve routine.

  I claim that these checks could be performed in parallel, that is,
  the tests could be carried out for all rows in parallel, and then the
  rows deleted and columns tightened afterward.  Obviously, this is true
  for useless rows.  The potential problem is forcing constraints, but
  I think that is ok.  By doing it in parallel rather than sequentially,
  we may miss transformations due to variables that were fixed by forcing
  constraints, though.

  Note that both of these operations will cause problems if the variables
  in question really need to exceed their bounds in order to make the
  problem feasible.

  See comments at head of file -- lh, 111210 --
*/
const CoinPresolveAction*
  forcing_constraint_action::presolve (CoinPresolveMatrix *prob,
  				       const CoinPresolveAction *next)
{
# if PRESOLVE_DEBUG > 0 || COIN_PRESOLVE_TUNING
  int startEmptyRows = 0 ;
  int startEmptyColumns = 0 ;
  startEmptyRows = prob->countEmptyRows() ;
  startEmptyColumns = prob->countEmptyCols() ;
# endif
# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
# if PRESOLVE_DEBUG > 0
  std::cout << "Entering forcing_constraint_action::presolve." << std::endl ;
# endif
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif
# if COIN_PRESOLVE_TUNING
  double startTime = 0.0 ;
  if (prob->tuning_) {
    startTime = CoinCpuTime() ;
  }
# endif

  // Column solution and bounds
  double *clo = prob->clo_ ;
  double *cup = prob->cup_ ;
  double *csol = prob->sol_ ;

  // Row-major representation
  const CoinBigIndex *mrstrt = prob->mrstrt_ ;
  const double *rowels = prob->rowels_ ;
  const int *hcol = prob->hcol_ ;
  const int *hinrow = prob->hinrow_ ;
  const int nrows = prob->nrows_ ;

  const double *rlo = prob->rlo_ ;
  const double *rup = prob->rup_ ;

  const double tol = ZTOLDP ;
  const double inftol = prob->feasibilityTolerance_ ;
  const int ncols = prob->ncols_ ;

  int *fixed_cols = new int[ncols] ;
  int nfixed_cols = 0 ;

  action *actions = new action [nrows] ;
  int nactions = 0 ;

  int *useless_rows = new int[nrows] ;
  int nuseless_rows = 0 ;

  const int numberLook = prob->numberRowsToDo_ ;
  int *look = prob->rowsToDo_ ;

  bool fixInfeasibility = ((prob->presolveOptions_&0x4000) != 0) ;
/*
  Open a loop to scan the constraints of interest. There must be variables
  left in the row.
*/
  for (int iLook = 0 ; iLook < numberLook ; iLook++) {
    int irow = look[iLook] ;
    if (hinrow[irow] <= 0) continue ;

    const CoinBigIndex krs = mrstrt[irow] ;
    const CoinBigIndex kre = krs+hinrow[irow] ;
/*
  Calculate upper and lower bounds on the row activity based on upper and lower
  bounds on the variables. If these are finite and incompatible with the given
  row bounds, we have infeasibility.
*/
    double maxup, maxdown;
    implied_row_bounds(rowels,clo,cup,hcol,krs,kre,&maxup,&maxdown) ;
/*
  If the maximum lhs value is less than L(i) or the minimum lhs value is
  greater than U(i), we're infeasible.
*/
    if (maxup < PRESOLVE_INF &&
        maxup+inftol < rlo[irow] && !fixInfeasibility) {
      CoinMessageHandler *hdlr = prob->messageHandler() ;
      prob->status_|= 1 ;
      hdlr->message(COIN_PRESOLVE_ROWINFEAS,prob->messages())
	 << irow << rlo[irow] << rup[irow] << CoinMessageEol ;
      break ;
    }
    if (-PRESOLVE_INF < maxdown &&
        rup[irow] < maxdown-inftol && !fixInfeasibility) {
      CoinMessageHandler *hdlr = prob->messageHandler() ;
      prob->status_|= 1;
      hdlr->message(COIN_PRESOLVE_ROWINFEAS,prob->messages())
	 << irow << rlo[irow] << rup[irow] << CoinMessageEol ;
      break ;
    }
/*
  We've dealt with prima facie infeasibility. Now check if the constraint
  is trivially satisfied. If so, add it to the list of useless rows and move
  on.

  The reason we require maxdown and maxup to be finite if the row bound is
  finite is to guard against some subsequent transform changing a column
  bound from infinite to finite. Once finite, bounds continue to tighten,
  so we're safe.
*/
    if (((rlo[irow] <= -PRESOLVE_INF) ||
	 (-PRESOLVE_INF < maxdown && rlo[irow] <= maxdown-inftol)) &&
	((rup[irow] >= PRESOLVE_INF) ||
	 (maxup < PRESOLVE_INF && rup[irow] >= maxup+inftol))) {
      useless_rows[nuseless_rows++] = irow ;
      continue ;
    }
/*
  Is it the case that we can just barely attain L(i) or U(i)? If so, we have a
  forcing constraint. As explained above, we need maxup and maxdown to be
  finite in order for the test to be valid.
*/
    const bool tightAtLower = ((maxup < PRESOLVE_INF) &&
    			       (fabs(rlo[irow]-maxup) < tol)) ;
    const bool tightAtUpper = ((-PRESOLVE_INF < maxdown) &&
			       (fabs(rup[irow]-maxdown) < tol)) ;
    if (!(tightAtLower || tightAtUpper)) continue ;
/*
  We have a forcing constraint. Do we have space to handle it? Rare, to be
  sure, but we have the potential to queue up a column many times before
  reducing the list to unique indices.
*/
    if (nfixed_cols+(kre-krs) >= ncols) break ;
/*
  Get down to the business of fixing the variables at the appropriate bound.
  We need to remember the original value of the bound we're tightening.
  Allocate a pair of arrays the size of the row. Load variables fixed at l<j>
  from the start, variables fixed at u<j> from the end. Add the column to
  the list of columns to be processed further.
*/
    double *bounds = new double[hinrow[irow]] ;
    int *rowcols = new int[hinrow[irow]] ;
    int lk = krs ;
    int uk = kre ;
    for (CoinBigIndex k = krs ; k < kre ; k++) {
      int jcol = hcol[k] ;
      prob->addCol(jcol) ;
      double coeff = rowels[k] ;
      PRESOLVEASSERT(fabs(coeff) > ZTOLDP);
/*
  If maxup is tight at L(i), then we want to force variables x<j> to the bound
  that produced maxup: u<j> if a<ij> > 0, l<j> if a<ij> < 0. If maxdown is
  tight at U(i), it'll be just the opposite.
*/
      if (tightAtLower == (coeff > 0.0)) {
	--uk ;
	bounds[uk-krs] = clo[jcol] ;
	rowcols[uk-krs] = jcol ;
	if (csol != 0) {
	  csol[jcol] = cup[jcol] ;
	}
	clo[jcol] = cup[jcol] ;
      } else {
	bounds[lk-krs] = cup[jcol] ;
	rowcols[lk-krs] = jcol ;
	++lk ;
	if (csol != 0) {
	  csol[jcol] = clo[jcol] ;
	}
	cup[jcol] = clo[jcol] ;
      }
      fixed_cols[nfixed_cols++] = jcol ;
    }
    PRESOLVEASSERT(uk == lk);
    PRESOLVE_DETAIL_PRINT(printf("pre_forcing %dR E\n",irow));
#   if PRESOLVE_DEBUG > 1
    std::cout
      << "FORCING: row(" << irow << "), " << (kre-krs) << " variables."
      << std::endl ;
#   endif
/*
  Done with this row. Remember the changes in a postsolve action.
*/
    action *f = &actions[nactions] ;
    nactions++ ;
    f->row = irow ;
    f->nlo = lk-krs ;
    f->nup = kre-uk ;
    f->rowcols = rowcols ;
    f->bounds = bounds ;
  }
# if PRESOLVE_DEBUG > 0
  std::cout
    << "FORCING: " << nactions << " forcing, " << nuseless_rows << " useless."
    << std::endl ;
#endif
/*
  Done processing the rows of interest. Create a postsolve object.

  \todo: Why are we making a copy of actions? Why not just assign the array
         to the postsolve object?
*/
  if (nactions) {
    next = new forcing_constraint_action(nactions, 
			       CoinCopyOfArray(actions,nactions),next) ;
  }
  deleteAction(actions,action*) ;
/*
  Hand off the job of dealing with the useless rows to a specialist.
*/
  if (nuseless_rows) {
    next = useless_constraint_action::presolve(prob,
			   useless_rows,nuseless_rows,next);
  }
  delete [] useless_rows ;
/*
  Hand off the job of dealing with the fixed columns to a specialist.

  We need to remove duplicates here, or we get into trouble in
  remove_fixed_action::postsolve when we try to reinstate a column multiple
  times.
*/
  if (nfixed_cols) {
    if (nfixed_cols > 1) {
      std::sort(fixed_cols,fixed_cols+nfixed_cols) ;
      int *end = std::unique(fixed_cols,fixed_cols+nfixed_cols) ;
      nfixed_cols = static_cast<int>(end-fixed_cols) ;
    }
    next = remove_fixed_action::presolve(prob,fixed_cols,nfixed_cols,next) ;
  }
  delete[]fixed_cols ;

# if COIN_PRESOLVE_TUNING
  if (prob->tuning_) double thisTime = CoinCpuTime() ;
# endif
# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  presolve_check_sol(prob) ;
  presolve_check_nbasic(prob) ;
# endif
# if PRESOLVE_DEBUG > 0 || COIN_PRESOLVE_TUNING
  int droppedRows = prob->countEmptyRows()-startEmptyRows ;
  int droppedColumns =  prob->countEmptyCols()-startEmptyColumns ;
  std::cout
    << "Leaving forcing_constraint_action::presolve: removed " << droppedRows
    << " rows, " << droppedColumns << " columns" ;
# if COIN_PRESOLVE_TUNING > 0
  if (prob->tuning_)
    std::cout << " in " << (thisTime-prob->startTime_) << "s" ;
# endif
  std::cout << "." << std::endl ;
# endif

  return (next);
}

/*
  We're here to undo the bound changes that were put in place for forcing
  constraints. This is a bit trickier than it appears because the constraints
  are not necessarily independent. We need to take care when adjusting column
  status.
*/
void forcing_constraint_action::postsolve(CoinPostsolveMatrix *prob) const
{
  const action *const actions = actions_ ;
  const int nactions = nactions_ ;

  const double *colels = prob->colels_ ;
  const int *hrow = prob->hrow_ ;
  const CoinBigIndex *mcstrt = prob->mcstrt_ ;
  const int *hincol = prob->hincol_ ;
  const int *link = prob->link_ ;

  double *clo = prob->clo_ ;
  double *cup = prob->cup_ ;
  double *rlo = prob->rlo_ ;
  double *rup = prob->rup_ ;

  const double *sol = prob->sol_ ;
  double *rcosts = prob->rcosts_ ;

  double *acts = prob->acts_;
  double *rowduals = prob->rowduals_;

  const double ztoldj = prob->ztoldj_;
  const double ztolzb = prob->ztolzb_;

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout
    << "Entering forcing_constraint_action::postsolve, "
    << nactions << " constraints to process." << std::endl ;
# endif
# endif
/*
  Open a loop to process the actions. One action per constraint.
*/
  for (const action *f = &actions[nactions-1] ; actions <= f ; f--) {
    const int irow = f->row ;
    const int nlo = f->nlo ;
    const int nup = f->nup ;
    const int ninrow = nlo+nup ;
    const int *rowcols = f->rowcols ;
    const double *bounds = f->bounds ;

#   if PRESOLVE_DEBUG > 1
    std::cout
      << "  Restoring constraint " << irow << ", " << nlo+nup
      << " variables." << std::endl ;
#   endif
/*
  Process variables where the upper bound is relaxed.
    * If the variable is basic, we should leave the status unchanged.
      This case occurs when the variable was selected as most out-of-whack
      below and changed to basic when processing a previous constraint.
    * The bound change may be a noop, in which case we can choose to leave
      the status unchanged if the reduced cost is right.
    * Otherwise, the status should be set to NBLB.
*/
    for (int k = 0 ; k < nlo ; k++) {
      const int jcol = rowcols[k] ;
      PRESOLVEASSERT(fabs(sol[jcol]-clo[jcol]) <= ztolzb) ;
      const double cbarj = rcosts[jcol] ;
      const double olduj = cup[jcol] ;
      const double newuj = bounds[k] ;
      const bool change = (fabs(newuj-olduj) > ztolzb) ;

#     if PRESOLVE_DEBUG > 2
      std::cout
        << "    x(" << jcol << ") " << prob->columnStatusString(jcol)
	<< " cbar = " << cbarj << ", lb = " << clo[jcol]
	<< ", ub = " << olduj << " -> " << newuj ;
#     endif

      if (prob->getColumnStatus(jcol) != CoinPrePostsolveMatrix::basic) {
        if (change || cbarj > 0)
	  prob->setColumnStatus(jcol,CoinPrePostsolveMatrix::atLowerBound) ;
      }
      cup[jcol] = bounds[k] ;

#     if PRESOLVE_DEBUG > 2
      std::cout
	<< " -> " << prob->columnStatusString(jcol)
	<< "." << std::endl ;
#     endif
    }
/*
  Process variables where the lower bound is relaxed. The comments above
  apply.
*/
    for (int k = nlo ; k < ninrow ; k++) {
      const int jcol = rowcols[k] ;
      PRESOLVEASSERT(fabs(sol[jcol]-cup[jcol]) <= ztolzb) ;
      const double cbarj = rcosts[jcol] ;
      const double oldlj = clo[jcol] ;
      const double newlj = bounds[k] ;
      const bool change = (fabs(newlj-oldlj) > ztolzb) ;

#     if PRESOLVE_DEBUG > 2
      std::cout
        << "    x(" << jcol << ") " << prob->columnStatusString(jcol)
	<< " cbar = " << cbarj << ", ub = " << cup[jcol]
	<< ", lb = " << oldlj << " -> " << newlj ;
#     endif

      if (prob->getColumnStatus(jcol) != CoinPrePostsolveMatrix::basic) {
        if (change || cbarj < 0)
	  prob->setColumnStatus(jcol,CoinPrePostsolveMatrix::atUpperBound) ;
      }
      clo[jcol] = bounds[k] ;

#     if PRESOLVE_DEBUG > 2
      std::cout
	<< " -> " << prob->columnStatusString(jcol)
	<< "." << std::endl ;
#     endif
    }
/*
  This is a lazy implementation.  We tightened the col bounds, then let
  them be eliminated by repeated use of FIX_VARIABLE and a final DROP_ROW.
  Therefore, at this point the logical s<irow> for the row is basic and
  y<irow> = 0.0.  But we know this row is tight (by definition of forcing
  constraint), so we can have a nonzero dual and nonbasic logical. On the
  other hand, the reduced costs for the cols may or may not be ok for the
  relaxed column bounds and consequent new status.  Find the variable x<joow>
  most out-of-whack with respect to reduced cost and reduce cbar<joow>
  to zero using y<irow>. Then we can make x<joow> basic and s<irow> nonbasic.
*/
    PRESOLVEASSERT(prob->getRowStatus(irow) == CoinPrePostsolveMatrix::basic) ;
    PRESOLVEASSERT(rowduals[irow] == 0.0) ;

    int joow = -1 ;
    double yi = 0.0 ;
    for (int k = 0 ; k < ninrow ; k++) {
      int jcol = rowcols[k] ;
      CoinBigIndex kk = presolve_find_row2(irow,mcstrt[jcol],
      					   hincol[jcol],hrow,link) ;
      const double &cbarj = rcosts[jcol] ;
      const CoinPrePostsolveMatrix::Status statj = prob->getColumnStatus(jcol) ;
      if ((cbarj < -ztoldj && statj != CoinPrePostsolveMatrix::atUpperBound) ||
          (cbarj > ztoldj && statj != CoinPrePostsolveMatrix::atLowerBound)) {
	double yi_j = cbarj/colels[kk] ;
	if (fabs(yi_j) > fabs(yi)) {
	  joow = jcol ;
	  yi = yi_j ;
	}
      }
    }
/*
  Make x<joow> be basic and set the row status according to whether we're
  tight at the lower or upper bound. Keep in mind the convention that a
  <= constraint has a slack 0 <= s <= infty, while a >= constraint has a
  surplus -infty <= s <= 0.
*/
    if (joow != -1) {
#     if PRESOLVE_DEBUG > 1
      std::cout
        << "    Adjusting row dual; x(" << joow
	<< ") " << prob->columnStatusString(joow) << " -> "
	<< statusName(CoinPrePostsolveMatrix::basic)
	<< ", y = 0.0 -> " << yi << "." << std::endl ;
#     endif
      prob->setColumnStatus(joow,CoinPrePostsolveMatrix::basic);
      if (acts[irow]-rlo[irow] < rup[irow]-acts[irow])
	prob->setRowStatus(irow,CoinPrePostsolveMatrix::atUpperBound);
      else
	prob->setRowStatus(irow,CoinPrePostsolveMatrix::atLowerBound);
      rowduals[irow] = yi;
#     if PRESOLVE_DEBUG > 1
      std::cout
        << "    Row status " << prob->rowStatusString(irow)
        << ", lb = " << rlo[irow] << ", ax = " << acts[irow]
	<< ", ub = " << rup[irow] << "." << std::endl ;
#     endif
      for (int k = 0 ; k < ninrow ; k++) {
	int jcol = rowcols[k] ;
	CoinBigIndex kk = presolve_find_row2(irow,mcstrt[jcol],
					     hincol[jcol],hrow,link) ;
	rcosts[jcol] -= yi*colels[kk] ;
      }
    }
# if PRESOLVE_DEBUG > 0
  presolve_check_nbasic(prob) ;
# endif
  }

# if PRESOLVE_DEBUG > 0 || PRESOLVE_CONSISTENCY > 0
  presolve_check_threads(prob) ;
  presolve_check_sol(prob,2,2,2) ;
  presolve_check_nbasic(prob) ;
# if PRESOLVE_DEBUG > 0
  std::cout << "Leaving forcing_constraint_action::postsolve." << std::endl ;
# endif
# endif

}



#if 0		// (A)
// Determine the maximum and minimum values the constraint sums
// may take, given the bounds on the variables.
// If there are infinite terms, record where the first one is,
// and whether there is more than one.
// It is possible to compute implied bounds for the (one) variable
// with no bound.
static void implied_bounds1(CoinPresolveMatrix * prob, const double *rowels,
				const int *mrstrt,
				const int *hrow,
				const int *hinrow,
				const double *clo, const double *cup,
				const int *hcol,
				int ncols,
				const double *rlo, const double *rup,
				const char *integerType,
				int nrows,
				double *ilbound, double *iubound)
{
  const double tol = prob->feasibilityTolerance_;

  for (int irow=0; irow<nrows; irow++) {
    CoinBigIndex krs = mrstrt[irow];
    CoinBigIndex kre = krs + hinrow[irow];

    double irlo = rlo[irow];
    double irup = rup[irow];

    // These are used to set column bounds below.
    // If there are no (positive) infinite terms,
    // the loop will range from krs to kre;
    // if there is just one, it will range over that one variable;
    // otherwise, it will be empty.
    int ub_inf_index = -1;
    int lb_inf_index = -1;

    double maxup = 0.0;
    double maxdown = 0.0;
    CoinBigIndex k;
    for (k=krs; k<kre; k++) {
      int jcol = hcol[k];
      double coeff = rowels[k];
      double lb = clo[jcol];
      double ub = cup[jcol];

      // HAVE TO DEAL WITH BOUNDS OF INTEGER VARIABLES
      if (coeff > 0.0) {
	if (PRESOLVE_INF <= ub) {
	  if (ub_inf_index == -1) {
	    ub_inf_index = k;
	  } else {
	    ub_inf_index = -2;
	    if (lb_inf_index == -2)
	      break;	// pointless
	  }
	} else
	  maxup += ub * coeff;

	if (lb <= -PRESOLVE_INF) {
	  if (lb_inf_index == -1) {
	    lb_inf_index = k;
	  } else {
	    lb_inf_index = -2;
	    if (ub_inf_index == -2)
	      break;	// pointless
	  }
	} else
	  maxdown += lb * coeff;
      }
      else {
	if (PRESOLVE_INF <= ub) {
	  if (lb_inf_index == -1) {
	    lb_inf_index = k;
	  } else {
	    lb_inf_index = -2;
	    if (ub_inf_index == -2)
	      break;	// pointless
	  }
	} else
	  maxdown += ub * coeff;

	if (lb <= -PRESOLVE_INF) {
	  if (ub_inf_index == -1) {
	    ub_inf_index = k;
	  } else {
	    ub_inf_index = -2;
	    if (lb_inf_index == -2)
	      break;	// pointless
	  }
	} else
	  maxup += lb * coeff;
      }
    }

    // ub_inf says whether the sum of the "other" ub terms is infinite
    // in the loop below.
    // In the case where we only saw one infinite term, the loop
    // will only cover that case, in which case the other terms
    // are *not* infinite.
    // With two or more terms, it is infinite.
    // If we only saw one infinite term, then
    if (ub_inf_index == -2)
      maxup = PRESOLVE_INF;

    if (lb_inf_index == -2)
      maxdown = -PRESOLVE_INF;

    const bool maxup_finite = PRESOLVEFINITE(maxup);
    const bool maxdown_finite = PRESOLVEFINITE(maxdown);

    if (ub_inf_index == -1 && maxup_finite && maxup + tol < rlo[irow]&&!fixInfeasibility) {
      /* infeasible */
	prob->status_|= 1;
	prob->messageHandler()->message(COIN_PRESOLVE_ROWINFEAS,
					     prob->messages())
					       <<irow
					       <<rlo[irow]
					       <<rup[irow]
					       <<CoinMessageEol;
	break;
    } else if (lb_inf_index == -1 && maxdown_finite && rup[irow] < maxdown - tol&&!fixInfeasibility) {
      /* infeasible */
	prob->status_|= 1;
	prob->messageHandler()->message(COIN_PRESOLVE_ROWINFEAS,
					     prob->messages())
					       <<irow
					       <<rlo[irow]
					       <<rup[irow]
					       <<CoinMessageEol;
	break;
    }

    for (k = krs; k<kre; ++k) {
      int jcol = hcol[k];
      double coeff = rowels[k];

      // SHOULD GET RID OF THIS
      if (fabs(coeff) > ZTOLDP2 &&
	  !integerType[jcol]) {
	double maxup1 = (ub_inf_index == -1 || ub_inf_index == k
			 ? maxup
			 : PRESOLVE_INF);
	bool maxup_finite1 = (ub_inf_index == -1 || ub_inf_index == k
			      ? maxup_finite
			      : false);
	double maxdown1 = (lb_inf_index == -1 || lb_inf_index == k
			 ? maxdown
			 : PRESOLVE_INF);
	bool maxdown_finite1 = (ub_inf_index == -1 || ub_inf_index == k
			      ? maxdown_finite
			      : false);

	double ilb = (irlo - maxup1) / coeff;
	bool finite_ilb = (-PRESOLVE_INF < irlo && maxup_finite1);

	double iub = (irup - maxdown1) / coeff;
	bool finite_iub = ( irup < PRESOLVE_INF && maxdown_finite1);

	double ilb1 = (coeff > 0.0
		       ? (finite_ilb ? ilb : -PRESOLVE_INF)
		       : (finite_iub ? iub : -PRESOLVE_INF));

	if (ilbound[jcol] < ilb1) {
	  ilbound[jcol] = ilb1;
	  //if (jcol == 278001)
	  //printf("JCOL LB %g\n", ilb1);
	}
      }
    }

    for (k = krs; k<kre; ++k) {
      int jcol = hcol[k];
      double coeff = rowels[k];

      // SHOULD GET RID OF THIS
      if (fabs(coeff) > ZTOLDP2 &&
	  !integerType[jcol]) {
	double maxup1 = (ub_inf_index == -1 || ub_inf_index == k
			 ? maxup
			 : PRESOLVE_INF);
	bool maxup_finite1 = (ub_inf_index == -1 || ub_inf_index == k
			      ? maxup_finite
			      : false);
	double maxdown1 = (lb_inf_index == -1 || lb_inf_index == k
			 ? maxdown
			 : PRESOLVE_INF);
	bool maxdown_finite1 = (ub_inf_index == -1 || ub_inf_index == k
			      ? maxdown_finite
			      : false);


	double ilb = (irlo - maxup1) / coeff;
	bool finite_ilb = (-PRESOLVE_INF < irlo && maxup_finite1);

	double iub = (irup - maxdown1) / coeff;
	bool finite_iub = ( irup < PRESOLVE_INF && maxdown_finite1);

	double iub1 = (coeff > 0.0
		       ? (finite_iub ? iub :  PRESOLVE_INF)
		       : (finite_ilb ? ilb :  PRESOLVE_INF));

	if (iub1 < iubound[jcol]) {
	  iubound[jcol] = iub1;
	  //if (jcol == 278001)
	  //printf("JCOL UB %g\n", iub1);
	}
      }
    }
  }
}

#if 0		// (B)
postsolve for implied_bound
	{
	  double lo0	= pa->clo;
	  double up0	= pa->cup;
	  int irow	= pa->irow;
	  int jcol	= pa->icol;
	  int *rowcols	= pa->rowcols;
	  int ninrow	= pa->ninrow;

	  clo[jcol] = lo0;
	  cup[jcol] = up0;

	  if ((colstat[jcol] & PRESOLVE_XBASIC) == 0 &&
	      fabs(lo0 - sol[jcol]) > ztolzb &&
	      fabs(up0 - sol[jcol]) > ztolzb) {

	    // this non-basic variable is now away from its bound
	    // it is ok just to force it to be basic
	    // informally:  if this variable is at its implied bound,
	    // then the other variables must be at their bounds,
	    // which means the bounds will stop them even if the aren't basic.
	    if (rowstat[irow] & PRESOLVE_XBASIC)
	      rowstat[irow] = 0;
	    else {
	      int k;
	      for (k=0; k<ninrow; k++) {
		int col = rowcols[k];
		if (cdone[col] &&
		    (colstat[col] & PRESOLVE_XBASIC) &&
		    ((fabs(clo[col] - sol[col]) <= ztolzb && rcosts[col] >= -ztoldj) || 
		     (fabs(cup[col] - sol[col]) <= ztolzb && rcosts[col] <= ztoldj)))
		  break;
	      }
	      if (k<ninrow) {
		int col = rowcols[k];
		// steal this basic variable
#if	PRESOLVE_DEBUG > 0
		printf("PIVOTING ON COL:  %d %d -> %d\n", irow, col, jcol);
#endif
		colstat[col] = 0;

		// since all vars were at their bounds, the slack must be 0
		PRESOLVEASSERT(fabs(acts[irow]) < ZTOLDP);
		rowstat[irow] = PRESOLVE_XBASIC;
	      }
	      else {
		// should never happen?
		abort();
	      }

	      // get rid of any remaining basic structurals, since their rcosts
	      // are going to become non-zero in a second.
	      abort();
	      ///////////////////zero_pivot();
	    }

	    double rdual_adjust;
	    {
	      CoinBigIndex kk = presolve_find_row(irow, mcstrt[jcol], mcstrt[jcol] + hincol[jcol], hrow);
	      // adjust rowdual to cancel out reduced cost
	      // should probably search for col with largest factor
	      rdual_adjust = (rcosts[jcol] / colels[kk]);
	      rowduals[irow] += rdual_adjust;
	      colstat[jcol] = PRESOLVE_XBASIC;
	    }

	    for (k=0; k<ninrow; k++) {
	      int jcol = rowcols[k];
	      CoinBigIndex kk = presolve_find_row(irow, mcstrt[jcol], mcstrt[jcol] + hincol[jcol], hrow);
	      
	      rcosts[jcol] -= (rdual_adjust * colels[kk]);
	    }

	    {
	      int k;
	      int badbasic = -1;

	      // we may have just screwed up the rcost of another basic variable
	      for (k=0; k<ninrow; k++) {
		int col = rowcols[k];
		if (col != jcol &&
		    cdone[col] &&
		    (colstat[col] & PRESOLVE_XBASIC) &&
		    !(fabs(rcosts[col]) < ztoldj))
		  if (badbasic == -1)
		    badbasic = k;
		  else
		    abort();	// two!!  what to do???
	      }

	      if (badbasic != -1) {
		int col = rowcols[badbasic];

		if (fabs(acts[irow]) < ZTOLDP) {
#if	PRESOLVE_DEBUG > 0
		  printf("PIVOTING COL TO SLACK!:  %d %d\n", irow, col);
#endif
		  colstat[col] = 0;
		  rowstat[irow] = PRESOLVE_XBASIC;
		}
		else
		  abort();
	      }
	    }
	  }
	}
#endif		// #if 0	// (B)
#endif		// #if 0	// (A)

forcing_constraint_action::~forcing_constraint_action() 
{ 
  int i;
  for (i=0;i<nactions_;i++) {
    //delete [] actions_[i].rowcols; MS Visual C++ V6 can not compile
    //delete [] actions_[i].bounds; MS Visual C++ V6 can not compile
    deleteAction(actions_[i].rowcols,int *);
    deleteAction(actions_[i].bounds,double *);
  }
  // delete [] actions_; MS Visual C++ V6 can not compile
  deleteAction(actions_,action *);
}



