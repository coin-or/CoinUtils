// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinUtilsConfig.h"
#include <cassert>

#include "CoinWarmStartBasis.hpp"
#include <cmath>
#include <iostream>

//#############################################################################

void 
CoinWarmStartBasis::setSize(int ns, int na) {
  delete[] structuralStatus_;
  delete[] artificialStatus_;
  // Round all so arrays multiple of 4
  int nint = (ns+15) >> 4;
  if (nint) {
    structuralStatus_ = new char[4*nint];
    memset (structuralStatus_, 0, (4*nint) * sizeof(char));
  } else {
    structuralStatus_ = NULL;
  }
  nint = (na+15) >> 4;
  if (nint) {
    artificialStatus_ = new char[4*nint];
    memset (artificialStatus_, 0, (4*nint) * sizeof(char));
  } else {
    artificialStatus_ = NULL;
  }
  numArtificial_ = na;
  numStructural_ = ns;
}

void 
CoinWarmStartBasis::assignBasisStatus(int ns, int na, char*& sStat, 
				     char*& aStat) {
  delete[] structuralStatus_;
  delete[] artificialStatus_;
  numStructural_ = ns;
  numArtificial_ = na;
  structuralStatus_ = sStat;
  artificialStatus_ = aStat;
  sStat = NULL;
  aStat = NULL;
}
CoinWarmStartBasis::CoinWarmStartBasis(int ns, int na, 
				     const char* sStat, const char* aStat) :
  numStructural_(ns), numArtificial_(na),
  structuralStatus_(NULL), artificialStatus_(NULL) {
  // Round all so arrays multiple of 4
  int nint = ((ns+15) >> 4);
  if (nint > 0) {
      structuralStatus_ = new char[4*nint];
      structuralStatus_[4*nint-3]=0;
      structuralStatus_[4*nint-2]=0;
      structuralStatus_[4*nint-1]=0;
      memcpy (structuralStatus_, sStat, ((ns + 3) / 4) * sizeof(char));
  }
  nint = ((na+15) >> 4);
  if (nint > 0) {
      artificialStatus_ = new char[4*nint];
      artificialStatus_[4*nint-3]=0;
      artificialStatus_[4*nint-2]=0;
      artificialStatus_[4*nint-1]=0;
      memcpy (artificialStatus_, aStat, ((na + 3) / 4) * sizeof(char));
  }
}

CoinWarmStartBasis::CoinWarmStartBasis(const CoinWarmStartBasis& ws) :
  numStructural_(ws.numStructural_), numArtificial_(ws.numArtificial_),
  structuralStatus_(NULL), artificialStatus_(NULL) {
  // Round all so arrays multiple of 4
  int nint = (numStructural_+15) >> 4;
  if (nint > 0) {
      structuralStatus_ = new char[4*nint];
      memcpy (structuralStatus_, ws.structuralStatus_, 
	      (4*nint) * sizeof(char));
  }
  nint = (numArtificial_+15) >> 4;
  if (nint > 0) {
      artificialStatus_ = new char[4*nint];
      memcpy (artificialStatus_, ws.artificialStatus_, 
	      (4*nint) * sizeof(char));
  }
}

CoinWarmStartBasis& 
CoinWarmStartBasis::operator=(const CoinWarmStartBasis& rhs)
{
  if (this != &rhs) {
    numStructural_=rhs.numStructural_;
    numArtificial_=rhs.numArtificial_;
    delete [] structuralStatus_;
    delete [] artificialStatus_;
    // Round all so arrays multiple of 4
    int nint = (numStructural_+15) >> 4;
    if (nint > 0) {
	structuralStatus_ = new char[4*nint + 4];
	memcpy (structuralStatus_, rhs.structuralStatus_, 
		(4*nint) * sizeof(char));
    } else {
	structuralStatus_ = NULL;
    }
    nint = (numArtificial_+15) >> 4;
    if (nint > 0) {
	artificialStatus_ = new char[4*nint + 4];
	memcpy (artificialStatus_, rhs.artificialStatus_, 
		(4*nint) * sizeof(char));
    } else {
	artificialStatus_ = NULL;
    }
  }
  return *this;
}

