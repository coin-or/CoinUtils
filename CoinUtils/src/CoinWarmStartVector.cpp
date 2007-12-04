// Copyright (C) 2003, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <cassert>

#include "CoinWarmStartVector.hpp"
#include <cmath>

//#############################################################################

/*
  Generate a `diff' that can convert the warm start passed as a parameter to
  the warm start specified by this.

  The capabilities are limited: the basis passed as a parameter can be no
  larger than the basis pointed to by this.
*/

CoinWarmStartDiff*
CoinWarmStartVector::generateDiff (const CoinWarmStart *const oldCWS) const
{ 
/*
  Make sure the parameter is CoinWarmStartVector or derived class.
*/
  const CoinWarmStartVector *oldVector =
      dynamic_cast<const CoinWarmStartVector *>(oldCWS) ;
  if (!oldVector)
  { throw CoinError("Old warm start not derived from CoinWarmStartVector.",
		    "generateDiff","CoinWarmStartVector") ; }
  const CoinWarmStartVector *newVector = this ;
/*
  Make sure newVector is equal or bigger than oldVector. Calculate the worst
  case number of diffs and allocate vectors to hold them.
*/
  const int oldCnt = oldVector->size() ;
  const int newCnt = newVector->size() ;

  assert(newCnt >= oldCnt) ;

  unsigned int *diffNdx = new unsigned int [newCnt]; 
  double *diffVal = new double [newCnt]; 
/*
  Scan the vector vectors.  For the portion of the vectors which overlap,
  create diffs. Then add any additional entries from newVector.
*/
  const double *oldVal = oldVector->values() ;
  const double *newVal = newVector->values() ;
  int numberChanged = 0 ;
  int i ;
  for (i = 0 ; i < oldCnt ; i++)
  { if (oldVal[i] != newVal[i])
    { diffNdx[numberChanged] = i ;
      diffVal[numberChanged++] = newVal[i] ; } }
  for ( ; i < newCnt ; i++)
  { diffNdx[numberChanged] = i ;
    diffVal[numberChanged++] = newVal[i] ; }
/*
  Create the object of our desire.
*/
  CoinWarmStartVectorDiff *diff =
    new CoinWarmStartVectorDiff(numberChanged,diffNdx,diffVal) ;
/*
  Clean up and return.
*/
  delete[] diffNdx ;
  delete[] diffVal ;

  return (dynamic_cast<CoinWarmStartDiff *>(diff)) ; }


/*
  Apply diff to this warm start.

  Update this warm start by applying diff. It's assumed that the
  allocated capacity of the warm start is sufficiently large.
*/

void CoinWarmStartVector::applyDiff (const CoinWarmStartDiff *const cwsdDiff)
{
/*
  Make sure we have a CoinWarmStartVectorDiff
*/
  const CoinWarmStartVectorDiff *diff =
    dynamic_cast<const CoinWarmStartVectorDiff *>(cwsdDiff) ;
  if (!diff)
  { throw CoinError("Diff not derived from CoinWarmStartVectorDiff.",
		    "applyDiff","CoinWarmStartVector") ; }
/*
  Application is by straighforward replacement of words in the vector vector.
*/
  const int numberChanges = diff->sze_ ;
  const unsigned int *diffNdxs = diff->diffNdxs_ ;
  const double *diffVals = diff->diffVals_ ;
  double *vals = this->values_ ;

  for (int i = 0 ; i < numberChanges ; i++)
  { unsigned int diffNdx = diffNdxs[i] ;
    double diffVal = diffVals[i] ;
    vals[diffNdx] = diffVal ; }

  return ; }


//#############################################################################


// Assignment

CoinWarmStartVectorDiff&
CoinWarmStartVectorDiff::operator= (const CoinWarmStartVectorDiff &rhs)

{ if (this != &rhs)
  { if (sze_ > 0)
    { delete[] diffNdxs_ ;
      delete[] diffVals_ ; }
    sze_ = rhs.sze_ ;
    if (sze_ > 0)
    { diffNdxs_ = new unsigned int[sze_] ;
      memcpy(diffNdxs_,rhs.diffNdxs_,sze_*sizeof(unsigned int)) ;
      diffVals_ = new double[sze_] ;
      memcpy(diffVals_,rhs.diffVals_,sze_*sizeof(double)) ; }
    else
    { diffNdxs_ = 0 ;
      diffVals_ = 0 ; } }

  return (*this) ; }


// Copy constructor

CoinWarmStartVectorDiff::CoinWarmStartVectorDiff (const CoinWarmStartVectorDiff &rhs)
  : sze_(rhs.sze_),
    diffNdxs_(0),
    diffVals_(0)
{ if (sze_ > 0)
  { diffNdxs_ = new unsigned int[sze_] ;
    memcpy(diffNdxs_,rhs.diffNdxs_,sze_*sizeof(unsigned int)) ;
    diffVals_ = new double[sze_] ;
    memcpy(diffVals_,rhs.diffVals_,sze_*sizeof(double)) ; }
  
  return ; }


/// Standard constructor

CoinWarmStartVectorDiff::CoinWarmStartVectorDiff
  (int sze, const unsigned int *const diffNdxs, const double *const diffVals)
  : sze_(sze),
    diffNdxs_(0),
    diffVals_(0)
{ if (sze > 0)
  { diffNdxs_ = new unsigned int[sze] ;
    memcpy(diffNdxs_,diffNdxs,sze*sizeof(unsigned int)) ;
    diffVals_ = new double[sze] ;
    memcpy(diffVals_,diffVals,sze*sizeof(double)) ; }
  
  return ; }

