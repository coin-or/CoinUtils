// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <cassert>

#include "CoinHelperFunctions.hpp"
#include "CoinIndexedVector.hpp"
//#############################################################################

void
CoinIndexedVector::clear()
{
  int i;
  for (i=0;i<nElements_;i++) {
    elements_[indices_[i]]=0.0;
  }
  nElements_ = 0;
  clearBase();
  delete [] packedElements_;
  packedElements_=NULL;
}

//#############################################################################

void
CoinIndexedVector::empty()
{
  delete [] indices_;
  indices_=NULL;
  delete [] elements_;
  elements_=NULL;
  nElements_ = 0;
  capacity_=0;
  clearBase();
  delete [] packedElements_;
  packedElements_=NULL;
}

//#############################################################################

CoinIndexedVector &
CoinIndexedVector::operator=(const CoinIndexedVector & rhs)
{
  if (this != &rhs) {
    clear();
    gutsOfSetVector(rhs.capacity_,rhs.nElements_, rhs.indices_, rhs.elements_,
      CoinPackedVectorBase::testForDuplicateIndex(),
      "operator=");
  }
  return *this;
}

//#############################################################################

CoinIndexedVector &
CoinIndexedVector::operator=(const CoinPackedVectorBase & rhs)
{
  if (this != &rhs) {
    clear();
    gutsOfSetVector(rhs.getNumElements(), rhs.getIndices(), rhs.getElements(),
      CoinPackedVectorBase::testForDuplicateIndex(),
      "operator= from base");
  }
  return *this;
}

//#############################################################################

void
CoinIndexedVector::borrowVector(int size, int numberIndices, int* inds, double* elems)
{
  empty();
  capacity_=size;
  nElements_ = numberIndices;
  indices_ = inds;  
  elements_ = elems;
  
  // whole point about borrowvector is that it is lightweight so no testing is done
}

//#############################################################################

void
CoinIndexedVector::returnVector()
{
  indices_=NULL;
  elements_=NULL;
  nElements_ = 0;
  capacity_=0;
  clearBase();
  delete [] packedElements_;
  packedElements_=NULL;
  // and clear index set
  clearIndexSet();
}

//#############################################################################

void
CoinIndexedVector::setVector(int size, const int * inds, const double * elems,
                            bool testForDuplicateIndex)
{
  clear();
  gutsOfSetVector(size, inds, elems, testForDuplicateIndex, "setVector");
}
//#############################################################################


void 
CoinIndexedVector::setVector(int size, int numberIndices, const int * inds, const double * elems)
{
  clear();
  gutsOfSetVector(size, numberIndices, inds, elems, false, "setVector");
}
//#############################################################################

void
CoinIndexedVector::setConstant(int size, const int * inds, double value,
                              bool testForDuplicateIndex)
{
  clear();
  gutsOfSetConstant(size, inds, value, testForDuplicateIndex, "setConstant");
}

//#############################################################################

void
CoinIndexedVector::setFull(int size, const double * elems,
                          bool testForDuplicateIndex) 
{
  // Clear out any values presently stored
  clear();
  CoinPackedVectorBase::setTestForDuplicateIndex(testForDuplicateIndex);
  // and clear index set
  clearIndexSet();
  
  if (size<0)
    throw CoinError("negative number of indices", "setFull", "CoinIndexedVector");
  
  reserve(size);
  nElements_ = 0;
  // elements_ array is all zero
  int i;
  for (i=0;i<size;i++) {
    int indexValue=i;
    if (fabs(elems[i])>=COIN_INDEXED_TINY_ELEMENT) {
      elements_[indexValue]=elems[i];
      indices_[nElements_++]=indexValue;
    }
  }
}
//#############################################################################

/** Access the i'th element of the full storage vector.  */
double &
CoinIndexedVector::operator[](int index) const
{
  if ( index >= capacity_ ) 
    throw CoinError("index >= capacity()", "[]", "CoinIndexedVector");
  if ( index < 0 ) 
    throw CoinError("index < 0" , "[]", "CoinIndexedVector");
  double * where = elements_ + index;
  return *where;
  
}
//#############################################################################

