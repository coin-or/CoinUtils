// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef CoinWarmStart_H
#define CoinWarmStart_H

//#############################################################################

/** Abstract base class for warm start information.

    Really nothing can be generalized for warm start information --- all we
    know is that it exists. Hence the abstract base class contains only a
    virtual destructor and a virtual clone function (a virtual constructor),
    so that derived classes can provide these functions.
*/

class CoinWarmStart {
public:
  virtual ~CoinWarmStart() {} ;

  virtual CoinWarmStart *clone() const = 0 ;
};

#endif
