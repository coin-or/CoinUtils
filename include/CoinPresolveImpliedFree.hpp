// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef CoinPresolveImpliedFree_H
#define CoinPresolveInpliedFree_H
#define	IMPLIED_FREE	9

class implied_free_action : public CoinPresolveAction {
  struct action {
    int row, col;
    double clo, cup;
    double rlo, rup;
    int ninrow;
    const double *rowels;
    const int *rowcols;
    const double *costs;
  };

  const int nactions_;
  const action *const actions_;

  implied_free_action(int nactions,
		      const action *actions,
		      const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nactions_(nactions), actions_(actions) {}

 public:
  const char *name() const;

  static const CoinPresolveAction *presolve(CoinPresolveMatrix * prob,
					 const CoinPresolveAction *next,
					int & fillLevel);

  void postsolve(CoinPostsolveMatrix *prob) const;

  ~implied_free_action() { deleteAction(actions_,action*); }
};

#endif
