// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef CoinPresolveForcing_H
#define CoinPresolveForcing_H

/*!
  \file
*/

#define	IMPLIED_BOUND	7

/*! \class forcing_constraint_action
    \brief Detect and process forcing constraints and useless constraints

  A constraint is useless if the bounds on the variables prevent the constraint
  from ever being violated.

  A constraint is a forcing constraint if the bounds on the constraint force
  the value of an involved variable to one of its bounds. A constraint can
  force more than one variable.
*/
class forcing_constraint_action : public CoinPresolveAction {
  struct action {
    const int *rowcols;
    const double *bounds;
    int row;
    int nlo;
    int nup;
  };

  const int nactions_;
  const action *const actions_;

  forcing_constraint_action(int nactions,
		      const action *actions,
		      const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nactions_(nactions), actions_(actions) {};

 public:
  const char *name() const;

  static const CoinPresolveAction *presolve(CoinPresolveMatrix * prob,
					 const CoinPresolveAction *next);

  void postsolve(CoinPostsolveMatrix *prob) const;

  ~forcing_constraint_action();
};

#endif
