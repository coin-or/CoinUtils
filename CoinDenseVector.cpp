// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Resized.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <cassert>
#include "CoinDenseVector.hpp"
#include "CoinHelperFunctions.hpp"

#define min(a,b)	( (a) < (b) ? (a) : (b) )
//#############################################################################

void
CoinDenseVector::clear()
{
   memset(elements_, 0, nElements_*sizeof(real));
}

//#############################################################################

CoinDenseVector &
CoinDenseVector::operator=(const CoinDenseVector & rhs)
{
   if (this != &rhs) {
     setVector(rhs.getNumElements(), rhs.getElements());
   }
   return *this;
}

//#############################################################################

void
CoinDenseVector::setVector(int size, const real * elems)
{
   resize(size);
   memcpy(elements_, elems, size*sizeof(real));
}

//#############################################################################

void
CoinDenseVector::setConstant(int size, real value)
{
   resize(size);
   for(int i=0; i<size; i++)
     elements_[i] = value;
}

//#############################################################################

void
CoinDenseVector::resize(int newsize, real value)
{
  if (newsize != nElements_){	
    assert(newsize > 0);
    real *newarray = new real[newsize];
    int cpysize = min(newsize, nElements_);
    memcpy(newarray, elements_, cpysize*sizeof(real));
    delete[] elements_;
    elements_ = newarray;
    nElements_ = newsize;
    for(int i=cpysize; i<newsize; i++)
      elements_[i] = value;
  }
}

//#############################################################################

void
CoinDenseVector::setElement(int index, real element)
{
  assert(index >= 0 && index < nElements_);
   elements_[index] = element;
}

//#############################################################################

void
CoinDenseVector::append(const CoinDenseVector & caboose)
{
   const int s = nElements_;
   const int cs = caboose.getNumElements();
   int newsize = s + cs;
   resize(newsize);
   const real * celem = caboose.getElements();
   CoinDisjointCopyN(celem, cs, elements_ + s);
}

//#############################################################################

void
CoinDenseVector::operator+=(real value) 
{
  for(int i=0; i<nElements_; i++)
    elements_[i] += value;
}

//-----------------------------------------------------------------------------

void
CoinDenseVector::operator-=(real value) 
{
  for(int i=0; i<nElements_; i++)
    elements_[i] -= value;
}

//-----------------------------------------------------------------------------

void
CoinDenseVector::operator*=(real value) 
{
  for(int i=0; i<nElements_; i++)
    elements_[i] *= value;
}

//-----------------------------------------------------------------------------

void
CoinDenseVector::operator/=(real value) 
{
  for(int i=0; i<nElements_; i++)
    elements_[i] /= value;
}

//#############################################################################

CoinDenseVector::CoinDenseVector():
   nElements_(0),
   elements_(NULL)
{}
  
//#############################################################################

CoinDenseVector::CoinDenseVector(int size, const real * elems):
   nElements_(0),
   elements_(NULL)
{
  gutsOfSetVector(size, elems);
}

//-----------------------------------------------------------------------------

CoinDenseVector::CoinDenseVector(int size, real value):
   nElements_(0),
   elements_(NULL)
{
  gutsOfSetConstant(size, value);
}

//-----------------------------------------------------------------------------

CoinDenseVector::CoinDenseVector(const CoinDenseVector & rhs):
   nElements_(0),
   elements_(NULL)
{
     setVector(rhs.getNumElements(), rhs.getElements());
}

//-----------------------------------------------------------------------------

CoinDenseVector::CoinDenseVector(const CoinDenseVector * &rhs):
   nElements_(0),
   elements_(NULL)
{
     setVector(rhs->getNumElements(), rhs->getElements());
}

//-----------------------------------------------------------------------------

CoinDenseVector::~CoinDenseVector ()
{
   delete [] elements_;
}

//#############################################################################

void
CoinDenseVector::gutsOfSetVector(int size, const real * elems)
{
   if ( size != 0 ) {
      resize(size);
      nElements_ = size;
      CoinDisjointCopyN(elems, size, elements_);
   }
}

//-----------------------------------------------------------------------------

void
CoinDenseVector::gutsOfSetConstant(int size, real value)
{
   if ( size != 0 ) {
      resize(size);
      nElements_ = size;
      CoinFillN(elements_, size, value);
   }
}

//#############################################################################
/** Access the i'th element of the dense vector.  */
real &
CoinDenseVector::operator[](int index) const
{
  assert(index >= 0 && index < nElements_);
  real *where = elements_ + index;
  return *where;
}
//#############################################################################
