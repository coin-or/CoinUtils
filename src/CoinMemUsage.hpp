// Copyright (C) 2026, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef _CoinMemUsage_hpp
#define _CoinMemUsage_hpp

#include "CoinUtilsConfig.h"

//#############################################################################
//
// Portable helpers to query the current process's resident memory usage and
// the total physical memory installed on the machine, in bytes. These are
// used (among other things) to implement a "memory limit" stopping criterion
// during branch and bound, in the same spirit as CoinCpuTime()/
// CoinWallclockTime() in CoinTime.hpp are used for time-based limits.
//
// Both functions return 0 if the requested quantity could not be
// determined; callers should treat 0 as "unknown" rather than "no memory
// used"/"no memory installed".
//
//#############################################################################

#include <cstddef>

#if defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>

/// Returns the current resident set size (physical memory currently used by
/// this process, a.k.a. the "working set" on Windows), in bytes.
inline size_t CoinGetCurrentMemUsage()
{
  PROCESS_MEMORY_COUNTERS pmc;
  // K32GetProcessMemoryInfo has been part of kernel32.dll since Windows
  // Vista, so no extra link-time dependency on psapi.lib is needed.
  if (K32GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    return static_cast< size_t >(pmc.WorkingSetSize);
  return 0;
}

/// Returns the total physical memory installed on this machine, in bytes.
inline size_t CoinGetTotalPhysicalMemory()
{
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  if (GlobalMemoryStatusEx(&status))
    return static_cast< size_t >(status.ullTotalPhys);
  return 0;
}

#elif defined(__APPLE__)

#include <mach/mach.h>
#include <sys/sysctl.h>
#include <unistd.h>

inline size_t CoinGetCurrentMemUsage()
{
  mach_task_basic_info_data_t info;
  mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                (task_info_t)&info, &count) == KERN_SUCCESS) {
    return static_cast< size_t >(info.resident_size);
  }
  return 0;
}

inline size_t CoinGetTotalPhysicalMemory()
{
  int64_t mem = 0;
  size_t len = sizeof(mem);
  int mib[2] = { CTL_HW, HW_MEMSIZE };
  if (sysctl(mib, 2, &mem, &len, NULL, 0) == 0 && mem > 0)
    return static_cast< size_t >(mem);
  return 0;
}

#else
// Linux and other systems providing /proc/self/statm and sysconf().

#include <cstdio>
#include <unistd.h>

inline size_t CoinGetCurrentMemUsage()
{
  FILE *fp = fopen("/proc/self/statm", "r");
  if (!fp)
    return 0;
  long residentPages = 0;
  // Fields are: size resident shared text lib data dt (all in pages).
  int matched = fscanf(fp, "%*ld%ld", &residentPages);
  fclose(fp);
  if (matched != 1 || residentPages < 0)
    return 0;
  long pageSize = sysconf(_SC_PAGESIZE);
  if (pageSize <= 0)
    return 0;
  return static_cast< size_t >(residentPages) * static_cast< size_t >(pageSize);
}

inline size_t CoinGetTotalPhysicalMemory()
{
#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGESIZE)
  long pages = sysconf(_SC_PHYS_PAGES);
  long pageSize = sysconf(_SC_PAGESIZE);
  if (pages > 0 && pageSize > 0)
    return static_cast< size_t >(pages) * static_cast< size_t >(pageSize);
#endif
  return 0;
}

#endif

#endif

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
