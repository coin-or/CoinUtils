// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

// Also put all base defines here e.g. VC++ pragmas
#ifndef CoinFinite_H
#define CoinFinite_H

#include <algorithm>
#include <cmath>

//=============================================================================

#if defined(__GNUC__) && defined(_AIX)
inline int CoinFinite(double d) { return d != DBL_MAX; }
# define CoinIsnan  isnan
#endif

//=============================================================================

#if !defined(__GNUC__) && defined(_AIX)
extern "C" {
   int isnan(double);
}
# define CoinFinite finite
# define CoinIsnan  isnan
#endif

//=============================================================================
#ifndef COIN_BIG_INDEX
typedef int CoinBigIndex;
#else
typedef long long CoinBigIndex;
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
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#if !defined(min)
#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#if !defined(max)
#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif
#else
// Put standard min and max here
using std::min;
using std::max;
#endif

//=============================================================================

#if defined(__linux__)
# define CoinFinite finite
# define CoinIsnan  isnan
#endif

//=============================================================================

#if defined(__CYGWIN32__)
# define CoinFinite finite
# define CoinIsnan  isnan
#endif

//=============================================================================

#if defined(__GNUC__) && defined(__MACH__)
extern "C" {
   int isnan(double);
}
inline int CoinFinite(double d);
# define CoinFinite finite
# define CoinIsnan  isnan
#endif

//=============================================================================

#if defined(__FreeBSD__)
extern "C" {
   int finite(double);
   int isnan(double);
}
# define CoinFinite finite
# define CoinIsnan  isnan
#endif

//=============================================================================
#endif
