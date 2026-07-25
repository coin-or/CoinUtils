// Copyright (C) 2026, COIN-OR.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinUtilsConfig.h"
#include "CoinBuildInfo.hpp"

namespace CoinBuildInfo {

bool hasZlib()
{
#ifdef COINUTILS_HAS_ZLIB
  return true;
#else
  return false;
#endif
}

bool hasBzlib()
{
#ifdef COINUTILS_HAS_BZLIB
  return true;
#else
  return false;
#endif
}

bool hasLapack()
{
#ifdef COINUTILS_HAS_LAPACK
  return true;
#else
  return false;
#endif
}

} // namespace CoinBuildInfo