// Resizes 
void 
CoinWarmStartBasis::resize (int newNumberRows, int newNumberColumns)
{
  char * array;
  int i , nCharNew, nCharOld;
  if (newNumberRows!=numArtificial_) {
    nCharOld  = 4*((numArtificial_+15)>>4);
    nCharNew  = 4*((newNumberRows+15)>>4);
    array = new char[nCharNew];
    // zap all for clarity and zerofault etc
    memset(array,0,nCharNew*sizeof(char));
    memcpy(array,artificialStatus_,(nCharOld>nCharNew)?nCharNew:nCharOld);
    delete [] artificialStatus_;
    artificialStatus_ = array;
    for (i=numArtificial_;i<newNumberRows;i++) 
      setArtifStatus(i, basic);
    numArtificial_ = newNumberRows;
  }
  if (newNumberColumns!=numStructural_) {
    nCharOld  = 4*((numStructural_+15)>>4);
    nCharNew  = 4*((newNumberColumns+15)>>4);
    array = new char[nCharNew];
    // zap all for clarity and zerofault etc
    memset(array,0,nCharNew*sizeof(char));
    memcpy(array,structuralStatus_,(nCharOld>nCharNew)?nCharNew:nCharOld);
    delete [] structuralStatus_;
    structuralStatus_ = array;
    for (i=numStructural_;i<newNumberColumns;i++) 
      setStructStatus(i, atLowerBound);
    numStructural_ = newNumberColumns;
  }
}

/*
  compressRows takes an ascending list of target indices without duplicates
  and removes them, compressing the artificialStatus_ array in place. It will
  fail spectacularly if the indices are not sorted. Use deleteRows if you
  need to preprocess the target indices to satisfy the conditions.
*/
void CoinWarmStartBasis::compressRows (int tgtCnt, const int *tgts)
{ 
  int i,keep,t,blkStart,blkEnd ;
  // basis may be null or smaller than members of list
  for (t = 0 ; t < tgtCnt ; t++) {
    if (tgts[t]>=numArtificial_) {
      // get rid of bad ones
      tgtCnt = t;
      break;
    }
  }
  if (tgtCnt <= 0) return ;
  Status stati ;

# ifdef COIN_DEBUG
/*
  If we're debugging, scan to see if we're deleting nonbasic artificials.
  (In other words, are we deleting tight constraints?) Easiest to just do this
  up front as opposed to integrating it with the loops below.
*/
  int nbCnt = 0 ;
  for (t = 0 ; t < tgtCnt ; t++)
  { i = tgts[t] ;
    stati = getStatus(artificialStatus_,i) ;
    if (stati != CoinWarmStartBasis::basic)
    { nbCnt++ ; } }
  if (nbCnt > 0)
  { std::cout << nbCnt << " nonbasic artificials deleted." << std::endl ; }
# endif

/*
  Preserve all entries before the first target. Skip across consecutive
  target indices to establish the start of the first block to be retained.
*/
  keep = tgts[0] ;
  for (t = 0 ; t < tgtCnt-1 && tgts[t]+1 == tgts[t+1] ; t++) ;
  blkStart = tgts[t]+1 ;
/*
  Outer loop works through the indices to be deleted. Inner loop copies runs
  of indices to keep.
*/
  while (t < tgtCnt-1)
  { blkEnd = tgts[t+1]-1 ;
    for (i = blkStart ; i <= blkEnd ; i++)
    { stati = getStatus(artificialStatus_,i) ;
      setStatus(artificialStatus_,keep++,stati) ; }
    for (t++ ; t < tgtCnt-1 && tgts[t]+1 == tgts[t+1] ; t++) ;
    blkStart = tgts[t]+1 ; }
/*
  Finish off by copying from last deleted index to end of status array.
*/
  for (i = blkStart ; i < numArtificial_ ; i++)
  { stati = getStatus(artificialStatus_,i) ;
    setStatus(artificialStatus_,keep++,stati) ; }

  numArtificial_ -= tgtCnt ;

  return ; }

