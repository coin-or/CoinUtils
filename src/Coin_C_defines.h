/* $Id$ */
/*
  Copyright (C) 2002, 2003 International Business Machines Corporation
  and others.  All Rights Reserved.

  This code is licensed under the terms of the Eclipse Public License (EPL).
*/
#ifndef CoinCDefine_H
#define CoinCDefine_H


#if COIN_BIG_INDEX == 0
typedef int CoinBigIndex;
#elif COIN_BIG_INDEX == 1
typedef long CoinBigIndex;
#else
typedef long long CoinBigIndex;
#endif

#endif /* CoinCDefine_H */
