// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

// Also put all base defines here e.g. VC++ pragmas
#ifndef CoinFinite_H
#define CoinFinite_H

#include <algorithm>
#include <cmath>

//=============================================================================
#if COIN_BIG_INDEX==0
typedef int CoinBigIndex;
#elif COIN_BIG_INDEX==1
typedef long CoinBigIndex;
#else
typedef long long CoinBigIndex;
#endif
// Plus infinity
#ifndef COIN_DBL_MAX
#define COIN_DBL_MAX DBL_MAX
#endif

//=============================================================================

#if defined (_AIX)
#  if defined(__GNUC__)
      inline int CoinFinite(double d) { return d != DBL_MAX; }
#     define CoinIsnan  isnan
#  else
      extern "C" {
         int isnan(double);
      }
#     define CoinFinite finite
#     define CoinIsnan  isnan
#  endif
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

#if defined (__hpux)
#  if defined(__GNUC__)
      inline int CoinFinite(double d) { return d != DBL_MAX; }
#     define CoinIsnan  isnan
#  else
#     define CoinFinite isfinite
#     define CoinIsnan  isnan
#  endif
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
