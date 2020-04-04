/* $Id$ */
/*
  Copyright (C) 2002, 2003 International Business Machines Corporation
  and others.  All Rights Reserved.

  This code is licensed under the terms of the Eclipse Public License (EPL).
*/
#ifndef CoinCDefine_H
#define CoinCDefine_H

/** This has #defines etc for the "C" interface to COIN-OR. */

#ifdef _MSC_VER
#define COINLINKAGE __stdcall
#define COINLINKAGE_CB __cdecl
#else
#define COINLINKAGE
#define COINLINKAGE_CB
#endif

#if COIN_BIG_INDEX == 0
typedef int CoinBigIndex;
#elif COIN_BIG_INDEX == 1
typedef long CoinBigIndex;
#else
typedef long long CoinBigIndex;
#endif

#endif /* CoinCDefine_H */
