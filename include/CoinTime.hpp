// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef _CoinTime_hpp
#define _CoinTime_hpp

//#############################################################################

#include <ctime>
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#else
// MacOS-X
#if defined(__MACH__)
#include <sys/time.h>
#endif
#include <sys/resource.h>
#endif

//#############################################################################

static inline double CoinCpuTime()
{
  double cpu_temp;
#if defined(_MSC_VER)
  unsigned int ticksnow;        /* clock_t is same as int */
  
  ticksnow = (unsigned int)clock();
  
  cpu_temp = (double)((double)ticksnow/CLOCKS_PER_SEC);
#else
  struct rusage usage;
  getrusage(RUSAGE_SELF,&usage);
  cpu_temp = usage.ru_utime.tv_sec;
  cpu_temp += 1.0e-6*((double) usage.ru_utime.tv_usec);
#endif
  return cpu_temp;
}

//#############################################################################

#endif
