// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
#ifndef CoinFinite_H
#define CoinFinite_H

#include <cmath>

//=============================================================================

#if defined(__GNUC__) && defined(_AIX)
inline int CoinFinite(double d) { return d != DBL_MAX; }
# define CoinIsnan  isnan
#endif

//=============================================================================

#if defined(sun)
extern "C" {
   int finite(double);
   int isnan(double);
}
# define CoinFinite finite
# define CoinIsnan  isnan
#endif

//=============================================================================

#if defined(_MSC_VER)
# include<float.h>
# define CoinIsnan   _isnan
# define CoinFinite  _finite
#endif

//=============================================================================

#if defined(__linux__)
# define CoinFinite finite
# define CoinIsnan  isnan
#endif

//=============================================================================

#endif
