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
   typedef void (__cdecl *func ) (int) CoinSighandler_t;
#  define CoinSighandler_t_defined   
#endif

//-----------------------------------------------------------------------------

#if (defined(__GNUC__) && defined(__linux__))
   typedef typeof(SIG_DFL) CoinSighandler_t;
#  define CoinSighandler_t_defined   
#endif

//-----------------------------------------------------------------------------

#if defined(__CYGWIN__) && defined(__GNUC__) 
   typedef typeof(SIG_DFL) CoinSighandler_t;
#  define CoinSighandler_t_defined   
#endif

//-----------------------------------------------------------------------------

#if defined(_AIX) && (defined(__GNUC__))
   typedef  CoinSighandler_t;
#  define CoinSighandler_t_defined   
#endif

//-----------------------------------------------------------------------------

#if defined(__sparc) && defined(__sun)
   typedef void ( *func ) (int) CoinSighandler_t;
#  define CoinSighandler_t_defined   
#endif

//-----------------------------------------------------------------------------

#if defined(__MACH__) && defined(__GNUC__)
   typedef typeof(SIG_DFL) CoinSighandler_t;
#  define CoinSighandler_t_defined   
#endif

//#############################################################################

#ifndef CoinSighandler_t_defined
#  warning("OS is not recognized.");
#  warning("CoinSighandler_t defaults to 'void(*func)(int)'.");
   typedef void ( *func ) (int) CoinSighandler_t;
#endif

#endif