void
CoinIndexedVector::setElement(int index, double element)
{
  if ( index >= nElements_ ) 
    throw CoinError("index >= size()", "setElement", "CoinIndexedVector");
  if ( index < 0 ) 
    throw CoinError("index < 0" , "setElement", "CoinIndexedVector");
  elements_[indices_[index]] = element;
}

//#############################################################################

void
CoinIndexedVector::insert( int index, double element )
{
  if ( index < 0 ) 
    throw CoinError("index < 0" , "setElement", "CoinIndexedVector");
  if (index >= capacity_)
    reserve(index+1);
  if (elements_[index])
    throw CoinError("Index already exists", "insert", "CoinIndexedVector");
  indices_[nElements_++] = index;
  elements_[index] = element;
  // and clear index set
  clearIndexSet();
  delete [] packedElements_;
  packedElements_=NULL;
}

//#############################################################################

void
CoinIndexedVector::add( int index, double element )
{
  if ( index < 0 ) 
    throw CoinError("index < 0" , "setElement", "CoinIndexedVector");
  if (index >= capacity_)
    reserve(index+1);
  if (elements_[index]) {
    element += elements_[index];
    if (fabs(element)>= COIN_INDEXED_TINY_ELEMENT) {
      elements_[index] = element;
    } else {
      elements_[index] = 1.0e-100;
    }
  } else if (fabs(element)>= COIN_INDEXED_TINY_ELEMENT) {
    indices_[nElements_++] = index;
    elements_[index] = element;
   }
  // and clear index set
  clearIndexSet();
  delete [] packedElements_;
  packedElements_=NULL;
}

//#############################################################################

void
CoinIndexedVector::stopQuickAdd( )
{
  // clear index set
  clearIndexSet();
  delete [] packedElements_;
  packedElements_=NULL;
}

//#############################################################################

int
CoinIndexedVector::clean( double tolerance )
{
  int number = nElements_;
  int i;
  nElements_=0;
  for (i=0;i<number;i++) {
    int indexValue = indices_[i];
    if (fabs(elements_[indexValue])>=tolerance) {
      indices_[nElements_++]=indexValue;
    } else {
      elements_[indexValue]=0.0;
    }
  }
  // and clear index set
  clearIndexSet();
  delete [] packedElements_;
  packedElements_=NULL;
  return nElements_;
}

//#############################################################################

// For debug check vector is clear i.e. no elements
void CoinIndexedVector::checkClear()
{
  assert(!nElements_);
  int i;
  for (i=0;i<capacity_;i++) {
    assert(!elements_[i]);
  }
}
// For debug check vector is clean i.e. elements match indices
void CoinIndexedVector::checkClean()
{
  double * copy = new double[capacity_];
  CoinDisjointCopyN(elements_,capacity_,copy);
  int i;
  for (i=0;i<nElements_;i++) {
    int indexValue = indices_[i];
    copy[indexValue]=0.0;
  }
  for (i=0;i<capacity_;i++) {
    assert(!copy[i]);
  }
  delete [] copy;
}

//#############################################################################

void
CoinIndexedVector::append(const CoinPackedVectorBase & caboose) 
{
  const int cs = caboose.getNumElements();
  
  const int * cind = caboose.getIndices();
  const double * celem = caboose.getElements();
  int maxIndex=-1;
  int i;
  for (i=0;i<cs;i++) {
    int indexValue = cind[i];
    if (indexValue<0)
      throw CoinError("negative index", "append", "CoinIndexedVector");
    if (maxIndex<indexValue)
      maxIndex = indexValue;
  }
  reserve(maxIndex+1);
  bool needClean=false;
  int numberDuplicates=0;
  for (i=0;i<cs;i++) {
    int indexValue=cind[i];
    if (elements_[indexValue]) {
      numberDuplicates++;
      elements_[indexValue] += celem[i] ;
      if (fabs(elements_[indexValue])<COIN_INDEXED_TINY_ELEMENT) 
	needClean=true; // need to go through again
    } else {
      if (fabs(celem[i])>=COIN_INDEXED_TINY_ELEMENT) {
	elements_[indexValue]=celem[i];
	indices_[nElements_++]=indexValue;
      }
    }
  }
  if (needClean) {
    // go through again
    int size=nElements_;
    nElements_=0;
    for (i=0;i<size;i++) {
      int indexValue=indices_[i];
      double value=elements_[indexValue];
      if (fabs(value)>=COIN_INDEXED_TINY_ELEMENT) {
	indices_[nElements_++]=indexValue;
      } else {
        elements_[indexValue]=0.0;
      }
    }
  }
  // and clear index set
  clearIndexSet();
  delete [] packedElements_;
  packedElements_=NULL;
  if (numberDuplicates &&  testForDuplicateIndex())
    throw CoinError("duplicate index", "append", "CoinIndexedVector");
}

