// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef CoinPresolveFixed_H
#define CoinPresolveFixed_H
#define	FIXED_VARIABLE	1

class remove_fixed_action : public CoinPresolveAction {
 public:
  struct action {
    int col;
    int nincol;

    double sol;
    int *colrows;
    double *colels;
  };

  int nactions_;
  const action *actions_;

 private:
  remove_fixed_action(int nactions,
		      const action *actions,
		      const CoinPresolveAction *next);

 public:
  const char *name() const;

  static const remove_fixed_action *presolve(CoinPresolveMatrix *prob,
					 int *fcols,
					 int nfcols,
					 const CoinPresolveAction *next);

  void postsolve(CoinPostsolveMatrix *prob) const;

  ~remove_fixed_action();
};


const CoinPresolveAction *remove_fixed(CoinPresolveMatrix *prob,
				    const CoinPresolveAction *next);



class make_fixed_action : public CoinPresolveAction {
  struct action {
    double bound;
  };

  int nactions_;
  const action *actions_;

  const bool fix_to_lower_;
  const remove_fixed_action *faction_;

  make_fixed_action(int nactions,
		    const action *actions,
		    bool fix_to_lower,
		    const remove_fixed_action *faction,
		    const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nactions_(nactions), actions_(actions),
    fix_to_lower_(fix_to_lower),
    faction_(faction)
{}

 public:
  const char *name() const;

  static const CoinPresolveAction *presolve(CoinPresolveMatrix *prob,
					 int *fcols,
					 int hfcols,
					 bool fix_to_lower,
					 const CoinPresolveAction *next);

  void postsolve(CoinPostsolveMatrix *prob) const;

  ~make_fixed_action() { 
    deleteAction(actions_,action*); 
    delete faction_;
  };
};


const CoinPresolveAction *make_fixed(CoinPresolveMatrix *prob,
				    const CoinPresolveAction *next);
#endif
