// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <algorithm>
#include <numeric>
#include <cassert>
#include <cstdio>
#include <iostream>

#include "CoinSort.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinPackedVectorBase.hpp"
#include "CoinPackedMatrix.hpp"
#include <stdio.h>

//#############################################################################

static inline int
CoinLengthWithExtra(int len, double extraGap)
{
  return static_cast<int>(ceil(len * (1 + extraGap)));
}

//#############################################################################

static inline void
CoinTestSortedIndexSet(const int num, const int * sorted, const int maxEntry,
		      const char * testingMethod)
{
   if (sorted[0] < 0 || sorted[num-1] >= maxEntry)
      throw CoinError("bad index", testingMethod, "CoinPackedMatrix");
   if (std::adjacent_find(sorted, sorted + num) != sorted + num)
      throw CoinError("duplicate index", testingMethod, "CoinPackedMatrix");
}

//-----------------------------------------------------------------------------

static inline int *
CoinTestIndexSet(const int numDel, const int * indDel, const int maxEntry,
		const char * testingMethod)
{
   if (! CoinIsSorted(indDel, indDel + numDel)) {
      // if not sorted then sort it, test for consistency and return a pointer
      // to the sorted array
      int * sorted = new int[numDel];
      CoinDisjointCopyN(indDel, numDel, sorted);
      std::sort(sorted, sorted + numDel);
      CoinTestSortedIndexSet(numDel, sorted, maxEntry, testingMethod);
      return sorted;
   }

   // Otherwise it's already sorted, so just test for consistency and return a
   // 0 pointer.
   CoinTestSortedIndexSet(numDel, indDel, maxEntry, testingMethod);
   return 0;
}

//#############################################################################