//#############################################################################

void
CoinIndexedVector::swap(int i, int j) 
{
  if ( i >= nElements_ ) 
    throw CoinError("index i >= size()","swap","CoinIndexedVector");
  if ( i < 0 ) 
    throw CoinError("index i < 0" ,"swap","CoinIndexedVector");
  if ( j >= nElements_ ) 
    throw CoinError("index j >= size()","swap","CoinIndexedVector");
  if ( j < 0 ) 
    throw CoinError("index j < 0" ,"swap","CoinIndexedVector");
  
  // Swap positions i and j of the
  // indices array
  
  int isave = indices_[i];
  indices_[i] = indices_[j];
  indices_[j] = isave;
}

//#############################################################################

void
CoinIndexedVector::truncate( int n ) 
{
  reserve(n);
}

//#############################################################################

void
CoinIndexedVector::operator+=(double value) 
{
  int i,indexValue;
  for (i=0;i<nElements_;i++) {
    indexValue = indices_[i];
    elements_[indexValue] += value;
  }
}

//-----------------------------------------------------------------------------

void
CoinIndexedVector::operator-=(double value) 
{
  int i,indexValue;
  for (i=0;i<nElements_;i++) {
    indexValue = indices_[i];
    elements_[indexValue] -= value;
  }
}

//-----------------------------------------------------------------------------

void
CoinIndexedVector::operator*=(double value) 
{
  int i,indexValue;
  for (i=0;i<nElements_;i++) {
    indexValue = indices_[i];
    elements_[indexValue] *= value;
  }
}

//-----------------------------------------------------------------------------

void
CoinIndexedVector::operator/=(double value) 
{
  int i,indexValue;
  for (i=0;i<nElements_;i++) {
    indexValue = indices_[i];
    elements_[indexValue] /= value;
  }
}
//#############################################################################

void
CoinIndexedVector::reserve(int n) 
{
  int i;
  // don't make allocated space smaller but do take off values
  if ( n < capacity_ ) {
    if (n<0) 
      throw CoinError("negative capacity", "reserve", "CoinIndexedVector");
    
    int nNew=0;
    for (i=0;i<nElements_;i++) {
      int indexValue=indices_[i];
      if (indexValue<n) {
        indices_[nNew++]=indexValue;
      } else {
        elements_[indexValue]=0.0;
      }
    }
    nElements_=nNew;
  } else if (n>capacity_) {
    
    // save pointers to existing data
    int * tempIndices = indices_;
    double * tempElements = elements_;
    
    // allocate new space
    indices_ = new int [n];
    elements_ = new double [n];
    
    // copy data to new space
    // and zero out part of array
    if (nElements_ > 0) {
      CoinDisjointCopyN(tempIndices, nElements_, indices_);
      CoinDisjointCopyN(tempElements, capacity_, elements_);
      CoinFillN(elements_+capacity_,n-capacity_,0.0);
    } else {
      CoinFillN(elements_,n,0.0);
    }
    capacity_ = n;
    
    // free old data
    delete [] tempElements;
    delete [] tempIndices;
  }
  // and clear index set
  clearIndexSet();
  delete [] packedElements_;
  packedElements_=NULL;
}

//#############################################################################

