// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef CoinWarmStartDual_H
#define CoinWarmStartDual_H

#include "CoinHelperFunctions.hpp"
#include "CoinWarmStart.hpp"

//#############################################################################

/** WarmStart information that is only a dual vector */

class CoinWarmStartDual : public CoinWarmStart {
protected:
   inline void gutsOfDestructor() {
      delete[] dualVector_;
   }
   inline void gutsOfCopy(const CoinWarmStartDual& rhs) {
      dualSize_  = rhs.dualSize_;
      dualVector_ = new double[dualSize_];
      CoinDisjointCopyN(rhs.dualVector_, dualSize_, dualVector_);
   }

public:
   /// return the size of the dual vector
   int size() const { return dualSize_; }
   /// return a pointer to the array of duals
   const double * dual() const { return dualVector_; }

   /** Assign the dual vector to be the warmstart information. In this method
       the object assumes ownership of the pointer and upon return "dual" will
       be a NULL pointer. If copying is desirable use the constructor. */
   void assignDual(int size, double *& dual) {
      dualSize_ = size;
      delete[] dualVector_;
      dualVector_ = dual;
      dual = NULL;
   }

   CoinWarmStartDual() : dualSize_(0), dualVector_(NULL) {}
   CoinWarmStartDual(int size, const double * dual) :
      dualSize_(size), dualVector_(new double[size]) {
      CoinDisjointCopyN(dual, size, dualVector_);
   }
   CoinWarmStartDual(const CoinWarmStartDual& rhs) {
      gutsOfCopy(rhs);
   }
   CoinWarmStartDual& operator=(const CoinWarmStartDual& rhs) {
      if (this != &rhs) {
	 gutsOfDestructor();
	 gutsOfCopy(rhs);
      }
      return *this;
   }
   /** `Virtual constructor' */
   virtual CoinWarmStart *clone() const {
      return new CoinWarmStartDual(*this);
   }
     
   virtual ~CoinWarmStartDual() {
      gutsOfDestructor();
   }

private:
   ///@name Private data members
   //@{
   /// the size of the dual vector
   int dualSize_;
   /// the dual vector itself
   double * dualVector_;
   //@}
};

#endif