#define CWSB_NEW_DELETE
#ifdef CWSB_NEW_DELETE
/*
  deleteRows takes an unordered list of target indices with duplicates and
  removes them from the basis. The strategy is to preprocesses the list into
  an ascending list without duplicates, suitable for compressRows.
*/
void 
CoinWarmStartBasis::deleteRows (int rawTgtCnt, const int *rawTgts)
{ if (rawTgtCnt <= 0) return ;

  int *tgts = new int[rawTgtCnt] ;
  memcpy(tgts,rawTgts,rawTgtCnt*sizeof(int)) ;
  int *first = &tgts[0] ;
  int *last = &tgts[rawTgtCnt] ;
  int *endUnique ;
  std::sort(first,last) ;
  endUnique = std::unique(first,last) ;
  int tgtCnt = endUnique-first ;
  compressRows(tgtCnt,tgts) ;
  delete [] tgts ;

  return  ; }

#else
/*
  Original model. Keep around until it's clear new version is cross-platform
  safe.
*/
// Deletes rows
void 
CoinWarmStartBasis::deleteRows(int number, const int * which)
{
  int i ;
  char * deleted = new char[numArtificial_];
  int numberDeleted=0;
  memset(deleted,0,numArtificial_*sizeof(char));
  for (i=0;i<number;i++) {
    int j = which[i];
    if (j>=0&&j<numArtificial_&&!deleted[j]) {
      numberDeleted++;
      deleted[j]=1;
    }
  }
  int nCharNew  = 4*((numArtificial_-numberDeleted+15)>>4);
  char * array = new char[nCharNew];
# ifdef ZEROFAULT
  memset(array,0,(nCharNew*sizeof(char))) ;
# endif
  int put=0;
# ifdef COIN_DEBUG
  int numberNotBasic=0;
# endif
  for (i=0;i<numArtificial_;i++) {
    Status status = getArtifStatus(i);
    if (!deleted[i]) {
      setStatus(array,put,status) ;
      put++; }
#   ifdef COIN_DEBUG
    else
    if (status!=CoinWarmStartBasis::basic)
    { numberNotBasic++ ; }
#   endif
  }
  delete [] artificialStatus_;
  artificialStatus_ = array;
  delete [] deleted;
  numArtificial_ -= numberDeleted;
# ifdef COIN_DEBUG
  if (numberNotBasic)
    std::cout<<numberNotBasic<<" non basic artificials deleted"<<std::endl;
# endif
}
#endif
// Deletes columns
void 
CoinWarmStartBasis::deleteColumns(int number, const int * which)
{
  int i ;
  char * deleted = new char[numStructural_];
  int numberDeleted=0;
  memset(deleted,0,numStructural_*sizeof(char));
  for (i=0;i<number;i++) {
    int j = which[i];
    if (j>=0&&j<numStructural_&&!deleted[j]) {
      numberDeleted++;
      deleted[j]=1;
    }
  }
  int nCharNew  = 4*((numStructural_-numberDeleted+15)>>4);
  char * array = new char[nCharNew];
# ifdef ZEROFAULT
  memset(array,0,(nCharNew*sizeof(char))) ;
# endif
  int put=0;
# ifdef COIN_DEBUG
  int numberBasic=0;
# endif
  for (i=0;i<numStructural_;i++) {
    Status status = getStructStatus(i);
    if (!deleted[i]) {
      setStatus(array,put,status) ;
      put++; }
#   ifdef COIN_DEBUG
    else
    if (status==CoinWarmStartBasis::basic)
    { numberBasic++ ; }
#   endif
  }
  delete [] structuralStatus_;
  structuralStatus_ = array;
  delete [] deleted;
  numStructural_ -= numberDeleted;
#ifdef COIN_DEBUG
  if (numberBasic)
    std::cout<<numberBasic<<" basic structurals deleted"<<std::endl;
#endif
}
// Prints in readable format (for debug)
void 
CoinWarmStartBasis::print() const
{
  std::cout<<"Basis "<<this<<" has "<<numArtificial_<<" rows and "
	   <<numStructural_<<" columns"<<std::endl;
  std::cout<<"Rows:"<<std::endl;
  int i;
  char type[]={'F','B','U','L'};

  for (i=0;i<numArtificial_;i++) 
    std::cout<<type[getArtifStatus(i)];
  std::cout<<std::endl;
  std::cout<<"Columns:"<<std::endl;

  for (i=0;i<numStructural_;i++) 
    std::cout<<type[getStructStatus(i)];
  std::cout<<std::endl;
}
CoinWarmStartBasis::CoinWarmStartBasis()
{
  
  numStructural_ = 0;
  numArtificial_ = 0;
  structuralStatus_ = NULL;
  artificialStatus_ = NULL;
}
CoinWarmStartBasis::~CoinWarmStartBasis()
{
  delete[] structuralStatus_;
  delete[] artificialStatus_;
}
// Returns number of basic structurals
int
CoinWarmStartBasis::numberBasicStructurals() const
{
  int i ;
  int numberBasic=0;
  for (i=0;i<numStructural_;i++) {
    Status status = getStructStatus(i);
    if (status==CoinWarmStartBasis::basic) 
      numberBasic++;
  }
  return numberBasic;
}