CoinIndexedVector::CoinIndexedVector (bool testForDuplicateIndex) :
CoinPackedVectorBase(),
indices_(NULL),
elements_(NULL),
nElements_(0),
packedElements_(NULL),
capacity_(0)
{
  // This won't fail, the indexed vector is empty. There can't be duplicate
  // indices.
  CoinPackedVectorBase::setTestForDuplicateIndex(testForDuplicateIndex);
}

//-----------------------------------------------------------------------------

CoinIndexedVector::CoinIndexedVector(int size,
                                   const int * inds, const double * elems,
                                   bool testForDuplicateIndex) :
CoinPackedVectorBase(),
indices_(NULL),
elements_(NULL),
nElements_(0),
packedElements_(NULL),
capacity_(0)
{
  gutsOfSetVector(size, inds, elems, testForDuplicateIndex,
    "constructor for array value");
}

//-----------------------------------------------------------------------------

CoinIndexedVector::CoinIndexedVector(int size,
                                   const int * inds, double value,
                                   bool testForDuplicateIndex) :
CoinPackedVectorBase(),
indices_(NULL),
elements_(NULL),
nElements_(0),
packedElements_(NULL),
capacity_(0)
{
  gutsOfSetConstant(size, inds, value, testForDuplicateIndex,
    "constructor for constant value");
}

//-----------------------------------------------------------------------------

CoinIndexedVector::CoinIndexedVector(int size, const double * element,
                                   bool testForDuplicateIndex) :
CoinPackedVectorBase(),
indices_(NULL),
elements_(NULL),
nElements_(0),
packedElements_(NULL),
capacity_(0)
{
  setFull(size, element, testForDuplicateIndex);
}

//-----------------------------------------------------------------------------

CoinIndexedVector::CoinIndexedVector(const CoinPackedVectorBase & rhs) :
CoinPackedVectorBase(),
indices_(NULL),
elements_(NULL),
nElements_(0),
packedElements_(NULL),
capacity_(0)
{  
  gutsOfSetVector(rhs.getNumElements(), rhs.getIndices(), rhs.getElements(),
    rhs.testForDuplicateIndex(), "copy constructor from base");
}

//-----------------------------------------------------------------------------

CoinIndexedVector::CoinIndexedVector(const CoinIndexedVector & rhs) :
CoinPackedVectorBase(),
indices_(NULL),
elements_(NULL),
nElements_(0),
packedElements_(NULL),
capacity_(0)
{  
  gutsOfSetVector(rhs.capacity_,rhs.nElements_, rhs.indices_, rhs.elements_,
    rhs.testForDuplicateIndex(), "copy constructor");
}

//-----------------------------------------------------------------------------

CoinIndexedVector::~CoinIndexedVector ()
{
  delete [] indices_;
  delete [] packedElements_;
  delete [] elements_;
}
//#############################################################################

// Get element values
const double * 
CoinIndexedVector::getElements() const 
{
  if (!packedElements_)
    packedElements_ = new double[nElements_];
  int i;
  for (i=0;i<nElements_;i++) {
    int indexValue=indices_[i];
    packedElements_[i]=elements_[indexValue];
  }
  return packedElements_;
}

double * 
CoinIndexedVector::getElements()  
{
  if (!packedElements_)
    packedElements_ = new double[nElements_];
  int i;
  for (i=0;i<nElements_;i++) {
    int indexValue=indices_[i];
    packedElements_[i]=elements_[indexValue];
  }
  return packedElements_;
}
//#############################################################################

