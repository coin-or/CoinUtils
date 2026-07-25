// Copyright (C) 2026, COIN-OR.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#ifndef CoinBuildInfo_H
#define CoinBuildInfo_H

#include "CoinUtilsConfig.h"

/** \brief Runtime queries for optional third-party packages that CoinUtils
    was compiled with.

    CoinUtils' own build configuration (zlib/bzip2/LAPACK detection) is
    recorded in an internal, non-installed header, so it isn't visible to
    downstream projects (Osi, Clp, Cgl, Cbc) that only see the public,
    installed CoinUtilsConfig.h. These small runtime accessors let those
    projects (e.g. Cbc's `-version` command) report which optional features
    are actually present in the CoinUtils library they are linked against.
*/
namespace CoinBuildInfo {

/// True if this CoinUtils build supports reading/writing gzip (.gz) files.
COINUTILSLIB_EXPORT bool hasZlib();

/// True if this CoinUtils build supports reading/writing bzip2 (.bz2) files.
COINUTILSLIB_EXPORT bool hasBzlib();

/// True if this CoinUtils build was linked against a LAPACK implementation.
COINUTILSLIB_EXPORT bool hasLapack();

} // namespace CoinBuildInfo

#endif
