// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef CoinPresolveSingleton_H
#define CoinPresolveSingleton_H
#define	SLACK_DOUBLETON	2

const int MAX_SLACK_DOUBLETONS	= 1000;

class slack_doubleton_action : public CoinPresolveAction {
  struct action {
    double clo;
    double cup;

    double rlo;
    double rup;

    double coeff;

    int col;
    int row;
  };

  const int nactions_;
  const action *const actions_;

  slack_doubleton_action(int nactions,
			 const action *actions,
			 const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nactions_(nactions),
    actions_(actions)
{}

 public:
  const char *name() const { return ("slack_doubleton_action"); }

  // notFinished is set if action array filled up
  static const CoinPresolveAction *presolve(CoinPresolveMatrix *,
					   const CoinPresolveAction *next,
					bool &notFinished);

  void postsolve(CoinPostsolveMatrix *prob) const;


  ~slack_doubleton_action() { deleteAction(actions_,action*); }
};
#endif
