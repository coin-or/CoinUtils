// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef CoinPresolveDupcol_H
#define CoinPresolveDupcol_H
#define	DUPCOL	10

class dupcol_action : public CoinPresolveAction {
  struct action {
    double thislo;
    double thisup;
    double lastlo;
    double lastup;
    int ithis;
    int ilast;

    int *colrows;
    double *colels;
    int nincol;
  };

  const int nactions_;
  const action *const actions_;

  dupcol_action():CoinPresolveAction(NULL),nactions_(0),actions_(NULL) {};
  dupcol_action(int nactions,
		const action *actions,
		const CoinPresolveAction *next)/* :
    nactions_(nactions), actions_(actions),
    CoinPresolveAction(next) {}*/;

 public:
  const char *name() const;

  static const CoinPresolveAction *presolve(CoinPresolveMatrix *prob,
					 const CoinPresolveAction *next);

  void postsolve(CoinPostsolveMatrix *prob) const;

  //~dupcol_action() { delete[]actions_; }
};


class duprow_action : public CoinPresolveAction {
  struct action {
    int row;
    double lbound;
    double ubound;
  };

  const int nactions_;
  const action *const actions_;

  duprow_action():CoinPresolveAction(NULL),nactions_(0),actions_(NULL) {};
  duprow_action(int nactions,
		      const action *actions,
		      const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nactions_(nactions), actions_(actions) {}

 public:
  const char *name() const;

  static const CoinPresolveAction *presolve(CoinPresolveMatrix *prob,
					 const CoinPresolveAction *next);

  void postsolve(CoinPostsolveMatrix *prob) const;

  //~duprow_action() { delete[]actions_; }
};

#endif

