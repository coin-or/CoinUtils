/* $Id$ */
// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

/* Defines COIN_DBL_MAX and relatives and provides CoinFinite and CoinIsnan.
 * NOTE: If used outside of CoinUtils, it is assumed that the HAVE_CFLOAT, HAVE_CMATH, and HAVE_CIEEEFP symbols have been defined,
 * so DBL_MAX and the functions behind COIN_C_FINITE and COIN_C_ISNAN are defined.
 * The reason is that the public config header file of CoinUtils does not define these symbols anymore, since it does not know on
 * which system the library will be used.
 *
 * As a consequence, it is suggested to include this header only in .c/.cpp files, but not in header files,
 * since this introduces the same assumptions to these headers and everyone using them.
 */

#ifndef CoinFinite_H
#define CoinFinite_H

#include "CoinUtilsConfig.h"

#ifdef HAVE_CFLOAT
# include <cfloat>
#else
# ifdef HAVE_FLOAT_H
#  include <float.h>
# endif
#endif

#ifdef HAVE_CMATH
# include <cmath>
#else
# ifdef HAVE_MATH_H
#  include <math.h>
# endif
#endif

#ifdef HAVE_CIEEEFP
# include <cieeefp>
#else
# ifdef HAVE_IEEEFP_H
#  include <ieeefp.h>
# endif
#endif

//=============================================================================
// Plus infinity (double and int)
#ifndef COIN_DBL_MAX
#ifndef DBL_MAX
#warning "Using COIN_DBL_MAX may fail since DBL_MAX is not defined. Probably your projects XyzConfig.h need to be included before CoinFinite.hpp."
#endif
#define COIN_DBL_MAX DBL_MAX
#endif

#ifndef COIN_INT_MAX
#define COIN_INT_MAX (static_cast<int>((~(static_cast<unsigned int>(0))) >> 1))
#endif

#ifndef COIN_INT_MAX_AS_DOUBLE
#define COIN_INT_MAX_AS_DOUBLE (static_cast<double>((~(static_cast<unsigned int>(0))) >> 1))
#endif

inline bool CoinFinite(double val)
{
#ifdef COIN_C_FINITE
    return COIN_C_FINITE(val)!=0;
#else
    return val != DBL_MAX && val != -DBL_MAX;
#endif
}

inline bool CoinIsnan(double val)
{
#ifdef COIN_C_ISNAN
    return COIN_C_ISNAN(val)!=0;
#else
    return false;
#endif
}

#endif
