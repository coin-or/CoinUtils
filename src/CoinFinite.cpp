// Copyright (C) 2011, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinFinite.hpp"

#include <cmath>

bool CoinFinite(double val)
{
  return std::isfinite(val);
}

bool CoinIsnan(double val)
{
  return std::isnan(val);
}

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
