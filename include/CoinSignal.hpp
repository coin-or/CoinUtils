// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef _CoinSignal_hpp
#define _CoinSignal_hpp

// This file is fully docified.
// There's nothing to docify...

//#############################################################################

#include <csignal>

//#############################################################################

#if defined(_MSC_VER)
   typedef void (__cdecl *CoinSighandler_t) (int);

#else /* ~_MSC_VER */
#  if defined(__GNUC__)
     typedef typeof(SIG_DFL) CoinSighandler_t;

#  else /* ~__GNUC__ */
#    if defined(_AIX) || defined(__sparc)
       typedef void (__cdecl *CoinSighandler_t) (int);
#    else
#      warning("Not g++ and OS is not recognized.");
#      warning("defaulting to typedef void (__cdecl *CoinSighandler_t) (int);")
       typedef void (__cdecl *CoinSighandler_t) (int);
#    endif

#  endif /* __GNUC__ */
#endif /* _MSC_VER */

//#############################################################################

#endif
