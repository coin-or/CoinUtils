// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef CoinWarmStartVector_H
#define CoinWarmStartVector_H

#include "CoinHelperFunctions.hpp"
#include "CoinWarmStart.hpp"


//#############################################################################

/** WarmStart information that is only a vector */

class CoinWarmStartVector : public CoinWarmStart {
protected:
   inline void gutsOfDestructor() {
      delete[] values_;
   }
   inline void gutsOfCopy(const CoinWarmStartVector& rhs) {
      size_  = rhs.size_;
      values_ = new double[size_];
      CoinDisjointCopyN(rhs.values_, size_, values_);
   }

public:
   /// return the size of the vector
   int size() const { return size_; }
   /// return a pointer to the array of vectors
   const double * values() const { return values_; }

   /** Assign the vector to be the warmstart information. In this method
       the object assumes ownership of the pointer and upon return #vector will
       be a NULL pointer. If copying is desirable use the constructor. */
   void assignVector(int size, double *& vec) {
      size_ = size;
      delete[] values_;
      values_ = vec;
      vec = NULL;
   }

   CoinWarmStartVector() : size_(0), values_(NULL) {}

   CoinWarmStartVector(int size, const double * vec) :
      size_(size), values_(new double[size]) {
      CoinDisjointCopyN(vec, size, values_);
   }

   CoinWarmStartVector(const CoinWarmStartVector& rhs) {
      gutsOfCopy(rhs);
   }

   CoinWarmStartVector& operator=(const CoinWarmStartVector& rhs) {
      if (this != &rhs) {
	 gutsOfDestructor();
	 gutsOfCopy(rhs);
      }
      return *this;
   }

   inline void swap(CoinWarmStartVector& rhs) {
       if (this != &rhs) {
	   std::swap(size_, rhs.size_);
	   std::swap(values_, rhs.values_);
       }
   }

   /** `Virtual constructor' */
   virtual CoinWarmStart *clone() const {
      return new CoinWarmStartVector(*this);
   }
     
   virtual ~CoinWarmStartVector() {
      gutsOfDestructor();
   }

/*! \name Vector warm start `diff' methods */
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
   /// the size of the vector
   int size_;
   /// the vector itself
   double * values_;
   //@}
};

//#############################################################################

/*! \class CoinWarmStartVectorDiff
    \brief A `diff' between two CoinWarmStartVector objects

  This class exists in order to hide from the world the details of
  calculating and representing a `diff' between two CoinWarmStartVector
  objects. For convenience, assignment, cloning, and deletion are visible to
  the world, and default and copy constructors are made available to derived
  classes.  Knowledge of the rest of this structure, and of generating and
  applying diffs, is restricted to the friend functions
  CoinWarmStartVector::generateDiff() and CoinWarmStartVector::applyDiff().

  The actual data structure is a pair of vectors, #diffNdxs_ and #diffVals_.
    
*/

class CoinWarmStartVectorDiff : public CoinWarmStartDiff
{
  friend CoinWarmStartDiff*
    CoinWarmStartVector::generateDiff(const CoinWarmStart *const oldCWS) const;
  friend void
    CoinWarmStartVector::applyDiff(const CoinWarmStartDiff *const diff) ;

public:

  /*! \brief `Virtual constructor' */
  virtual CoinWarmStartDiff *
  clone() const { return new CoinWarmStartVectorDiff(*this) ; }

  /*! \brief Assignment */
  virtual CoinWarmStartVectorDiff &
  operator= (const CoinWarmStartVectorDiff &rhs) ;

  /*! \brief Destructor */
  virtual ~CoinWarmStartVectorDiff()
  { delete[] diffNdxs_ ;
    delete[] diffVals_ ; }

  inline void swap(CoinWarmStartVectorDiff& rhs) {
      if (this != &rhs) {
	  std::swap(sze_, rhs.sze_);
	  std::swap(diffNdxs_, rhs.diffNdxs_);
	  std::swap(diffVals_, rhs.diffVals_);
       }
   }

  /*! \brief Default constructor
  */
  CoinWarmStartVectorDiff () : sze_(0), diffNdxs_(0), diffVals_(NULL) {} 

  /*! \brief Copy constructor
  
    For convenience when copying objects containing CoinWarmStartVectorDiff
    objects. But consider whether you should be using #clone() to retain
    polymorphism.
  */
  CoinWarmStartVectorDiff (const CoinWarmStartVectorDiff &rhs) ;

  /*! \brief Standard constructor */
  CoinWarmStartVectorDiff (int sze, const unsigned int *const diffNdxs,
			 const double *const diffVals) ;
  
private:

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
