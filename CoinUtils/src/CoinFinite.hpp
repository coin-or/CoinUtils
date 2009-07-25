/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

// Also put all base defines here e.g. VC++ pragmas
#ifndef CoinFinite_H
#define CoinFinite_H

#include "CoinUtilsConfig.h"

#include <cstdlib>
#ifdef HAVE_CMATH
# include <cmath>
#else
# ifdef HAVE_MATH_H
#  include <math.h>
# else
#  error "don't have header file for math"
# endif
#endif

#ifdef HAVE_CFLOAT
# include <cfloat>
#else
# ifdef HAVE_FLOAT_H
#  include <float.h>
# endif
#endif

#ifdef HAVE_CIEEEFP
# include <cieeefp>
#else
# ifdef HAVE_IEEEFP_H
#  include <ieeefp.h>
# endif
#endif

#include <algorithm>

//=============================================================================
// Compilers can produce better code if they know about __restrict
#ifdef COIN_USE_RESTRICT
#define COIN_RESTRICT __restrict
#else
#define COIN_RESTRICT 
#endif
//=============================================================================
// Switch on certain things if COIN_FAST_CODE
#ifdef COIN_FAST_CODE
#ifndef COIN_NOTEST_DUPLICATE
#define COIN_NOTEST_DUPLICATE
#endif
#ifndef COIN_USE_EKK_SORT
#define COIN_USE_EKK_SORT
#endif
#endif
//=============================================================================
#if COIN_BIG_INDEX==0
typedef int CoinBigIndex;
#elif COIN_BIG_INDEX==1
typedef long CoinBigIndex;
#else
typedef long long CoinBigIndex;
#endif

//=============================================================================
#ifndef COIN_BIG_DOUBLE 
#define COIN_BIG_DOUBLE 0
#endif
// See if we want the ability to have long double work arrays
#if COIN_BIG_DOUBLE==2
#undef COIN_BIG_DOUBLE 
#define COIN_BIG_DOUBLE 0
#define COIN_LONG_WORK 1
typedef long double CoinWorkDouble;
#elif COIN_BIG_DOUBLE==3
#undef COIN_BIG_DOUBLE 
#define COIN_BIG_DOUBLE 1
#define COIN_LONG_WORK 1
typedef long double CoinWorkDouble;
#else
#define COIN_LONG_WORK 0
typedef double CoinWorkDouble;
#endif
#if COIN_BIG_DOUBLE==0
typedef double CoinFactorizationDouble;
#elif COIN_BIG_DOUBLE==1
typedef long double CoinFactorizationDouble;
#else
typedef double CoinFactorizationDouble;
#endif

//=============================================================================
// Plus infinity (double and int)
#ifndef COIN_DBL_MAX
#define COIN_DBL_MAX DBL_MAX
#endif

#ifndef COIN_INT_MAX
#define COIN_INT_MAX (static_cast<int>((~(static_cast<unsigned int>(0))) >> 1))
#endif

#ifndef COIN_INT_MAX_AS_DOUBLE
#define COIN_INT_MAX_AS_DOUBLE (static_cast<double>((~(static_cast<unsigned int>(0))) >> 1))
#endif

//=============================================================================

inline bool CoinFinite(double val)
{
#ifdef MY_C_FINITE
  //    return static_cast<bool>(MY_C_FINITE(val));
    return MY_C_FINITE(val)!=0;
#else
    return val != DBL_MAX && val != -DBL_MAX;
#endif
}

//=============================================================================

inline bool CoinIsnan(double val)
{
#ifdef MY_C_ISNAN
  //    return static_cast<bool>(MY_C_ISNAN(val));
    return MY_C_ISNAN(val)!=0;
#else
    return false;
#endif
}

//=============================================================================

#endif