void
CoinPackedMatrix::reserve(const int newMaxMajorDim, const CoinBigIndex newMaxSize)
{
   if (newMaxMajorDim > maxMajorDim_) {
      maxMajorDim_ = newMaxMajorDim;
      int * oldlength = length_;
      CoinBigIndex * oldstart = start_;
      length_ = new int[newMaxMajorDim];
      start_ = new CoinBigIndex[newMaxMajorDim+1];
      if (majorDim_ > 0) {
	 CoinDisjointCopyN(oldlength, majorDim_, length_);
	 CoinDisjointCopyN(oldstart, majorDim_ + 1, start_);
      }
      delete[] oldlength;
      delete[] oldstart;
   }
   if (newMaxSize > maxSize_) {
      maxSize_ = newMaxSize;
      int * oldind = index_;
      double * oldelem = element_;
      index_ = new int[newMaxSize];
      element_ = new double[newMaxSize];
      for (int i = majorDim_ - 1; i >= 0; --i) {
	 CoinDisjointCopyN(oldind+start_[i], length_[i], index_+start_[i]);
	 CoinDisjointCopyN(oldelem+start_[i], length_[i], element_+start_[i]);
      }
      delete[] oldind;
      delete[] oldelem;
   }
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::clear()
{
   majorDim_ = 0;
   minorDim_ = 0;
   size_ = 0;
}

//#############################################################################
//#############################################################################

void
CoinPackedMatrix::setDimensions(int newnumrows, int newnumcols)
  throw(CoinError)
{
  const int numrows = getNumRows();
  if (newnumrows < 0)
    newnumrows = numrows;
  if (newnumrows < numrows)
    throw CoinError("Bad new rownum (less than current)",
		    "setDimensions", "CoinPackedMatrix");

  const int numcols = getNumCols();
  if (newnumcols < 0)
    newnumcols = numcols;
  if (newnumcols < numcols)
    throw CoinError("Bad new colnum (less than current)",
		    "setDimensions", "CoinPackedMatrix");

  int numplus = 0;
  if (isColOrdered()) {
    minorDim_ = newnumrows;
    numplus = newnumcols - numcols;
  } else {
    minorDim_ = newnumcols;
    numplus = newnumrows - numrows;
  }
#if 0
  if (numplus > 0) {
    int * lengths = new int[numplus];
    CoinFillN(lengths, numplus, 1);
    resizeForAddingMajorVectors(numplus, lengths);
    delete[] lengths;
  }
#else
  if (numplus > 0) {
    int* lengths = new int[numplus];
    CoinFillN(lengths, numplus, 0); //1 in the original version
    resizeForAddingMajorVectors(numplus, lengths);
    delete[] lengths;
    majorDim_ += numplus; //forgot to change majorDim_
  }
#endif

}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::setExtraGap(const double newGap) throw(CoinError)
{
   if (newGap < 0)
      throw CoinError("negative new extra gap",
		     "setExtraGap", "CoinPackedMatrix");
   extraGap_ = newGap;
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::setExtraMajor(const double newMajor) throw(CoinError)
{
   if (newMajor < 0)
      throw CoinError("negative new extra major",
		     "setExtraMajor", "CoinPackedMatrix");
   extraMajor_ = newMajor;
}

//#############################################################################

void
CoinPackedMatrix::appendCol(const CoinPackedVectorBase& vec) throw(CoinError)
{
   if (colOrdered_)
      appendMajorVector(vec);
   else
      appendMinorVector(vec);
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::appendCol(const int vecsize,
			   const int *vecind,
			   const double *vecelem) throw(CoinError)
{
   if (colOrdered_)
      appendMajorVector(vecsize, vecind, vecelem);
   else
      appendMinorVector(vecsize, vecind, vecelem);
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::appendCols(const int numcols,
			    const CoinPackedVectorBase * const * cols)
  throw(CoinError)
{
   if (colOrdered_)
      appendMajorVectors(numcols, cols);
   else
      appendMinorVectors(numcols, cols);
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::appendRow(const CoinPackedVectorBase& vec) throw(CoinError)
{
   if (colOrdered_)
      appendMinorVector(vec);
   else
      appendMajorVector(vec);
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::appendRow(const int vecsize,
			   const int *vecind,
			   const double *vecelem) throw(CoinError)
{
   if (colOrdered_)
      appendMinorVector(vecsize, vecind, vecelem);
   else
      appendMajorVector(vecsize, vecind, vecelem);
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::appendRows(const int numrows,
			    const CoinPackedVectorBase * const * rows)
  throw(CoinError)
{
   if (colOrdered_)
      appendMinorVectors(numrows, rows);
   else
      appendMajorVectors(numrows, rows);
}

//#############################################################################

void
CoinPackedMatrix::rightAppendPackedMatrix(const CoinPackedMatrix& matrix)
  throw(CoinError)
{
   if (colOrdered_) {
      if (matrix.colOrdered_) {
	 majorAppendSameOrdered(matrix);
      } else {
	 majorAppendOrthoOrdered(matrix);
      }
   } else {
      if (matrix.colOrdered_) {
	 minorAppendOrthoOrdered(matrix);
      } else {
	 minorAppendSameOrdered(matrix);
      }
   }
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::bottomAppendPackedMatrix(const CoinPackedMatrix& matrix)
  throw(CoinError)
{
   if (colOrdered_) {
      if (matrix.colOrdered_) {
	 minorAppendSameOrdered(matrix);
      } else {
	 minorAppendOrthoOrdered(matrix);
      }
   } else {
      if (matrix.colOrdered_) {
	 majorAppendOrthoOrdered(matrix);
      } else {
	 majorAppendSameOrdered(matrix);
      }
   }
}

//#############################################################################

void
CoinPackedMatrix::deleteCols(const int numDel, const int * indDel)
{
   if (colOrdered_)
      deleteMajorVectors(numDel, indDel);
   else
      deleteMinorVectors(numDel, indDel);
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::deleteRows(const int numDel, const int * indDel)
{
   if (colOrdered_)
      deleteMinorVectors(numDel, indDel);
   else
      deleteMajorVectors(numDel, indDel);
}

//#############################################################################
/* Replace the elements of a vector.  The indices remain the same.
   At most the number specified will be replaced.
   The index is between 0 and major dimension of matrix */
void 
CoinPackedMatrix::replaceVector(const int index,
			       const int numReplace, 
			       const double * newElements)
{
  if (index >= 0 && index < majorDim_) {
    int length = (length_[index] < numReplace) ? length_[index] : numReplace;
    CoinDisjointCopyN(newElements, length, element_ + start_[index]);
  } else {
#ifdef COIN_DEBUG
    throw CoinError("bad index", "replaceVector", "CoinPackedMatrix");
#endif
  }
}
//#############################################################################
/* Eliminate all elements in matrix whose 
   absolute value is less than threshold.
   The column starts are not affected.  Returns number of elements
   eliminated.  Elements eliminated are at end of each vector
*/
int 
CoinPackedMatrix::compress(double threshold)
{
  CoinBigIndex numberEliminated =0;
  // space for eliminated
  int * eliminatedIndex = new int[minorDim_];
  double * eliminatedElement = new double[minorDim_];
  int i;
  for (i=0;i<majorDim_;i++) {
    int length = length_[i];
    CoinBigIndex k=start_[i];
    int kbad=0;
    CoinBigIndex j;
    for (j=start_[i];j<start_[i]+length;j++) {
      if (fabs(element_[j])>=threshold) {
	element_[k]=element_[j];
	index_[k++]=index_[j];
      } else {
	eliminatedElement[kbad]=element_[j];
	eliminatedIndex[kbad++]=index_[j];
      }
    }
    if (kbad) {
      numberEliminated += kbad;
      length_[i] = k-start_[i];
      memcpy(index_+k,eliminatedIndex,kbad*sizeof(int));
      memcpy(element_+k,eliminatedElement,kbad*sizeof(double));
    }
  }
  size_ -= numberEliminated;
  delete [] eliminatedIndex;
  delete [] eliminatedElement;
  return numberEliminated;
}
//#############################################################################

void
CoinPackedMatrix::removeGaps()
{
   for (int i = 1; i < majorDim_; ++i) {
      const CoinBigIndex si = start_[i];
      const int li = length_[i];
      start_[i] = start_[i-1] + length_[i-1];
      CoinCopy(index_ + si, index_ + (si + li), index_ + start_[i]);
      CoinCopy(element_ + si, element_ + (si + li), element_ + start_[i]);
   }
   start_[majorDim_] = size_;
}

//#############################################################################

void
CoinPackedMatrix::submatrixOf(const CoinPackedMatrix& matrix,
			     const int numMajor, const int * indMajor)
   throw(CoinError)
{
   int i;

   int* sortedIndPtr = CoinTestIndexSet(numMajor, indMajor, matrix.majorDim_,
				       "submatrixOf");
   const int * sortedInd = sortedIndPtr == 0 ? indMajor : sortedIndPtr;

   gutsOfDestructor();

   // Count how many nonzeros there'll be
   CoinBigIndex nzcnt = 0;
   const int* length = matrix.getVectorLengths();
   for (i = 0; i < numMajor; ++i) {
      nzcnt += length[sortedInd[i]];
   }

   colOrdered_ = matrix.colOrdered_;
   maxMajorDim_ = int(numMajor * (1+extraMajor_) + 1);
   maxSize_ = int(nzcnt * (1+extraMajor_) * (1+extraGap_) + 100);
   length_ = new int[maxMajorDim_];
   start_ = new CoinBigIndex[maxMajorDim_+1];
   index_ = new int[maxSize_];
   element_ = new double[maxSize_];
   majorDim_ = 0;
   minorDim_ = matrix.minorDim_;
   size_ = 0;

   for (i = 0; i < numMajor; ++i) {
      appendMajorVector(matrix.getVector(sortedInd[i]));
   }

   delete[] sortedIndPtr;
}

//#############################################################################

void
CoinPackedMatrix::copyOf(const CoinPackedMatrix& rhs)
{
   if (this != &rhs) {
      gutsOfDestructor();
      gutsOfCopyOf(rhs.colOrdered_,
		   rhs.minorDim_, rhs.majorDim_, rhs.size_,
		   rhs.element_, rhs.index_, rhs.start_, rhs.length_,
		   rhs.extraMajor_, rhs.extraGap_);
   }
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::copyOf(const bool colordered,
			const int minor, const int major,
			const CoinBigIndex numels,
			const double * elem, const int * ind,
			const CoinBigIndex * start, const int * len,
			const double extraMajor, const double extraGap)
{
   gutsOfDestructor();
   gutsOfCopyOf(colordered, minor, major, numels, elem, ind, start, len,
		extraMajor, extraGap);
}

//#############################################################################

// This method is essentially the same as minorAppendOrthoOrdered(). However,
// since we start from an empty matrix, lots of fluff can be avoided.

void
CoinPackedMatrix::reverseOrderedCopyOf(const CoinPackedMatrix& rhs)
{
   if (this == &rhs) {
      reverseOrdering();
      return;
   }

   int i;
   colOrdered_ = !rhs.colOrdered_;
   majorDim_ = rhs.minorDim_;
   minorDim_ = rhs.majorDim_;
   size_ = rhs.size_;

   if (size_ == 0)
      return;

   // first compute how long each major-dimension vector will be
   // this trickery is needed because MSVC++ is not willing to delete[] a
   // 'const int *'
   int * orthoLengthPtr = rhs.countOrthoLength();
   const int * orthoLength = orthoLengthPtr;

   // Allocate sufficient space (resizeForAddingMinorVectors())
   
   const int newMaxMajorDim_ =
     CoinMax(maxMajorDim_, CoinLengthWithExtra(majorDim_, extraMajor_));

   if (newMaxMajorDim_ > maxMajorDim_) {
      maxMajorDim_ = newMaxMajorDim_;
      delete[] start_;
      delete[] length_;
      start_ = new CoinBigIndex[maxMajorDim_ + 1];
      length_ = new int[maxMajorDim_];
   }

   start_[0] = 0;
   if (extraGap_ == 0) {
      for (i = 0; i < majorDim_; ++i)
	 start_[i+1] = start_[i] + orthoLength[i];
   } else {
      const double eg = extraGap_;
      for (i = 0; i < majorDim_; ++i)
	start_[i+1] = start_[i] + CoinLengthWithExtra(orthoLength[i], eg);
   }

   const CoinBigIndex newMaxSize =
      CoinMax(maxSize_, getLastStart()+ (CoinBigIndex) extraMajor_);

   if (newMaxSize > maxSize_) {
      maxSize_ = newMaxSize;
      delete[] index_;
      delete[] element_;
      index_ = new int[maxSize_];
      element_ = new double[maxSize_];
   }

   delete[] orthoLengthPtr;

   // now insert the entries of matrix
   minorDim_ = 0;
   CoinFillN(length_, majorDim_, 0);
   for (i = 0; i < rhs.majorDim_; ++i) {
      const CoinBigIndex last = rhs.getVectorLast(i);
      for (CoinBigIndex j = rhs.getVectorFirst(i); j != last; ++j) {
	 const int ind = rhs.index_[j];
	 element_[start_[ind] + length_[ind]] = rhs.element_[j];
	 index_[start_[ind] + (length_[ind]++)] = minorDim_;
      }
      ++minorDim_;
   }
}
   
//#############################################################################

void
CoinPackedMatrix::assignMatrix(const bool colordered,
			      const int minor, const int major,
			      const CoinBigIndex numels,
			      double *& elem, int *& ind,
			      CoinBigIndex *& start, int *& len,
			      const int maxmajor, const CoinBigIndex maxsize)
{
   gutsOfDestructor();
   colOrdered_ = colordered;
   element_ = elem;
   index_ = ind;
   start_ = start;
   majorDim_ = major;
   minorDim_ = minor;
   size_ = numels;
   maxMajorDim_ = maxmajor != -1 ? maxmajor : major;
   maxSize_ = maxsize != -1 ? maxsize : numels;
   if (len == NULL) {
      length_ = new int[maxMajorDim_];
      std::adjacent_difference(start + 1, start + (major + 1), length_);
   } else {
      length_ = len;
   }
   elem = NULL;
   ind = NULL;
   start = NULL;
   len = NULL;
}

//#############################################################################

CoinPackedMatrix &
CoinPackedMatrix::operator=(const CoinPackedMatrix& rhs)
{
   if (this != &rhs) {
      gutsOfDestructor();
      gutsOfOpEqual(rhs.colOrdered_,
		    rhs.minorDim_,  rhs.majorDim_, rhs.size_,
		    rhs.element_, rhs.index_, rhs.start_, rhs.length_);
   }
   return *this;
}

//#############################################################################

void
CoinPackedMatrix::reverseOrdering()
{
   CoinPackedMatrix m;
   m.extraGap_ = extraMajor_;
   m.extraMajor_ = extraGap_;
   m.reverseOrderedCopyOf(*this);
   swap(m);
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::transpose()
{
   colOrdered_ = ! colOrdered_;
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::swap(CoinPackedMatrix& m)
{
   std::swap(colOrdered_,  m.colOrdered_);
   std::swap(extraGap_,	   m.extraGap_);
   std::swap(extraMajor_,  m.extraMajor_);
   std::swap(element_, 	   m.element_);
   std::swap(index_,	   m.index_);
   std::swap(start_,	   m.start_);
   std::swap(length_,	   m.length_);
   std::swap(majorDim_,	   m.majorDim_);
   std::swap(minorDim_,	   m.minorDim_);
   std::swap(size_,	   m.size_);
   std::swap(maxMajorDim_, m.maxMajorDim_);
   std::swap(maxSize_,     m.maxSize_);
}

//#############################################################################
//#############################################################################

void
CoinPackedMatrix::times(const double * x, double * y) const 
{
   if (colOrdered_)
      timesMajor(x, y);
   else
      timesMinor(x, y);
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::times(const CoinPackedVectorBase& x, double * y) const 
{
   if (colOrdered_)
      timesMajor(x, y);
   else
      timesMinor(x, y);
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::transposeTimes(const double * x, double * y) const 
{
   if (colOrdered_)
      timesMinor(x, y);
   else
      timesMajor(x, y);
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::transposeTimes(const CoinPackedVectorBase& x, double * y) const
{
   if (colOrdered_)
      timesMinor(x, y);
   else
      timesMajor(x, y);
}

//#############################################################################
//#############################################################################

int *
CoinPackedMatrix::countOrthoLength() const
{
   int * orthoLength = new int[minorDim_];
   CoinFillN(orthoLength, minorDim_, 0);
   for (int i = majorDim_ - 1; i >= 0; --i) {
      const CoinBigIndex last = getVectorLast(i);
      for (CoinBigIndex j = getVectorFirst(i); j != last; ++j) {
         assert( index_[j] < minorDim_ );
	 ++orthoLength[index_[j]];
      }
   }
   return orthoLength;
}

//#############################################################################
//#############################################################################

void
CoinPackedMatrix::appendMajorVector(const int vecsize,
				   const int *vecind,
				   const double *vecelem)
   throw(CoinError)
{
#ifdef COIN_DEBUG
  for (int i = 0; i < vecsize; ++i) {
    if (vecind[i] < 0 || vecind[i] >= minorDim_)
      throw CoinError("out of range index",
		     "appendMajorVector", "CoinPackedMatrix");
  }
#if 0   
  if (std::find_if(vecind, vecind + vecsize,
		   compose2(logical_or<bool>(),
			    bind2nd(less<int>(), 0),
			    bind2nd(greater_equal<int>(), minorDim_))) !=
      vecind + vecsize)
    throw CoinError("out of range index",
		   "appendMajorVector", "CoinPackedMatrix");
#endif
#endif
  
  if (majorDim_ == maxMajorDim_ || vecsize > maxSize_ - getLastStart()) {
    resizeForAddingMajorVectors(1, &vecsize);
  }

  // got to get this again since it might change!
  const CoinBigIndex last = getLastStart();

  // OK, now just append the major-dimension vector to the end

  length_[majorDim_] = vecsize;
  CoinDisjointCopyN(vecind, vecsize, index_ + last);
  CoinDisjointCopyN(vecelem, vecsize, element_ + last);
  if (majorDim_ == 0)
    start_[0] = 0;
  start_[majorDim_ + 1] =
    CoinMin(last + CoinLengthWithExtra(vecsize, extraGap_), maxSize_ );

   // LL: Do we want to allow appending a vector that has more entries than
   // the current size?
   if (vecsize > 0) {
     minorDim_ = CoinMax(minorDim_,
			(*std::max_element(vecind, vecind+vecsize)) + 1);
   }

   ++majorDim_;
   size_ += vecsize;
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::appendMajorVector(const CoinPackedVectorBase& vec)
   throw(CoinError)
{
   appendMajorVector(vec.getNumElements(),
		     vec.getIndices(), vec.getElements());
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::appendMajorVectors(const int numvecs,
				    const CoinPackedVectorBase * const * vecs)
   throw(CoinError)
{
  int i;
  CoinBigIndex nz = 0;
  for (i = 0; i < numvecs; ++i)
    nz += CoinLengthWithExtra(vecs[i]->getNumElements(), extraGap_);
  reserve(majorDim_ + numvecs, getLastStart() + nz);
  for (i = 0; i < numvecs; ++i)
    appendMajorVector(*vecs[i]);
}

//#############################################################################

void
CoinPackedMatrix::appendMinorVector(const int vecsize,
				   const int *vecind,
				   const double *vecelem)
   throw(CoinError)
{
  int i;
#ifdef COIN_DEBUG
  for (i = 0; i < vecsize; ++i) {
    if (vecind[i] < 0 || vecind[i] >= majorDim_)
      throw CoinError("out of range index",
		     "appendMinorVector", "CoinPackedMatrix");
  }
#if 0   
  if (std::find_if(vecind, vecind + vecsize,
		   compose2(logical_or<bool>(),
			    bind2nd(less<int>(), 0),
			    bind2nd(greater_equal<int>(), majorDim_))) !=
      vecind + vecsize)
    throw CoinError("out of range index",
		   "appendMinorVector", "CoinPackedMatrix");
#endif
#endif

  // test that there's a gap at the end of every major-dimension vector where
  // we want to add a new entry
   
  for (i = vecsize - 1; i >= 0; --i) {
    const int j = vecind[i];
    if (start_[j] + length_[j] == start_[j+1])
      break;
  }

  if (i >= 0) {
    int * addedEntries = new int[majorDim_];
    memset(addedEntries, 0, majorDim_ * sizeof(int));
    for (i = vecsize - 1; i >= 0; --i)
      addedEntries[vecind[i]] = 1;
    resizeForAddingMinorVectors(addedEntries);
    delete[] addedEntries;
  }

  // OK, now insert the entries of the minor-dimension vector
  for (i = vecsize - 1; i >= 0; --i) {
    const int j = vecind[i];
    const CoinBigIndex posj = start_[j] + (length_[j]++);
    index_[posj] = minorDim_;
    element_[posj] = vecelem[i];
  }

  ++minorDim_;
  size_ += vecsize;
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::appendMinorVector(const CoinPackedVectorBase& vec)
   throw(CoinError)
{
   appendMinorVector(vec.getNumElements(),
		     vec.getIndices(), vec.getElements());
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::appendMinorVectors(const int numvecs,
				    const CoinPackedVectorBase * const * vecs)
   throw(CoinError)
{
  if (numvecs == 0)
    return;

  int i;

  int * addedEntries = new int[majorDim_];
  CoinFillN(addedEntries, majorDim_, 0);
  for (i = numvecs - 1; i >= 0; --i) {
    const int vecsize = vecs[i]->getNumElements();
    const int* vecind = vecs[i]->getIndices();
    for (int j = vecsize - 1; j >= 0; --j) {
#ifdef COIN_DEBUG
      if (vecind[j] < 0 || vecind[j] >= majorDim_)
	throw CoinError("out of range index", "appendMinorVectors",
		       "CoinPackedMatrix");
#endif
      ++addedEntries[vecind[j]];
    }
  }
 
  for (i = majorDim_ - 1; i >= 0; --i) {
    if (start_[i] + length_[i] + addedEntries[i] > start_[i+1])
      break;
  }
  if (i >= 0)
    resizeForAddingMinorVectors(addedEntries);
  delete[] addedEntries;

  // now insert the entries of the vectors
  for (i = 0; i < numvecs; ++i) {
    const int vecsize = vecs[i]->getNumElements();
    const int* vecind = vecs[i]->getIndices();
    const double* vecelem = vecs[i]->getElements();
    for (int j = vecsize - 1; j >= 0; --j) {
      const int ind = vecind[j];
      element_[start_[ind] + length_[ind]] = vecelem[j];
      index_[start_[ind] + (length_[ind]++)] = minorDim_;
    }
    ++minorDim_;
    size_ += vecsize;
  }
}

//#############################################################################
//#############################################################################

void
CoinPackedMatrix::majorAppendSameOrdered(const CoinPackedMatrix& matrix)
   throw(CoinError)
{
   if (minorDim_ != matrix.minorDim_) {
      throw CoinError("dimension mismatch", "rightAppendSameOrdered",
		     "CoinPackedMatrix");
   }
   if (matrix.majorDim_ == 0)
      return;

   int i;
   if (majorDim_ + matrix.majorDim_ > maxMajorDim_ ||
       getLastStart() + matrix.getLastStart() > maxSize_) {
      // we got to resize before we add. note that the resizing method
      // properly fills out start_ and length_ for the major-dimension
      // vectors to be added!
      resizeForAddingMajorVectors(matrix.majorDim_, matrix.length_);
      start_ += majorDim_;
      for (i = 0; i < matrix.majorDim_; ++i) {
	 const int l = matrix.length_[i];
	 CoinDisjointCopyN(matrix.index_ + matrix.start_[i], l,
			   index_ + start_[i]);
	 CoinDisjointCopyN(matrix.element_ + matrix.start_[i], l,
			   element_ + start_[i]);
      }
      start_ -= majorDim_;
   } else {
      start_ += majorDim_;
      length_ += majorDim_;
      for (i = 0; i < matrix.majorDim_; ++i) {
	 const int l = matrix.length_[i];
	 CoinDisjointCopyN(matrix.index_ + matrix.start_[i], l,
			   index_ + start_[i]);
	 CoinDisjointCopyN(matrix.element_ + matrix.start_[i], l,
			   element_ + start_[i]);
	 start_[i+1] = start_[i] + matrix.start_[i+1] - matrix.start_[i];
	 length_[i] = l;
      }
      start_ -= majorDim_;
      length_ -= majorDim_;
   }
   majorDim_ += matrix.majorDim_;
   size_ += matrix.size_;
}

//-----------------------------------------------------------------------------
   
void
CoinPackedMatrix::minorAppendSameOrdered(const CoinPackedMatrix& matrix)
   throw(CoinError)
{
   if (majorDim_ != matrix.majorDim_) {
      throw CoinError("dimension mismatch", "bottomAppendSameOrdered",
		      "CoinPackedMatrix");
   }
   if (matrix.minorDim_ == 0)
      return;

   int i;
   for (i = majorDim_ - 1; i >= 0; --i) {
      if (start_[i] + length_[i] + matrix.length_[i] > start_[i+1])
	 break;
   }
   if (i >= 0)
      resizeForAddingMinorVectors(matrix.length_);

   // now insert the entries of matrix
   for (i = majorDim_ - 1; i >= 0; --i) {
      const int l = matrix.length_[i];
      std::transform(matrix.index_ + matrix.start_[i],
		matrix.index_ + (matrix.start_[i] + l),
		index_ + (start_[i] + length_[i]),
		std::bind2nd(std::plus<int>(), minorDim_));
      CoinDisjointCopyN(matrix.element_ + matrix.start_[i], l,
		       element_ + (start_[i] + length_[i]));
      length_[i] += l;
   }
   minorDim_ += matrix.minorDim_;
   size_ += matrix.size_;
}
   
//-----------------------------------------------------------------------------
   
void
CoinPackedMatrix::majorAppendOrthoOrdered(const CoinPackedMatrix& matrix)
   throw(CoinError)
{
   if (minorDim_ != matrix.majorDim_) {
      throw CoinError("dimension mismatch", "rightAppendOrthoOrdered",
		     "CoinPackedMatrix");
      }
   if (matrix.majorDim_ == 0)
      return;

   int i;
   CoinBigIndex j;
   // this trickery is needed because MSVC++ is not willing to delete[] a
   // 'const int *'
   int * orthoLengthPtr = matrix.countOrthoLength();
   const int * orthoLength = orthoLengthPtr;

   if (majorDim_ + matrix.minorDim_ > maxMajorDim_) {
      resizeForAddingMajorVectors(matrix.minorDim_, orthoLength);
   } else {
     const double extra_gap = extraGap_;
     start_ += majorDim_;
     for (i = 0; i < matrix.minorDim_ - 1; ++i) {
       start_[i+1] = start_[i] + CoinLengthWithExtra(orthoLength[i], extra_gap);
     }
     start_ -= majorDim_;
     if (start_[majorDim_ + matrix.minorDim_] > maxSize_) {
       resizeForAddingMajorVectors(matrix.minorDim_, orthoLength);
     }
   }
   // At this point everything is big enough to accommodate the new entries.
   // Also, start_ is set to the correct starting points for all the new
   // major-dimension vectors. The length of the new major-dimension vectors
   // may or may not be correctly set. Hence we just zero them out and they'll
   // be set when the entries are actually added below.

   start_ += majorDim_;
   length_ += majorDim_;

   CoinFillN(length_, matrix.minorDim_, 0);

   for (i = 0; i < matrix.majorDim_; ++i) {
      const CoinBigIndex last = matrix.getVectorLast(i);
      for (j = matrix.getVectorFirst(i); j < last; ++j) {
	 const int ind = matrix.index_[j];
	 element_[start_[ind] + length_[ind]] = matrix.element_[j];
	 index_[start_[ind] + (length_[ind]++)] = i;
      }
   }
   
   length_ -= majorDim_;
   start_ -= majorDim_;

   delete[] orthoLengthPtr;
}

//-----------------------------------------------------------------------------
   
void
CoinPackedMatrix::minorAppendOrthoOrdered(const CoinPackedMatrix& matrix)
   throw(CoinError)
{
   if (majorDim_ != matrix.minorDim_) {
      throw CoinError("dimension mismatch", "bottomAppendOrthoOrdered",
		     "CoinPackedMatrix");
      }
   if (matrix.majorDim_ == 0)
      return;

   int i;
   // first compute how many entries will be added to each major-dimension
   // vector, and if needed, resize the matrix to accommodate all
   // this trickery is needed because MSVC++ is not willing to delete[] a
   // 'const int *'
   int * addedEntriesPtr = matrix.countOrthoLength();
   const int * addedEntries = addedEntriesPtr;
   for (i = majorDim_ - 1; i >= 0; --i) {
      if (start_[i] + length_[i] + addedEntries[i] > start_[i+1])
	 break;
   }
   if (i >= 0)
      resizeForAddingMinorVectors(addedEntries);
   delete[] addedEntriesPtr;

   // now insert the entries of matrix
   for (i = 0; i < matrix.majorDim_; ++i) {
      const CoinBigIndex last = matrix.getVectorLast(i);
      for (CoinBigIndex j = matrix.getVectorFirst(i); j != last; ++j) {
	 const int ind = matrix.index_[j];
	 element_[start_[ind] + length_[ind]] = matrix.element_[j];
	 index_[start_[ind] + (length_[ind]++)] = minorDim_;
      }
      ++minorDim_;
   }
   size_ += matrix.size_;
}

//#############################################################################
//#############################################################################

void
CoinPackedMatrix::deleteMajorVectors(const int numDel,
				    const int * indDel) throw(CoinError)
{
   int *sortedDelPtr = CoinTestIndexSet(numDel, indDel, majorDim_,
				       "deleteMajorVectors");
   const int * sortedDel = sortedDelPtr == 0 ? indDel : sortedDelPtr;

   if (numDel == majorDim_) {
      // everything is deleted
      majorDim_ = 0;
      size_ = 0;
      if (sortedDelPtr)
	 delete[] sortedDelPtr;
      return;
   }

   CoinBigIndex deleted = 0;
   const int last = numDel - 1;
   for (int i = 0; i < last; ++i) {
      const int ind = sortedDel[i];
      const int ind1 = sortedDel[i+1];
      deleted += length_[ind];
      if (ind1 - ind > 1) {
	 CoinCopy(start_ + (ind + 1), start_ + ind1, start_ + (ind - i));
	 CoinCopy(length_ + (ind + 1), length_ + ind1, length_ + (ind - i));
      }
   }

   // copy the last block of length_ and start_
   if (sortedDel[last] != majorDim_ - 1) {
      const int ind = sortedDel[last];
      const int ind1 = majorDim_;
      deleted += length_[ind];
      CoinCopy(start_ + (ind + 1), start_ + ind1, start_ + (ind - last));
      CoinCopy(length_ + (ind + 1), length_ + ind1, length_ + (ind - last));
   }
   majorDim_ -= numDel;
   const int lastlength = CoinLengthWithExtra(length_[majorDim_-1], extraGap_);
   start_[majorDim_] = CoinMin(start_[majorDim_-1] + lastlength, maxSize_);
   size_ -= deleted;

   // if the very first major vector was deleted then copy the new first major
   // vector to the beginning to make certain that start_[0] is 0. This may
   // not be necessary, but better safe than sorry...
   if (sortedDel[0] == 0) {
      CoinCopyN(index_ + start_[0], length_[0], index_);
      CoinCopyN(element_ + start_[0], length_[0], element_);
      start_[0] = 0;
   }

   delete[] sortedDelPtr;
}

//#############################################################################

void
CoinPackedMatrix::deleteMinorVectors(const int numDel,
				    const int * indDel) throw(CoinError)
{
  int i, j, k;

  // first compute the new index of every row
  int* newindexPtr = new int[minorDim_];
  CoinIotaN(newindexPtr, minorDim_, 0);
  for (j = 0; j < numDel; ++j) {
    const int ind = indDel[j];
#ifdef COIN_DEBUG
    if (ind < 0 || ind >= minorDim_)
      throw CoinError("out of range index",
		     "deleteMinorVectors", "CoinPackedMatrix");
    if (newindexPtr[ind] == -1)
      throw CoinError("duplicate index",
		     "deleteMinorVectors", "CoinPackedMatrix");
#endif
    newindexPtr[ind] = -1;
  }
  for (i = 0, k = 0; i < minorDim_; ++i) {
    if (newindexPtr[i] != -1) {
      newindexPtr[i] = k++;
    }
  }

  // Now crawl through the matrix
  const int * newindex = newindexPtr;
  int deleted = 0;
  for (i = 0; i < majorDim_; ++i) {
    int * index = index_ + start_[i];
    double * elem = element_ + start_[i];
    const int length_i = length_[i];
    for (j = 0, k = 0; j < length_i; ++j) {
      const int ind = newindex[index[j]];
      if (ind != -1) {
	index[k] = ind;
	elem[k++] = elem[j];
      }
    }
    deleted += length_i - k;
    length_[i] = k;
  }

  delete[] newindexPtr;

  minorDim_ -= numDel;
  size_ -= deleted;
}

//#############################################################################
//#############################################################################

void
CoinPackedMatrix::timesMajor(const double * x, double * y) const 
{
   memset(y, 0, minorDim_ * sizeof(double));
   for (int i = majorDim_ - 1; i >= 0; --i) {
      const double x_i = x[i];
      if (x_i != 0.0) {
	 const CoinBigIndex last = getVectorLast(i);
	 for (CoinBigIndex j = getVectorFirst(i); j < last; ++j)
	    y[index_[j]] += x_i * element_[j];
      }
   }
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::timesMajor(const CoinPackedVectorBase& x, double * y) const 
{
   memset(y, 0, minorDim_ * sizeof(double));
   for (CoinBigIndex i = x.getNumElements() - 1; i >= 0; --i) {
      const double x_i = x.getElements()[i];
      if (x_i != 0.0) {
	 const int ind = x.getIndices()[i];
	 const CoinBigIndex last = getVectorLast(ind);
	 for (CoinBigIndex j = getVectorFirst(ind); j < last; ++j)
	    y[index_[j]] += x_i * element_[j];
      }
   }
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::timesMinor(const double * x, double * y) const 
{
   memset(y, 0, majorDim_ * sizeof(double));
   for (int i = majorDim_ - 1; i >= 0; --i) {
      double y_i = 0;
      const CoinBigIndex last = getVectorLast(i);
      for (CoinBigIndex j = getVectorFirst(i); j < last; ++j)
	 y_i += x[index_[j]] * element_[j];
      y[i] = y_i;
   }
}

//-----------------------------------------------------------------------------

void
CoinPackedMatrix::timesMinor(const CoinPackedVectorBase& x, double * y) const 
{
   memset(y, 0, majorDim_ * sizeof(double));
   for (int i = majorDim_ - 1; i >= 0; --i) {
      double y_i = 0;
      const CoinBigIndex last = getVectorLast(i);
      for (CoinBigIndex j = getVectorFirst(i); j < last; ++j)
	 y_i += x[index_[j]] * element_[j];
      y[i] = y_i;
   }
}

//#############################################################################
//#############################################################################

CoinPackedMatrix::CoinPackedMatrix() :
   colOrdered_(true),
   extraGap_(0.25),
   extraMajor_(0.25),
   element_(0), 
   index_(0),
   length_(0),
   majorDim_(0),
   minorDim_(0),
   size_(0),
   maxMajorDim_(0),
   maxSize_(0) 
{
  start_ = new CoinBigIndex[1];
  start_[0] = 0;
}

//-----------------------------------------------------------------------------

CoinPackedMatrix::CoinPackedMatrix(const bool colordered,
				 const double extraMajor,
				 const double extraGap) :
   colOrdered_(colordered),
   extraGap_(extraGap),
   extraMajor_(extraMajor),
   element_(0), 
   index_(0),
   length_(0),
   majorDim_(0),
   minorDim_(0),
   size_(0),
   maxMajorDim_(0),
   maxSize_(0)
{
  start_ = new CoinBigIndex[1];
  start_[0] = 0;
}

//-----------------------------------------------------------------------------

CoinPackedMatrix::CoinPackedMatrix(const bool colordered,
				 const int minor, const int major,
				 const CoinBigIndex numels,
				 const double * elem, const int * ind,
				 const CoinBigIndex * start, const int * len,
				 const double extraMajor,
				 const double extraGap) :
   colOrdered_(colordered),
   extraGap_(extraGap),
   extraMajor_(extraMajor),
   element_(NULL),
   index_(NULL),
   start_(NULL),
   length_(NULL),
   majorDim_(0),
   minorDim_(0),
   size_(0),
   maxMajorDim_(0),
   maxSize_(0)
{
   gutsOfOpEqual(colordered, minor, major, numels, elem, ind, start, len);
}

//-----------------------------------------------------------------------------
   
CoinPackedMatrix::CoinPackedMatrix(const bool colordered,
				 const int minor, const int major,
         const CoinBigIndex numels,
         const double * elem, const int * ind,
         const CoinBigIndex * start, const int * len) :
   colOrdered_(colordered),
   extraGap_(0.0),
   extraMajor_(0.0),
   element_(NULL),
   index_(NULL),
   start_(NULL),
   length_(NULL),
   majorDim_(0),
   minorDim_(0),
   size_(0),
   maxMajorDim_(0),
   maxSize_(0)
{
     gutsOfOpEqual(colordered, minor, major, numels, elem, ind, start, len);
}
  
//-----------------------------------------------------------------------------
// makes column ordered from triplets and takes out duplicates 
// will be sorted 
//
// This is an interesting in-place sorting algorithm; 
// We have triples, and want to sort them so that triples with the same column
// are adjacent.
// We begin by computing how many entries there are for each column (columnCount)
// and using that to compute where each set of column entries will *end* (startColumn).
// As we drop entries into place, startColumn is decremented until it contains
// the position where the column entries *start*.
// The invalid column index -2 means there's a "hole" in that position;
// the invalid column index -1 means the entry in that spot is "where it wants to go".
// Initially, no one is where they want to go.
// Going back to front,
//    if that entry is where it wants to go
//    then leave it there
//    otherwise pick it up (which leaves a hole), and 
//	      for as long as you have an entry in your right hand,
//	- pick up the entry (with your left hand) in the position where the one in 
//		your right hand wants to go;
//	- pass the entry in your left hand to your right hand;
//	- was that entry really just the "hole"?  If so, stop.
// It could be that all the entries get shuffled in the first loop iteration
// and all the rest just confirm that everyone is happy where they are.
// We never move an entry that is where it wants to go, so entries are moved at
// most once.  They may not change position if they happen to initially be
// where they want to go when the for loop gets to them.
// It depends on how many subpermutations the triples initially defined.
// Each while loop takes care of one permutation.
// The while loop has to stop, because each time around we mark one entry as happy.
// We can't run into a happy entry, because we are decrementing the startColumn
// all the time, so we must be running into new entries.
// Once we've processed all the slots for a column, it cannot be the case that
// there are any others that want to go there.
// This all means that we eventually must run into the hole.
CoinPackedMatrix::CoinPackedMatrix(
     const bool colordered,
     const int * indexRow ,
     const int * indexColumn,
     const double * element, 
     CoinBigIndex numberElements ) 
     :
   colOrdered_(colordered),
     extraGap_(0.0),
     extraMajor_(0.0),
     element_(NULL),
     index_(NULL),
     start_(NULL),
     length_(NULL),
     majorDim_(0),
     minorDim_(0),
     size_(0),
     maxMajorDim_(0),
     maxSize_(0)
{
     CoinAbsFltEq eq;
       int * colIndices = new int[numberElements];
       int * rowIndices = new int[numberElements];
       double * elements = new double[numberElements];
       CoinCopyN(element,numberElements,elements);
     if ( colordered ) {
       CoinCopyN(indexColumn,numberElements,colIndices);
       CoinCopyN(indexRow,numberElements,rowIndices);
     }
     else {
       CoinCopyN(indexColumn,numberElements,rowIndices);
       CoinCopyN(indexRow,numberElements,colIndices);
     }

  int numberRows=*std::max_element(rowIndices,rowIndices+numberElements)+1;
  int * rowCount = new int[numberRows];
  int numberColumns=*std::max_element(colIndices,colIndices+numberElements)+1;
  int * columnCount = new int[numberColumns];
  CoinBigIndex * startColumn = new CoinBigIndex[numberColumns+1];
  int * lengths = new int[numberColumns+1];

  int iColumn,i;
  CoinBigIndex k;
  for (i=0;i<numberRows;i++) {
    rowCount[i]=0;
  }
  for (i=0;i<numberColumns;i++) {
    columnCount[i]=0;
  }
  for (i=0;i<numberElements;i++) {
    int iRow=rowIndices[i];
    int iColumn=colIndices[i];
    rowCount[iRow]++;
    columnCount[iColumn]++;
  }
  CoinBigIndex iCount=0;
  for (iColumn=0;iColumn<numberColumns;iColumn++) {
    /* position after end of Column */
    iCount+=columnCount[iColumn];
    startColumn[iColumn]=iCount;
  } /* endfor */
  startColumn[iColumn]=iCount;
  for (k=numberElements-1;k>=0;k--) {
    iColumn=colIndices[k];
    if (iColumn>=0) {
      /* pick up the entry with your right hand */
      double value = elements[k];
      int iRow=rowIndices[k];
      colIndices[k]=-2;	/* the hole */

      while (1) {
	/* pick this up with your left */
        CoinBigIndex iLook=startColumn[iColumn]-1;
        double valueSave=elements[iLook];
        int iColumnSave=colIndices[iLook];
        int iRowSave=rowIndices[iLook];

	/* put the right-hand entry where it wanted to go */
        startColumn[iColumn]=iLook;
        elements[iLook]=value;
        rowIndices[iLook]=iRow;
        colIndices[iLook]=-1;	/* mark it as being where it wants to be */

	/* there was something there */
        if (iColumnSave>=0) {
          iColumn=iColumnSave;
          value=valueSave;
          iRow=iRowSave;
	} else if (iColumnSave == -2) {	/* that was the hole */
          break;
	} else {
	  assert(1==0);	/* should never happen */
	}
	/* endif */
      } /* endwhile */
    } /* endif */
  } /* endfor */

  /* now pack the elements and combine entries with the same row and column */
  /* also, drop entries with "small" coefficients */
  numberElements=0;
  for (iColumn=0;iColumn<numberColumns;iColumn++) {
    CoinBigIndex start=startColumn[iColumn];
    CoinBigIndex end =startColumn[iColumn+1];
    lengths[iColumn]=0;
    startColumn[iColumn]=numberElements;
    if (end>start) {
      int lastRow;
      double lastValue;
      // sorts on indices dragging elements with
      CoinSort_2(rowIndices+start,rowIndices+end,elements+start,CoinFirstLess_2<int, double>());
      lastRow=rowIndices[start];
      lastValue=elements[start];
      for (i=start+1;i<end;i++) {
        int iRow=rowIndices[i];
        double value=elements[i];
        if (iRow>lastRow) {
          //if(fabs(lastValue)>tolerance) {
          if(!eq(lastValue,0.0)) {
            rowIndices[numberElements]=lastRow;
            elements[numberElements]=lastValue;
            numberElements++;
            lengths[iColumn]++;
          }
          lastRow=iRow;
          lastValue=value;
        } else {
          lastValue+=value;
        } /* endif */
      } /* endfor */
      //if(fabs(lastValue)>tolerance) {
      if(!eq(lastValue,0.0)) {
        rowIndices[numberElements]=lastRow;
        elements[numberElements]=lastValue;
        numberElements++;
        lengths[iColumn]++;
      }
    }
  } /* endfor */
  startColumn[numberColumns]=numberElements;
#if 0
  gutsOfOpEqual(colordered,numberRows,numberColumns,numberElements,elements,rowIndices,startColumn,lengths);
  
  delete [] rowCount;
  delete [] columnCount;
  delete [] startColumn;
  delete [] lengths;

  delete [] colIndices;
  delete [] rowIndices;
  delete [] elements;
#else
  assignMatrix(colordered,numberRows,numberColumns,numberElements,
    elements,rowIndices,startColumn,lengths); 
  delete [] rowCount;
  delete [] columnCount;
  delete [] lengths;
  delete [] colIndices;
#endif

}

//-----------------------------------------------------------------------------

CoinPackedMatrix::CoinPackedMatrix (const CoinPackedMatrix & rhs) :
   colOrdered_(true),
   extraGap_(0.0),
   extraMajor_(0.0),
   element_(0), 
   index_(0),
   start_(0),
   length_(0),
   majorDim_(0),
   minorDim_(0),
   size_(0),
   maxMajorDim_(0),
   maxSize_(0)
{
   gutsOfCopyOf(rhs.colOrdered_,
		rhs.minorDim_, rhs.majorDim_, rhs.size_,
		rhs.element_, rhs.index_, rhs.start_, rhs.length_,
		rhs.extraMajor_, rhs.extraGap_);
}

//-----------------------------------------------------------------------------

CoinPackedMatrix::~CoinPackedMatrix ()
{
   gutsOfDestructor();
}

//#############################################################################
//#############################################################################
//#############################################################################

void
CoinPackedMatrix::gutsOfDestructor()
{
   delete[] length_;
   delete[] start_;
   delete[] index_;
   delete[] element_;
   length_ = 0;
   start_ = 0;
   index_ = 0;
   element_ = 0;
}

//#############################################################################

void
CoinPackedMatrix::gutsOfCopyOf(const bool colordered,
			      const int minor, const int major,
			      const CoinBigIndex numels,
			      const double * elem, const int * ind,
			      const CoinBigIndex * start, const int * len,
			      const double extraMajor, const double extraGap)
{
   colOrdered_ = colordered;
   majorDim_ = major;
   minorDim_ = minor;
   size_ = numels;

   extraGap_ = extraGap;
   extraMajor_ = extraMajor;

   maxMajorDim_ = CoinLengthWithExtra(majorDim_, extraMajor_);

   if (maxMajorDim_ > 0) {
      length_ = new int[maxMajorDim_];
      if (len == 0) {
	 std::adjacent_difference(start + 1, start + (major + 1), length_);
      } else {
	 CoinDisjointCopyN(len, major, length_);
      }
      start_ = new CoinBigIndex[maxMajorDim_+1];
      CoinDisjointCopyN(start, major+1, start_);
   }

   maxSize_ = maxMajorDim_ > 0 ? start_[major] : 0;
   maxSize_ = CoinLengthWithExtra(maxSize_, extraMajor_);

   if (maxSize_ > 0) {
      element_ = new double[maxSize_];
      index_ = new int[maxSize_];
      // we can't just simply memcpy these content over, because that can
      // upset memory debuggers like purify if there were gaps and those gaps
      // were uninitialized memory blocks
      for (int i = majorDim_ - 1; i >= 0; --i) {
	 CoinDisjointCopyN(ind + start[i], length_[i], index_ + start_[i]);
	 CoinDisjointCopyN(elem + start[i], length_[i], element_ + start_[i]);
      }
   }
}

//#############################################################################

void
CoinPackedMatrix::gutsOfOpEqual(const bool colordered,
			       const int minor, const int major,
			       const CoinBigIndex numels,
			       const double * elem, const int * ind,
			       const CoinBigIndex * start, const int * len)
{
   colOrdered_ = colordered;
   majorDim_ = major;
   minorDim_ = minor;
   size_ = numels;

   maxMajorDim_ = CoinLengthWithExtra(majorDim_, extraMajor_);

   int i;
   if (maxMajorDim_ > 0) {
      length_ = new int[maxMajorDim_];
      if (len == 0) {
	 std::adjacent_difference(start + 1, start + (major + 1), length_);
      } else {
	 CoinDisjointCopyN(len, major, length_);
      }
      start_ = new CoinBigIndex[maxMajorDim_+1];
      start_[0] = 0;
      if (extraGap_ == 0) {
	 for (i = 0; i < major; ++i)
	    start_[i+1] = start_[i] + length_[i];
      } else {
	const double extra_gap = extraGap_;
	for (i = 0; i < major; ++i)
	  start_[i+1] = start_[i] + CoinLengthWithExtra(length_[i], extra_gap);
      }
   } else {
     // empty matrix
     start_ = new CoinBigIndex[1];
     start_[0] = 0;
   }

   maxSize_ = maxMajorDim_ > 0 ? start_[major] : 0;
   maxSize_ = CoinLengthWithExtra(maxSize_, extraMajor_);

   if (maxSize_ > 0) {
      element_ = new double[maxSize_];
      index_ = new int[maxSize_];
      // we can't just simply memcpy these content over, because that can
      // upset memory debuggers like purify if there were gaps and those gaps
      // were uninitialized memory blocks
      for (i = majorDim_ - 1; i >= 0; --i) {
	 CoinDisjointCopyN(ind + start[i], length_[i], index_ + start_[i]);
	 CoinDisjointCopyN(elem + start[i], length_[i], element_ + start_[i]);
      }
   }
}

//#############################################################################

// This routine is called only if we MUST resize!
void
CoinPackedMatrix::resizeForAddingMajorVectors(const int numVec,
					     const int * lengthVec)
{
  const double extra_gap = extraGap_;
  int i;

  maxMajorDim_ =
    CoinMax(maxMajorDim_, CoinLengthWithExtra(majorDim_ + numVec, extraMajor_));

  CoinBigIndex * newStart = new CoinBigIndex[maxMajorDim_ + 1];
  int * newLength = new int[maxMajorDim_];

  CoinDisjointCopyN(length_, majorDim_, newLength);
  // fake that the new vectors are there
  CoinDisjointCopyN(lengthVec, numVec, newLength + majorDim_);
  majorDim_ += numVec;

  newStart[0] = 0;
  if (extra_gap == 0) {
    for (i = 0; i < majorDim_; ++i)
      newStart[i+1] = newStart[i] + newLength[i];
  } else {
    for (i = 0; i < majorDim_; ++i)
      newStart[i+1] = newStart[i] + CoinLengthWithExtra(newLength[i],extra_gap);
  }

  maxSize_ =
    CoinMax(maxSize_,newStart[majorDim_]+ (int) extraMajor_);
  majorDim_ -= numVec;

  int * newIndex = new int[maxSize_];
  double * newElem = new double[maxSize_];
  for (i = majorDim_ - 1; i >= 0; --i) {
    CoinDisjointCopyN(index_ + start_[i], length_[i], newIndex + newStart[i]);
    CoinDisjointCopyN(element_ + start_[i], length_[i], newElem + newStart[i]);
  }

  gutsOfDestructor();
  start_   = newStart;
  length_  = newLength;
  index_   = newIndex;
  element_ = newElem;
}

//#############################################################################

void
CoinPackedMatrix::resizeForAddingMinorVectors(const int * addedEntries)
{
   int i;
   maxMajorDim_ =
     CoinMax(CoinLengthWithExtra(majorDim_, extraMajor_), maxMajorDim_);
   CoinBigIndex * newStart = new CoinBigIndex[maxMajorDim_ + 1];
   int * newLength = new int[maxMajorDim_];
   // increase the lengths temporarily so that the correct new start positions
   // can be easily computed (it's faster to modify the lengths and reset them
   // than do a test for every entry when the start positions are computed.
   for (i = majorDim_ - 1; i >= 0; --i)
      newLength[i] = length_[i] + addedEntries[i];

   newStart[0] = 0;
   if (extraGap_ == 0) {
      for (i = 0; i < majorDim_; ++i)
	 newStart[i+1] = newStart[i] + newLength[i];
   } else {
      const double eg = extraGap_;
      for (i = 0; i < majorDim_; ++i)
	 newStart[i+1] = newStart[i] + CoinLengthWithExtra(newLength[i], eg);
   }

   // reset the lengths
   for (i = majorDim_ - 1; i >= 0; --i)
      newLength[i] -= addedEntries[i];

   maxSize_ =
     CoinMax(newStart[majorDim_]+ (int) extraMajor_, maxSize_);
   int * newIndex = new int[maxSize_];
   double * newElem = new double[maxSize_];
   for (i = majorDim_ - 1; i >= 0; --i) {
      CoinDisjointCopyN(index_ + start_[i], length_[i],
			newIndex + newStart[i]);
      CoinDisjointCopyN(element_ + start_[i], length_[i],
			newElem + newStart[i]);
   }

   gutsOfDestructor();
   start_   = newStart;
   length_  = newLength;
   index_   = newIndex;
   element_ = newElem;
}

//#############################################################################
//#############################################################################

void
CoinPackedMatrix::dumpMatrix(const char* fname) const
{
  if (! fname) {
    printf("Dumping matrix...\n\n");
    printf("colordered: %i\n", isColOrdered() ? 1 : 0);
    const int major = getMajorDim();
    const int minor = getMinorDim();
    printf("major: %i   minor: %i\n", major, minor);
    for (int i = 0; i < major; ++i) {
      printf("vec %i has length %i with entries:\n", i, length_[i]);
      for (CoinBigIndex j = start_[i]; j < start_[i] + length_[i]; ++j) {
	printf("        %15i  %40.25f\n", index_[j], element_[j]);
      }
    }
    printf("\nFinished dumping matrix\n");
  } else {
    FILE* out = fopen(fname, "w");
    fprintf(out, "Dumping matrix...\n\n");
    fprintf(out, "colordered: %i\n", isColOrdered() ? 1 : 0);
    const int major = getMajorDim();
    const int minor = getMinorDim();
    fprintf(out, "major: %i   minor: %i\n", major, minor);
    for (int i = 0; i < major; ++i) {
      fprintf(out, "vec %i has length %i with entries:\n", i, length_[i]);
      for (CoinBigIndex j = start_[i]; j < start_[i] + length_[i]; ++j) {
	fprintf(out, "        %15i  %40.25f\n", index_[j], element_[j]);
      }
    }
    fprintf(out, "\nFinished dumping matrix\n");
    fclose(out);
  }
}
bool 
CoinPackedMatrix::isEquivalent2(const CoinPackedMatrix& rhs) const
{
  CoinRelFltEq eq;
  // Both must be column order or both row ordered and must be of same size
  if (isColOrdered() ^ rhs.isColOrdered()) {
    std::cerr<<"Ordering "<<isColOrdered()<<
      " rhs - "<<rhs.isColOrdered()<<std::endl;
    return false;
  }
  if (getNumCols() != rhs.getNumCols()) {
    std::cerr<<"NumCols "<<getNumCols()<<
      " rhs - "<<rhs.getNumCols()<<std::endl;
    return false;
  }
  if  (getNumRows() != rhs.getNumRows()) {
    std::cerr<<"NumRows "<<getNumRows()<<
      " rhs - "<<rhs.getNumRows()<<std::endl;
    return false;
  }
  if  (getNumElements() != rhs.getNumElements()) {
    std::cerr<<"NumElements "<<getNumElements()<<
      " rhs - "<<rhs.getNumElements()<<std::endl;
    return false;
  }
  
  for (int i=getMajorDim()-1; i >= 0; --i) {
    CoinShallowPackedVector pv = getVector(i);
    CoinShallowPackedVector rhsPv = rhs.getVector(i);
    if ( !pv.isEquivalent(rhsPv,eq) ) {
      std::cerr<<"vector # "<<i<<" nel "<<pv.getNumElements()<<
      " rhs - "<<rhsPv.getNumElements()<<std::endl;
      int j;
      const int * inds = pv.getIndices();
      const double * elems = pv.getElements();
      const int * inds2 = rhsPv.getIndices();
      const double * elems2 = rhsPv.getElements();
      for ( j = 0 ;j < pv.getNumElements() ;  ++j) {
	double diff = elems[j]-elems2[j];
	if (diff) {
	  std::cerr<<j<<"( "<<inds[j]<<", "<<elems[j]<<"), rhs ( "<<
	    inds2[j]<<", "<<elems2[j]<<") diff "<<
	    diff<<std::endl;
	  const int * xx = (const int *) (elems+j);
	  printf("%x %x",xx[0],xx[1]);
	  xx = (const int *) (elems2+j);
	  printf(" %x %x\n",xx[0],xx[1]);
	}
      }
      //return false;
    }
  }
  return true;
}
