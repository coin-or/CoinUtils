
/***************************************************************************/
/*           HERE DEFINE THE PROJECT SPECIFIC PUBLIC MACROS                */
/*    These are only in effect in a setting that doesn't use configure     */
/***************************************************************************/

/* Version number of project */
#define COINUTILS_VERSION "trunk"

/* Major Version number of project */
#define COINUTILS_VERSION_MAJOR 9999

/* Minor Version number of project */
#define COINUTILS_VERSION_MINOR 9999

/* Release Version number of project */
#define COINUTILS_VERSION_RELEASE 9999

/*
  Define to 64bit integer types. Note that MS does not provide __uint64.

  Microsoft defines types in BaseTsd.h, part of the Windows SDK. Given
  that this file only gets used in the Visual Studio environment, it
  seems to me we'll be better off simply including it and using the
  types MS defines. But since I have no idea of history here, I'll leave
  all of this inside the guard for MSC_VER >= 1200. If you're reading this
  and have been developing in MSVS long enough to know, fix it.  -- lh, 100915 --
*/
#if _MSC_VER >= 1200
#include <BaseTsd.h>
#define COIN_INT64_T INT64
#define COIN_UINT64_T UINT64
/* Define to integer type capturing pointer */
#define COIN_INTPTR_T ULONG_PTR
#else
#define COIN_INT64_T long long
#define COIN_UINT64_T unsigned long long
#define COIN_INTPTR_T int *
#endif

/* Define to 1 if CoinUtils uses C++11 */
#define COINUTILS_CPLUSPLUS11 1

/* Define to 1 if cstdint is available for CoinUtils */
#define COINUTILS_HAS_CSTDINT 1

/* Define to 1 if stdint.h is available for CoinUtils */
#define COINUTILS_HAS_STDINT_H 1

#ifndef COINUTILSLIB_EXPORT
#if defined(_WIN32) && defined(DLL_EXPORT)
#define COINUTILSLIB_EXPORT __declspec(dllimport)
#else
#define COINUTILSLIB_EXPORT
#endif
#endif

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
