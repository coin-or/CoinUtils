// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef CoinPresolveZeros_H
#define CoinPresolveZeros_H

#define	DROP_ZERO	8

class drop_zero_coefficients_action : public CoinPresolveAction {

  const int nzeros_;
  const dropped_zero *const zeros_;

  drop_zero_coefficients_action(int nzeros,
				const dropped_zero *zeros,
				const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    nzeros_(nzeros), zeros_(zeros)
{}

 public:
  const char *name() const { return ("drop_zero_coefficients_action"); }

  static const CoinPresolveAction *presolve(CoinPresolveMatrix *prob,
					 int *checkcols,
					 int ncheckcols,
					 const CoinPresolveAction *next);

  void postsolve(CoinPostsolveMatrix *prob) const;

  ~drop_zero_coefficients_action() { deleteAction(zeros_,dropped_zero*); }
};

const CoinPresolveAction *drop_zero_coefficients(CoinPresolveMatrix *prob,
					      const CoinPresolveAction *next);

#endif
