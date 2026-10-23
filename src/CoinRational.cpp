// Authors: Matthew Saltzman and Ted Ralphs
// Copyright 2015, Matthew Saltzman and Ted Ralphs
// Licensed under the Eclipse Public License 

#include "CoinUtilsConfig.h"

#include <algorithm>
#include <cmath>
#ifdef __clang__
//labs() is in cstdlib with clang
#include <cstdlib>
#include <cstdio>
#endif
#include <cassert>

#include "CoinRational.hpp"

// Based on Python code from
// http://www.johndcook.com/blog/2010/10/20/best-rational-approximation/
// (with permission).
//
// Returns closest (or almost, anyway) rational to val with denominator less
// than or equal to maxdnom.  Return value is true if within tolerance, false
// otherwise.
bool CoinRational::nearestRational_(double val, double maxdelta, int64_t maxdnom)
{
  double intpart;
  if (floor(val)==val) {
    // An integral double can still lie far outside int64_t's range, and the
    // conversion is then undefined behaviour -- on x86 it yields INT64_MIN.
    // Returning that as a numerator poisons every gcd/lcm derived from it, and
    // INT64_MIN % -1 is a signed-division overflow, i.e. SIGFPE. That is how a
    // CglGMI cut carrying an rhs of 3.6e300 crashed
    // OsiCuts::insertIfNotDuplicateAndClean.
    //
    // Report "no nearby rational" instead. The constructor turns a false return
    // into 0/1, which every caller already handles as "cannot scale" -- this
    // path is not otherwise reachable, because the early return here is the
    // only one that skips the accuracy check at the end of the function.
    //
    // Both bounds are exactly representable (they are +/-2^63), and writing the
    // test negated also rejects NaN.
    if (!(val >= -9223372036854775808.0 && val < 9223372036854775808.0))
      return false;
    numerator_ = (int64_t) val;
    denominator_ = 1;
    return true;
  }
  double fracpart = fabs(modf(val, &intpart));
  // Consider using remainder() instead?

  int64_t a = 0, b = 1, c = 1, d = 1;
#define DEBUG_X 1
#if DEBUG_X
  bool shouldBeOK = false;
#endif
  while (b <= maxdnom && d <= maxdnom) {
    double mediant = (a + c) / (double(b + d));

    if (fabs(fracpart - mediant) < maxdelta) {
#if DEBUG_X
      shouldBeOK = true;
#endif
      if (b + d <= maxdnom * 2) { // seems more accurate (always true)
        numerator_ = a + c;
        denominator_ = b + d;
        break;
      } else if (d > b) {
        numerator_ = c;
        denominator_ = d;
        break;
      } else {
        numerator_ = a;
        denominator_ = b;
        break;
      }
    } else if (fracpart > mediant) {
      a = a + c;
      b = b + d;
    } else {
      c = a + c;
      d = b + d;
    }
    if (b > maxdnom) {
      numerator_ = c;
      denominator_ = d;
    } else {
      numerator_ = a;
      denominator_ = b;
    }
  }

#if DEBUG_X
  if (shouldBeOK) {
    double inaccuracy = fabs(fracpart - numerator_ / double(denominator_));
    assert(inaccuracy <= maxdelta);
  }
#endif
  numerator_ += ((int64_t) std::abs(intpart)) * denominator_;
  if (val < 0)
    numerator_ *= -1;
#if DEBUG_X > 1
  if (shouldBeOK) {
    printf("val %g is %lld/%lld to accuracy %g\n", val, numerator_, denominator_,
      fabs(val - numerator_ / double(denominator_)));
  }
#endif
  return fabs(val - numerator_ / double(denominator_)) <= maxdelta;
}

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
