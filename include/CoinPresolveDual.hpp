
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef CoinPresolveDual_H
#define CoinPresolveDual_H
class remove_dual_action : public CoinPresolveAction {
 public:
  remove_dual_action(int nactions,
		     //const action *actions,
		      const CoinPresolveAction *next);
  static const CoinPresolveAction *presolve(CoinPresolveMatrix *prob,
					 const CoinPresolveAction *next);
};
#endif


