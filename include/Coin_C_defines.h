/* Copyright (C) 2002, 2003 International Business Machines
   Corporation and others.  All Rights Reserved.*/
#ifndef CoinCDefine_H
#define CoinCDefine_H

/** This has #defines etc for the "C" interface to Coin.

*/

/* Plus infinity */
#ifndef COIN_DBL_MAX
#define COIN_DBL_MAX DBL_MAX
#endif

/* We need to allow for Microsoft */
#ifndef COINLIBAPI
#if defined (CLPMSDLL)
/* for backward compatibility */
#define COINMSDLL
#endif
#if defined (COINMSDLL)
#   define COINLIBAPI __declspec(dllexport)
#   define COINLINKAGE  __stdcall
#   define COINLINKAGE_CB  __cdecl
#else
#   define COINLIBAPI 
#   define COINLINKAGE
#   define COINLINKAGE_CB 
#endif

#endif
/** User does not need to see structure of model */
typedef void Clp_Simplex;
/** typedef for user call back.
 The cvec are constructed so don't need to be const*/
typedef  void (COINLINKAGE_CB *clp_callback) (Clp_Simplex * model,int  msgno, int ndouble,
                            const double * dvec, int nint, const int * ivec,
                            int nchar, char ** cvec);
/** User does not need to see structure of model */
typedef void Sbb_Model;
/** typedef for user call back.
 The cvec are constructed so don't need to be const*/
typedef  void (COINLINKAGE_CB *clp_callback) (Sbb_Model * model,int  msgno, int ndouble,
                            const double * dvec, int nint, const int * ivec,
                            int nchar, char ** cvec);

#if COIN_BIG_INDEX==0
typedef int CoinBigIndex;
#elif COIN_BIG_INDEX==1
typedef long CoinBigIndex;
#else
typedef long long CoinBigIndex;
#endif
#endif
