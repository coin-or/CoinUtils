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
// MacOS-X and FreeBSD needs sys/time.h
#if defined(__MACH__) || defined (__FreeBSD__)
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

class CoinTimer
{
private:
   inline bool evaluate(const bool past) {
#ifndef COIN_COMPILE_WITH_TRACING
      int ipast = past;
      if (stream) {
	 
	 if (write_stream)
	    stream << ipast << "\n";
	 else 
	    stream >> ipast;
      }
      return ipast;
#else
      return past;
#endif
   }

private:
   double start;
   double limit;
   double end;
#ifdef COIN_COMPILE_WITH_TRACING
   std::fstream* stream;
   bool write_stream;
#endif

public:
   CoinTimer() :
      start(0), limit(DBL_MAX), end(DBL_MAX)
#ifdef COIN_COMPILE_WITH_TRACING
      , stream(0), write_stream(true)
#endif
   {}
   CoinTimer(double lim) :
      start(CoinCpuTime()), limit(lim), end(start+lim)
#ifdef COIN_COMPILE_WITH_TRACING
      , stream(0), write_stream(true)
#endif
   {}

#ifdef COIN_COMPILE_WITH_TRACING
   CoinTimer(std::fstream* s, bool w) :
      start(0), limit(DBL_MAX), end(DBL_MAX),
      stream(s), write_stream(true) {}
   CoinTimer(double lim, std::fstream* s, bool w) :
      start(CoinCpuTime()), limit(lim), end(start+lim),
      stream(s), write_stream(true) {}
#endif
   

   inline void restart() { start=CoinCpuTime(); end=start+limit; }
   inline void reset() { restart(); }
   inline void reset(double lim) { limit=lim; restart(); }

   inline bool isPastPercent(double pct) {
      return evaluate(start + limit * pct > CoinCpuTime());
   }
   inline bool isPast(double lim) {
      return evaluate(start + lim > CoinCpuTime());
   }
   inline bool isExpired() {
      return evaluate(end < CoinCpuTime());
   }
}

#endif
