// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
#ifndef CoinPragma_H
#define CoinPragma_H

//-------------------------------------------------------------------
//
// This is a file which can contain Pragma's that are
// generally applicable to any source file.
//
//-------------------------------------------------------------------

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
// Turn off compiler warning: 
// "empty controlled statement found; is this the intent?"
#  pragma warning(disable:4390)
#endif

#endif
