// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef CoinPresolveEmpty_H
#define CoinPresolveEmpty_H
// Drop all empty rows/cols from the problem.
// 
// This should only be done once, after all other presolving actions have
// been done.  

const int DROP_ROW = 3;
const int DROP_COL = 4;

class drop_empty_cols_action : public CoinPresolveAction {
private:
  const int nactions_;

  struct action {
    double clo;
    double cup;
    double cost;
    double sol;
    int jcol;
  };
  const action *const actions_;

  drop_empty_cols_action(int nactions,
			 const action *const actions,
			 const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nactions_(nactions), 
    actions_(actions)
  {}

 public:
  const char *name() const { return ("drop_empty_cols_action"); }

  static const CoinPresolveAction *presolve(CoinPresolveMatrix *,
					 int *ecols,
					 int necols,
					 const CoinPresolveAction*);

  static const CoinPresolveAction *presolve(CoinPresolveMatrix *prob,
					 const CoinPresolveAction *next);

  void postsolve(CoinPostsolveMatrix *prob) const;

  ~drop_empty_cols_action() { deleteAction(actions_,action*); }
};



class drop_empty_rows_action : public CoinPresolveAction {
private:
  struct action {
    double rlo;
    double rup;
    int row;
    int fill_row;	// which row was moved into position row to fill it
  };

  const int nactions_;
  const action *const actions_;

  drop_empty_rows_action(int nactions,
			 const action *actions,
			 const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nactions_(nactions), actions_(actions)
{}

 public:
  const char *name() const { return ("drop_empty_rows_action"); }

  static const CoinPresolveAction *presolve(CoinPresolveMatrix *prob,
					    const CoinPresolveAction *next);

  void postsolve(CoinPostsolveMatrix *prob) const;

  ~drop_empty_rows_action() { deleteAction(actions_,action*); }
};
#endif

