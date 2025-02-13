// Authors: Matthew Saltzman and Ted Ralphs
// Copyright 2015, Matthew Saltzman and Ted Ralphs
// Licensed under the Eclipse Public License 

#ifndef CoinRational_H
#define CoinRational_H

#include <cmath>

#include "CoinUtilsConfig.h"

//Small class for rational numbers
class COINUTILSLIB_EXPORT CoinRational
{

public:
  int64_t getDenominator() { return denominator_; }
  int64_t getNumerator() { return numerator_; }

  CoinRational()
    : numerator_(0)
    , denominator_(1) {};

  CoinRational(int64_t n, int64_t d)
    : numerator_(n)
    , denominator_(d) {};

  CoinRational(double val, double maxdelta, int64_t maxdnom)
  {
    if (!nearestRational_(val, maxdelta, maxdnom)) {
      numerator_ = 0;
      denominator_ = 1;
    }
  };

private:
  int64_t numerator_;
  int64_t denominator_;

  bool nearestRational_(double val, double maxdelta, int64_t maxdnom);
};

#endif

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
