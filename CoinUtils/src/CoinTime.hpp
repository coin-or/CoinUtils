// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef _CoinTime_hpp
#define _CoinTime_hpp

// Uncomment the next three lines for thorough memory initialisation.
// #ifndef ZEROFAULT
// # define ZEROFAULT
// #endif

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
#if !defined(__MSVCRT__)
#include <sys/resource.h>
#endif
#endif

//#############################################################################

static inline double CoinCpuTime()
{
  double cpu_temp;
#if defined(_MSC_VER) || defined(__MSVCRT__)
  unsigned int ticksnow;        /* clock_t is same as int */
  
  ticksnow = (unsigned int)clock();
  
  cpu_temp = (double)((double)ticksnow/CLOCKS_PER_SEC);
#else
  struct rusage usage;
# ifdef ZEROFAULT
  usage.ru_utime.tv_sec = 0 ;
  usage.ru_utime.tv_usec = 0 ;
# endif
  getrusage(RUSAGE_SELF,&usage);
  cpu_temp = usage.ru_utime.tv_sec;
  cpu_temp += 1.0e-6*((double) usage.ru_utime.tv_usec);
#endif
  return cpu_temp;
}

//#############################################################################

#include <fstream>

/**
 This class implements a timer that also implements a tracing functionality.

 The timer stores the start time of the timer, for how much time it was set to
 and when does it expire (start + limit = end). Queries can be made that tell
 whether the timer is expired, is past an absolute time, is past a percentage
 of the length of the timer. All times are given in seconds, but as double
 numbers, so there can be fractional values.

 The timer can also be initialized with a stream and a specification whether
 to write to or read from the stream. In the former case the result of every
 query is written into the stream, in the latter case timing is not tested at
 all, rather the supposed result is read out from the stream. This makes it
 possible to exactly retrace time sensitive program execution.
*/
class CoinTimer
{
private:
   /// When the timer was initialized/reset/restarted
   double start;
   /// 
   double limit;
   double end;
#ifdef COIN_COMPILE_WITH_TRACING
   std::fstream* stream;
   bool write_stream;
#endif

private:
#ifdef COIN_COMPILE_WITH_TRACING
   inline bool evaluate(bool b_tmp) const {
      int i_tmp = b_tmp;
      if (stream) {
	 if (write_stream)
	    (*stream) << i_tmp << "\n";
	 else 
	    (*stream) >> i_tmp;
      }
      return i_tmp;
   }
   inline double evaluate(double d_tmp) const {
      if (stream) {
	 if (write_stream)
	    (*stream) << d_tmp << "\n";
	 else 
	    (*stream) >> d_tmp;
      }
      return d_tmp;
   }
#else
   inline bool evaluate(const bool b_tmp) const {
      return b_tmp;
   }
   inline double evaluate(const double d_tmp) const {
      return d_tmp;
   }
#endif   

public:
   /// Default constructor creates a timer with no time limit and no tracing
   CoinTimer() :
      start(0), limit(1e100), end(1e100)
#ifdef COIN_COMPILE_WITH_TRACING
      , stream(0), write_stream(true)
#endif
   {}

   /// Create a timer with the given time limit and with no tracing
   CoinTimer(double lim) :
      start(CoinCpuTime()), limit(lim), end(start+lim)
#ifdef COIN_COMPILE_WITH_TRACING
      , stream(0), write_stream(true)
#endif
   {}

#ifdef COIN_COMPILE_WITH_TRACING
   /** Create a timer with no time limit and with writing/reading the trace
       to/from the given stream, depending on the argument \c write. */
   CoinTimer(std::fstream* s, bool write) :
      start(0), limit(1e100), end(1e100),
      stream(s), write_stream(write) {}
   
   /** Create a timer with the given time limit and with writing/reading the
       trace to/from the given stream, depending on the argument \c write. */
   CoinTimer(double lim, std::fstream* s, bool w) :
      start(CoinCpuTime()), limit(lim), end(start+lim),
      stream(s), write_stream(w) {}
#endif
   
   /// Restart the timer (keeping the same time limit)
   inline void restart() { start=CoinCpuTime(); end=start+limit; }
   /// An alternate name for \c restart()
   inline void reset() { restart(); }
   /// Reset (and restart) the timer and change its time limit
   inline void reset(double lim) { limit=lim; restart(); }

   /** Return whether the given percentage of the time limit has elapsed since
       the timer was started */
   inline bool isPastPercent(double pct) const {
      return evaluate(start + limit * pct < CoinCpuTime());
   }
   /** Return whether the given amount of time has elapsed since the timer was
       started */
   inline bool isPast(double lim) const {
      return evaluate(start + lim < CoinCpuTime());
   }
   /** Return whether the originally specified time limit has passed since the
       timer was started */
   inline bool isExpired() const {
      return evaluate(end < CoinCpuTime());
   }

   /** Return how much time is left on the timer */
   inline double timeLeft() const {
      return evaluate(end - CoinCpuTime());
   }

   /** Return how much time has elapsed */
   inline double timeElapsed() const {
      return evaluate(CoinCpuTime() - start);
   }

   inline void setLimit(double l) {
      limit = l;
      return;
   }
};

#endif
