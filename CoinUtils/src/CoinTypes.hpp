// Copyright (C) 2004, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef _CoinTypes_hpp
#define _CoinTypes_hpp

#include "CoinUtilsConfig.h"

#ifdef HAVE_CINTTYPES
# include <cinttypes>
#else
# ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
# endif
#endif

#define CoinInt64 COIN_INT64_T
#define CoinIntPtr COIN_INTPTR_T

//#############################################################################

#if defined(_MSC_VER)
   typedef __int64 int64_t;
#else
#  include <climits>
#endif

//#############################################################################

#endif
