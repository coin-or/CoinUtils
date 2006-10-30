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

#ifdef COIN_INT64_T
#define CoinInt64 COIN_INT64_T
#else
#define CoinInt64 int64_t
#endif
#define CoinIntPtr COIN_INTPTR_T

#endif
