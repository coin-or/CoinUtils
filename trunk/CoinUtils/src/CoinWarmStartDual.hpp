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

/*! \name Dual warm start `diff' methods */
//@{

  /*! \brief Generate a `diff' that can convert the warm start passed as a
	     parameter to the warm start specified by \c this.

    The capabilities are limited: the basis passed as a parameter can be no
    larger than the basis pointed to by \c this.
  */

  virtual CoinWarmStartDiff*
  generateDiff (const CoinWarmStart *const oldCWS) const ;

  /*! \brief Apply \p diff to this warm start.

    Update this warm start by applying \p diff. It's assumed that the
    allocated capacity of the warm start is sufficiently large.
  */

  virtual void applyDiff (const CoinWarmStartDiff *const cwsdDiff) ;

//@}

private:
   ///@name Private data members
   //@{
   /// the size of the dual vector
   int dualSize_;
   /// the dual vector itself
   double * dualVector_;
   //@}
};

//#############################################################################

/*! \class CoinWarmStartDualDiff
    \brief A `diff' between two CoinWarmStartDual objects

  This class exists in order to hide from the world the details of
  calculating and representing a `diff' between two CoinWarmStartDual
  objects. For convenience, assignment, cloning, and deletion are visible to
  the world, and default and copy constructors are made available to derived
  classes.  Knowledge of the rest of this structure, and of generating and
  applying diffs, is restricted to the friend functions
  CoinWarmStartDual::generateDiff() and CoinWarmStartDual::applyDiff().

  The actual data structure is a pair of vectors, #diffNdxs_ and #diffVals_.
    
*/

class CoinWarmStartDualDiff : public CoinWarmStartDiff
{ public:

  /*! \brief `Virtual constructor' */
  virtual CoinWarmStartDiff *clone() const
  { CoinWarmStartDualDiff *cwsdd =  new CoinWarmStartDualDiff(*this) ;
    return (dynamic_cast<CoinWarmStartDiff *>(cwsdd)) ; }

  /*! \brief Assignment */
  virtual CoinWarmStartDualDiff &operator= (const CoinWarmStartDualDiff &rhs) ;

  /*! \brief Destructor */
  virtual ~CoinWarmStartDualDiff()
  { delete[] diffNdxs_ ;
    delete[] diffVals_ ; }

  protected:

  /*! \brief Default constructor
  
    This is protected (rather than private) so that derived classes can
    see it when they make <i>their</i> default constructor protected or
    private.
  */
  CoinWarmStartDualDiff () : sze_(0), diffNdxs_(0), diffVals_(NULL) {} 

  /*! \brief Copy constructor
  
    For convenience when copying objects containing CoinWarmStartDualDiff
    objects. But consider whether you should be using #clone() to retain
    polymorphism.

    This is protected (rather than private) so that derived classes can
    see it when the make <i>their</i> copy constructor protected or
    private.
  */
  CoinWarmStartDualDiff (const CoinWarmStartDualDiff &rhs) ;

  private:

  friend CoinWarmStartDiff*
    CoinWarmStartDual::generateDiff(const CoinWarmStart *const oldCWS) const ;
  friend void
    CoinWarmStartDual::applyDiff(const CoinWarmStartDiff *const diff) ;

  /*! \brief Standard constructor */
  CoinWarmStartDualDiff (int sze, const unsigned int *const diffNdxs,
			 const double *const diffVals) ;
  
  /*!
      \brief Number of entries (and allocated capacity), in units of
	     \c double.
  */
  int sze_ ;

  /*! \brief Array of diff indices */

  unsigned int *diffNdxs_ ;

  /*! \brief Array of diff values */

  double *diffVals_ ;
} ;


#endif