/// Return the sum of two indexed vectors
CoinIndexedVector 
CoinIndexedVector::operator+(
                            const CoinIndexedVector& op2)
{
  int i;
  int nElements=nElements_;
  int capacity = CoinMax(capacity_,op2.capacity_);
  CoinIndexedVector newOne(*this);
  newOne.reserve(capacity);
  bool needClean=false;
  // new one now can hold everything so just modify old and add new
  for (i=0;i<op2.nElements_;i++) {
    int indexValue=op2.indices_[i];
    double value=op2.elements_[indexValue];
    double oldValue=elements_[indexValue];
    if (!oldValue) {
      if (fabs(value)>=COIN_INDEXED_TINY_ELEMENT) {
	newOne.elements_[indexValue]=value;
	newOne.indices_[nElements++]=indexValue;
      }
    } else {
      value += oldValue;
      newOne.elements_[indexValue]=value;
      if (fabs(value)<COIN_INDEXED_TINY_ELEMENT) {
	needClean=true;
      }
    }
  }
  newOne.nElements_=nElements;
  if (needClean) {
    // go through again
    nElements_=0;
    for (i=0;i<nElements;i++) {
      int indexValue=newOne.indices_[i];
      double value=newOne.elements_[indexValue];
      if (fabs(value)>=COIN_INDEXED_TINY_ELEMENT) {
	newOne.indices_[nElements_++]=indexValue;
      } else {
        newOne.elements_[indexValue]=0.0;
      }
    }
  }
  return newOne;
}

/// Return the difference of two indexed vectors
CoinIndexedVector 
CoinIndexedVector::operator-(
                            const CoinIndexedVector& op2)
{
  int i;
  int nElements=nElements_;
  int capacity = CoinMax(capacity_,op2.capacity_);
  CoinIndexedVector newOne(*this);
  newOne.reserve(capacity);
  bool needClean=false;
  // new one now can hold everything so just modify old and add new
  for (i=0;i<op2.nElements_;i++) {
    int indexValue=op2.indices_[i];
    double value=op2.elements_[indexValue];
    double oldValue=elements_[indexValue];
    if (!oldValue) {
      if (fabs(value)>=COIN_INDEXED_TINY_ELEMENT) {
	newOne.elements_[indexValue]=-value;
	newOne.indices_[nElements++]=indexValue;
      }
    } else {
      value = oldValue-value;
      newOne.elements_[indexValue]=value;
      if (fabs(value)<COIN_INDEXED_TINY_ELEMENT) {
	needClean=true;
      }
    }
  }
  newOne.nElements_=nElements;
  if (needClean) {
    // go through again
    nElements_=0;
    for (i=0;i<nElements;i++) {
      int indexValue=newOne.indices_[i];
      double value=newOne.elements_[indexValue];
      if (fabs(value)>=COIN_INDEXED_TINY_ELEMENT) {
	newOne.indices_[nElements_++]=indexValue;
      } else {
        newOne.elements_[indexValue]=0.0;
      }
    }
  }
  return newOne;
}

/// Return the element-wise product of two indexed vectors
CoinIndexedVector 
CoinIndexedVector::operator*(
                            const CoinIndexedVector& op2)
{
  int i;
  int nElements=nElements_;
  int capacity = CoinMax(capacity_,op2.capacity_);
  CoinIndexedVector newOne(*this);
  newOne.reserve(capacity);
  bool needClean=false;
  // new one now can hold everything so just modify old and add new
  for (i=0;i<op2.nElements_;i++) {
    int indexValue=op2.indices_[i];
    double value=op2.elements_[indexValue];
    double oldValue=elements_[indexValue];
    if (oldValue) {
      value *= oldValue;
      newOne.elements_[indexValue]=value;
      if (fabs(value)<COIN_INDEXED_TINY_ELEMENT) {
	needClean=true;
      }
    }
  }

// I don't see why this is necessary. Multiplication cannot add new values.
//newOne.nElements_=nElements;
  assert(newOne.nElements_ == nElements) ;

  if (needClean) {
    // go through again
    nElements_=0;
    for (i=0;i<nElements;i++) {
      int indexValue=newOne.indices_[i];
      double value=newOne.elements_[indexValue];
      if (fabs(value)>=COIN_INDEXED_TINY_ELEMENT) {
	newOne.indices_[nElements_++]=indexValue;
      } else {
        newOne.elements_[indexValue]=0.0;
      }
    }
  }
  return newOne;
}