/*
  Generate a diff that'll convert oldCWS into the basis pointed to by this.

  This routine is a bit of a hack, for efficiency's sake. Rather than work
  with individual status vector entries, we're going to treat the vectors as
  int's --- in effect, we create one diff entry for each block of 16 status
  entries. Diffs for logicals are tagged with 0x80000000.
*/

CoinWarmStartDiff*
CoinWarmStartBasis::generateDiff (const CoinWarmStart *const oldCWS) const
{ 
/*
  Make sure the parameter is CoinWarmStartBasis or derived class.
*/
  const CoinWarmStartBasis *oldBasis =
      dynamic_cast<const CoinWarmStartBasis *>(oldCWS) ;
  if (!oldBasis)
  { throw CoinError("Old basis not derived from CoinWarmStartBasis.",
		    "generateDiff","CoinWarmStartBasis") ; }
  const CoinWarmStartBasis *newBasis = this ;
/*
  Make sure newBasis is equal or bigger than oldBasis. Calculate the worst case
  number of diffs and allocate vectors to hold them.
*/
  const int oldArtifCnt = oldBasis->getNumArtificial() ;
  const int oldStructCnt = oldBasis->getNumStructural() ;
  const int newArtifCnt = newBasis->getNumArtificial() ;
  const int newStructCnt = newBasis->getNumStructural() ;

  assert(newArtifCnt >= oldArtifCnt) ;
  assert(newStructCnt >= oldStructCnt) ;

  int sizeOldArtif = (oldArtifCnt+15)>>4 ;
  int sizeNewArtif = (newArtifCnt+15)>>4 ;
  int sizeOldStruct = (oldStructCnt+15)>>4 ;
  int sizeNewStruct = (newStructCnt+15)>>4 ;
  int maxBasisLength = sizeNewArtif+sizeNewStruct ;

  unsigned int *diffNdx = new unsigned int [maxBasisLength]; 
  unsigned int *diffVal = new unsigned int [maxBasisLength]; 
/*
  Ok, setup's over. Now scan the logicals (aka artificials, standing in for
  constraints). For the portion of the status arrays which overlap, create
  diffs. Then add any additional status from newBasis.

  I removed the following bit of code & comment:

    if (sizeNew == sizeOld) sizeOld--; // make sure all taken

  I assume this is meant to trap cases where oldBasis does not occupy all of
  the final int, but I can't see where it's necessary.
*/
  const unsigned int *oldStatus =
      reinterpret_cast<const unsigned int *>(oldBasis->getArtificialStatus()) ;
  const unsigned int *newStatus = 
      reinterpret_cast<const unsigned int *>(newBasis->getArtificialStatus()) ;
  int numberChanged = 0 ;
  int i ;
  for (i = 0 ; i < sizeOldArtif ; i++)
  { if (oldStatus[i] != newStatus[i])
    { diffNdx[numberChanged] = i|0x80000000 ;
      diffVal[numberChanged++] = newStatus[i] ; } }
  for ( ; i < sizeNewArtif ; i++)
  { diffNdx[numberChanged] = i|0x80000000 ;
    diffVal[numberChanged++] = newStatus[i] ; }
/*
  Repeat for structural variables.
*/
  oldStatus =
      reinterpret_cast<const unsigned int *>(oldBasis->getStructuralStatus()) ;
  newStatus =
      reinterpret_cast<const unsigned int *>(newBasis->getStructuralStatus()) ;
  for (i = 0 ; i < sizeOldStruct ; i++)
  { if (oldStatus[i] != newStatus[i])
    { diffNdx[numberChanged] = i ;
      diffVal[numberChanged++] = newStatus[i] ; } }
  for ( ; i < sizeNewStruct ; i++)
  { diffNdx[numberChanged] = i ;
    diffVal[numberChanged++] = newStatus[i] ; }
/*
  Create the object of our desire.
*/
  CoinWarmStartBasisDiff *diff =
    new CoinWarmStartBasisDiff(numberChanged,diffNdx,diffVal) ;
/*
  Clean up and return.
*/
  delete[] diffNdx ;
  delete[] diffVal ;

  return (dynamic_cast<CoinWarmStartDiff *>(diff)) ; }


