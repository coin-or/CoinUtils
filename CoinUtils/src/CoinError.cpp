/* $Id$ */
// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinError.hpp"

bool COINUTILSLIB_EXPORT CoinError::printErrors_ = false;

/** A function to block the popup windows that windows creates when the code
    crashes */
#ifdef HAVE_WINDOWS_H
#include <windows.h>
COINUTILSLIB_EXPORT
void WindowsErrorPopupBlocker()
{
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
}
#else
COINUTILSLIB_EXPORT
void WindowsErrorPopupBlocker() {}
#endif