/// Return the element-wise ratio of two indexed vectors
CoinIndexedVector 
CoinIndexedVector::operator/ (const CoinIndexedVector& op2) 
{
  // I am treating 0.0/0.0 as 0.0
  int i;
  int nElements=nElements_;
  int capacity = CoinMax(capacity_,op2.capacity_);
  CoinIndexedVector newOne(*this);
  newOne.reserve(capacity);
  bool needClean=false;
  // new one now can hold everything so just modify old and add new
  for (i=0;i<op2.nElements_;i++) {
    int indexValue=op2.indices_[i];
    double value=op2.elements_[indexValue];
    double oldValue=elements_[indexValue];
    if (oldValue) {
      if (!value)
        throw CoinError("zero divisor", "/", "CoinIndexedVector");
      value = oldValue/value;
      newOne.elements_[indexValue]=value;
      if (fabs(value)<COIN_INDEXED_TINY_ELEMENT) {
	needClean=true;
      }
    }
  }

// I don't see why this is necessary. Division can only modify existing.
//newOne.nElements_=nElements;
  assert(newOne.nElements_ == nElements) ;

  if (needClean) {
    // go through again
    nElements_=0;
    for (i=0;i<nElements;i++) {
      int indexValue=newOne.indices_[i];
      double value=newOne.elements_[indexValue];
      if (fabs(value)>=COIN_INDEXED_TINY_ELEMENT) {
	newOne.indices_[nElements_++]=indexValue;
      } else {
        newOne.elements_[indexValue]=0.0;
      }
    }
  }
  return newOne;
}
//#############################################################################

void 
CoinIndexedVector::sortDecrIndex()
{ 
  double * elements = getElements();
  CoinSort_2(indices_, indices_ + nElements_, elements,
    CoinFirstGreater_2<int, double>());
}

void 
CoinIndexedVector::sortIncrElement()
{ 
  double * elements = getElements();
  CoinSort_2(elements, elements + nElements_, indices_,
    CoinFirstLess_2<double, int>());
}

void 
CoinIndexedVector::sortDecrElement()
{ 
  double * elements = getElements();
  CoinSort_2(elements, elements + nElements_, indices_,
    CoinFirstGreater_2<double, int>());
}

//#############################################################################

void
CoinIndexedVector::gutsOfSetVector(int size,
                                  const int * inds, const double * elems,
                                  bool testForDuplicateIndex,
                                  const char * method) 
{
  // we are going to do a faster test for duplicates so test base class when empty
  CoinPackedVectorBase::setTestForDuplicateIndex(testForDuplicateIndex);
  // and clear index set
  clearIndexSet();
  
  if (size<0)
    throw CoinError("negative number of indices", method, "CoinIndexedVector");
  
  // find largest
  int i;
  int maxIndex=-1;
  for (i=0;i<size;i++) {
    int indexValue = inds[i];
    if (indexValue<0)
      throw CoinError("negative index", method, "CoinIndexedVector");
    if (maxIndex<indexValue)
      maxIndex = indexValue;
  }
  reserve(maxIndex+1);
  nElements_ = 0;
  // elements_ array is all zero
  bool needClean=false;
  int numberDuplicates=0;
  for (i=0;i<size;i++) {
    int indexValue=inds[i];
    if (elements_[indexValue] == 0)
    {
      if (fabs(elems[i])>=COIN_INDEXED_TINY_ELEMENT) {
	indices_[nElements_++]=indexValue;
	elements_[indexValue]=elems[i];
      }
    }
    else
    {
      numberDuplicates++;
      elements_[indexValue] += elems[i] ;
      if (fabs(elements_[indexValue])<COIN_INDEXED_TINY_ELEMENT) 
	needClean=true; // need to go through again
    }
  }
  if (needClean) {
    // go through again
    size=nElements_;
    nElements_=0;
    for (i=0;i<size;i++) {
      int indexValue=indices_[i];
      double value=elements_[indexValue];
      if (fabs(value)>=COIN_INDEXED_TINY_ELEMENT) {
	indices_[nElements_++]=indexValue;
      } else {
        elements_[indexValue]=0.0;
      }
    }
  }
  if (numberDuplicates &&  testForDuplicateIndex)
    throw CoinError("duplicate index", "setVector", "CoinIndexedVector");
}

