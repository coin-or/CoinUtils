// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef CoinWarmStart_H
#define CoinWarmStart_H

//#############################################################################

/** Warmstart information abstract base class. <br>
    Really nothing can be generalized for warmstarting information. All we
    know that it exists. Hence the abstract base class contains only a virtual
    destructor. */

class CoinWarmStart {
public:
  virtual ~CoinWarmStart() {}
};

#endif
