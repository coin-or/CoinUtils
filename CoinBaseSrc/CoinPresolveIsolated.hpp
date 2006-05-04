// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef CoinPresolveIsolated_H
#define CoinPresolveIsolated_H
class isolated_constraint_action : public CoinPresolveAction {
  double rlo_;
  double rup_;
  int row_;
  int ninrow_;
  const int *rowcols_;
  const double *rowels_;
  const double *costs_;

  isolated_constraint_action(double rlo,
			     double rup,
			     int row,
			     int ninrow,
			     const int *rowcols,
			     const double *rowels,
			     const double *costs,
			     const CoinPresolveAction *next) :
    CoinPresolveAction(next),
    rlo_(rlo), rup_(rup), row_(row), ninrow_(ninrow),
    rowcols_(rowcols), rowels_(rowels), costs_(costs) {}

 public:
  const char *name() const;

  static const CoinPresolveAction *presolve(CoinPresolveMatrix * prob,
					 int row,
					 const CoinPresolveAction *next);

  void postsolve(CoinPostsolveMatrix *prob) const;
};



#endif