//#############################################################################

void
CoinIndexedVector::gutsOfSetVector(int size, int numberIndices, 
                                  const int * inds, const double * elems,
                                  bool testForDuplicateIndex,
                                  const char * method) 
{
  // we are not going to test for duplicates so test base class when empty
  CoinPackedVectorBase::setTestForDuplicateIndex(testForDuplicateIndex);
  // and clear index set
  clearIndexSet();
  
  int i;
  reserve(size);
  if (numberIndices<0)
    throw CoinError("negative number of indices", method, "CoinIndexedVector");
  nElements_ = 0;
  // elements_ array is all zero
  bool needClean=false;
  int numberDuplicates=0;
  for (i=0;i<numberIndices;i++) {
    int indexValue=inds[i];
    if (indexValue<0) 
      throw CoinError("negative index", method, "CoinIndexedVector");
    else if (indexValue>=size) 
      throw CoinError("too large an index", method, "CoinIndexedVector");
    if (elements_[indexValue]) {
      numberDuplicates++;
      elements_[indexValue] += elems[indexValue] ;
      if (fabs(elements_[indexValue])<COIN_INDEXED_TINY_ELEMENT) 
	needClean=true; // need to go through again
    } else {
      if (fabs(elems[indexValue])>=COIN_INDEXED_TINY_ELEMENT) {
	elements_[indexValue]=elems[indexValue];
	indices_[nElements_++]=indexValue;
      }
    }
  }
  if (needClean) {
    // go through again
    size=nElements_;
    nElements_=0;
    for (i=0;i<size;i++) {
      int indexValue=indices_[i];
      double value=elements_[indexValue];
      if (fabs(value)>=COIN_INDEXED_TINY_ELEMENT) {
	indices_[nElements_++]=indexValue;
      } else {
        elements_[indexValue]=0.0;
      }
    }
  }
  if (numberDuplicates &&  testForDuplicateIndex)
    throw CoinError("duplicate index", "setVector", "CoinIndexedVector");
}

//-----------------------------------------------------------------------------

void
CoinIndexedVector::gutsOfSetConstant(int size,
                                    const int * inds, double value,
                                    bool testForDuplicateIndex,
                                    const char * method) 
{

  // we are going to do a faster test for duplicates so test base class
  // when empty
  CoinPackedVectorBase::setTestForDuplicateIndex(testForDuplicateIndex);
  // and clear index set
  clearIndexSet();
  
  if (size<0)
    throw CoinError("negative number of indices", method, "CoinIndexedVector");
  
  // find largest
  int i;
  int maxIndex=-1;
  for (i=0;i<size;i++) {
    int indexValue = inds[i];
    if (indexValue<0)
      throw CoinError("negative index", method, "CoinIndexedVector");
    if (maxIndex<indexValue)
      maxIndex = indexValue;
  }
  
  reserve(maxIndex+1);
  nElements_ = 0;
  int numberDuplicates=0;
  // elements_ array is all zero
  bool needClean=false;
  for (i=0;i<size;i++) {
    int indexValue=inds[i];
    if (elements_[indexValue] == 0)
    {
      if (fabs(value)>=COIN_INDEXED_TINY_ELEMENT) {
	elements_[indexValue] += value;
	indices_[nElements_++]=indexValue;
      }
    }
    else
    {
      numberDuplicates++;
      elements_[indexValue] += value ;
      if (fabs(elements_[indexValue])<COIN_INDEXED_TINY_ELEMENT) 
	needClean=true; // need to go through again
    }
  }
  if (needClean) {
    // go through again
    size=nElements_;
    nElements_=0;
    for (i=0;i<size;i++) {
      int indexValue=indices_[i];
      double value=elements_[indexValue];
      if (fabs(value)>=COIN_INDEXED_TINY_ELEMENT) {
	indices_[nElements_++]=indexValue;
      } else {
        elements_[indexValue]=0.0;
      }
    }
  }
  if (numberDuplicates &&  testForDuplicateIndex)
    throw CoinError("duplicate index", "setConstant", "CoinIndexedVector");
}

//#############################################################################