/*
  Apply a diff to the basis pointed to by this.  It's assumed that the
  allocated capacity of the basis is sufficiently large.
*/
void CoinWarmStartBasis::applyDiff (const CoinWarmStartDiff *const cwsdDiff)
{
/*
  Make sure we have a CoinWarmStartBasisDiff
*/
  const CoinWarmStartBasisDiff *diff =
    dynamic_cast<const CoinWarmStartBasisDiff *>(cwsdDiff) ;
  if (!diff)
  { throw CoinError("Diff not derived from CoinWarmStartBasisDiff.",
		    "applyDiff","CoinWarmStartBasis") ; }
/*
  Application is by straighforward replacement of words in the status arrays.
  Index entries for logicals (aka artificials) are tagged with 0x80000000.
*/
  const int numberChanges = diff->sze_ ;
  const unsigned int *diffNdxs = diff->diffNdxs_ ;
  const unsigned int *diffVals = diff->diffVals_ ;
  unsigned int *structStatus =
      reinterpret_cast<unsigned int *>(this->getStructuralStatus()) ;
  unsigned int *artifStatus =
      reinterpret_cast<unsigned int *>(this->getArtificialStatus()) ;

  for (int i = 0 ; i < numberChanges ; i++)
  { unsigned int diffNdx = diffNdxs[i] ;
    unsigned int diffVal = diffVals[i] ;
    if ((diffNdx&0x80000000) == 0)
    { structStatus[diffNdx] = diffVal ; }
    else
    { artifStatus[diffNdx&0x7fffffff] = diffVal ; } }

  return ; }



/* Routines for CoinWarmStartBasisDiff */

/*
  Constructor given diff data.
*/
CoinWarmStartBasisDiff::CoinWarmStartBasisDiff (int sze,
  const unsigned int *const diffNdxs, const unsigned int *const diffVals)
  : sze_(sze),
    diffNdxs_(0),
    diffVals_(0)

{ if (sze > 0)
  { diffNdxs_ = new unsigned int[sze] ;
    memcpy(diffNdxs_,diffNdxs,sze*sizeof(unsigned int)) ;
    diffVals_ = new unsigned int[sze] ;
    memcpy(diffVals_,diffVals,sze*sizeof(unsigned int)) ; }
  
  return ; }

/*
  Copy constructor.
*/

CoinWarmStartBasisDiff::CoinWarmStartBasisDiff
  (const CoinWarmStartBasisDiff &rhs)
  : sze_(rhs.sze_),
    diffNdxs_(0),
    diffVals_(0)
{ if (sze_ > 0)
  { diffNdxs_ = new unsigned int[sze_] ;
    memcpy(diffNdxs_,rhs.diffNdxs_,sze_*sizeof(unsigned int)) ;
    diffVals_ = new unsigned int[sze_] ;
    memcpy(diffVals_,rhs.diffVals_,sze_*sizeof(unsigned int)) ; }

  return ; }

/*
  Assignment --- for convenience when assigning objects containing
  CoinWarmStartBasisDiff objects.
*/
CoinWarmStartBasisDiff&
CoinWarmStartBasisDiff::operator= (const CoinWarmStartBasisDiff &rhs)

{ if (this != &rhs)
  { if (sze_ > 0)
    { delete[] diffNdxs_ ;
      delete[] diffVals_ ; }
    sze_ = rhs.sze_ ;
    if (sze_ > 0)
    { diffNdxs_ = new unsigned int[sze_] ;
      memcpy(diffNdxs_,rhs.diffNdxs_,sze_*sizeof(unsigned int)) ;
      diffVals_ = new unsigned int[sze_] ;
      memcpy(diffVals_,rhs.diffVals_,sze_*sizeof(unsigned int)) ; }
    else
    { diffNdxs_ = 0 ;
      diffVals_ = 0 ; } }
  
  return (*this) ; }

