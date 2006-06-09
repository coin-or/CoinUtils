// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
#ifndef CoinPackedMatrix_H
#define CoinPackedMatrix_H

#include "CoinError.hpp"
#ifndef CLP_NO_VECTOR
#include "CoinPackedVectorBase.hpp"
#include "CoinShallowPackedVector.hpp"
#else
#include "CoinFinite.hpp"
#include "CoinFloatEqual.hpp"
#endif

/** Sparse Matrix Base Class

  This class is used for storing a matrix by rows or columns.

  The sparse represention can be completely compact or it can 
  have "extra" space. The extra space can be added at the end 
  of rows and/or columns.  Incorporating extra space into the 
  sparse matrix representation can improve performance in 
  cases where new data needs to be inserted into the packed 
  matrix.

  For example if the matrix:
  @verbatim
     3  1  0   -2   -1  0  0   -1                 
     0  2  1.1  0    0  0  0    0                       
     0  0  1    0    0  1  0    0         
     0  0  0    2.8  0  0 -1.2  0   
   5.6  0  0    0    1  0  0    1.9

  was stored by rows (with no extra space) in 
  CoinPackedMatrix r then: 
    r.getElements() returns a vector containing: 
      3 1 -2 -1 -1 2 1.1 1 1 2.8 -1.2 5.6 1 1.9 
    r.getIndices() returns a vector containing: 
      0 1  3  4  7 1 2   2 5 3    6   0   4 7 
    r.getVectorStarts() returns a vector containing: 
      0 5 7 9 11 14 
    r.getNumElements() returns 14. 
    r.getMajorDim() returns 5. 
    r.getVectorSize(0) returns 5. 
    r.getVectorSize(1) returns 2. 
    r.getVectorSize(2) returns 2. 
    r.getVectorSize(3) returns 2. 
    r.getVectorSize(4) returns 3. 
 
  If stored by columns (with no extra space) then: 
    c.getElements() returns a vector containing: 
      3 5.6 1 2 1.1 1 -2 2.8 -1 1 1 -1.2 -1 1.9 
    c.getIndices() returns a vector containing: 
      0  4  0 1 1   2  0 3    0 4 2  3    0 4 
    c.getVectorStarts() returns a vector containing: 
      0 2 4 6 8 10 11 12 14 
    c.getNumElements() returns 14. 
    c.getMajorDim() returns 8. 
  @endverbatim
*/
class CoinPackedMatrix  {
   friend void CoinPackedMatrixUnitTest();

public:


  //---------------------------------------------------------------------------
  /**@name Query members */
  //@{
    /** Return the current setting of the extra gap. */
    double getExtraGap() const { return extraGap_; }
    /** Return the current setting of the extra major. */
    double getExtraMajor() const { return extraMajor_; }

    /** Reserve sufficient space for appending major-ordered vectors. 
	If create is true, empty columns are created (for column generation) */
    void reserve(const int newMaxMajorDim, const CoinBigIndex newMaxSize,
		 bool create=false);
    /** Clear the data, but do not free any arrays */
    void clear();

    /** Whether the packed matrix is column major ordered or not. */
    bool isColOrdered() const { return colOrdered_; }
    /** Number of entries in the packed matrix. */
    CoinBigIndex getNumElements() const { return size_; }
    /** Number of columns. */
    int getNumCols() const { return colOrdered_ ? majorDim_ : minorDim_; }
    /** Number of rows. */
    int getNumRows() const { return colOrdered_ ? minorDim_ : majorDim_; }

    /** A vector containing the elements in the packed matrix. Note that there
	might be gaps in this list, entries that do not belong to any
	major-dimension vector. To get the actual elements one should look at
	this vector together with vectorStarts and vectorLengths. */
    inline const double * getElements() const { return element_; }
    /** A vector containing the minor indices of the elements in the packed
        matrix. Note that there might be gaps in this list, entries that do not
        belong to any major-dimension vector. To get the actual elements one
        should look at this vector together with vectorStarts and
        vectorLengths. */
    inline const int * getIndices() const { return index_; }

    /** The size of the <code>vectorStarts</code> array */
    int getSizeVectorStarts()const { return majorDim_ > 0 ? majorDim_+1 : 0;}
    /** The size of the <code>vectorLengths</code> array */
    int getSizeVectorLengths() const { return majorDim_; }
    /** The positions where the major-dimension vectors start in elements and
        indices. */
    inline const CoinBigIndex * getVectorStarts() const { return start_; }
    /** The lengths of the major-dimension vectors. */
    inline const int * getVectorLengths() const { return length_; }


    /** The position of the first element in the i'th major-dimension vector.
     */
    CoinBigIndex getVectorFirst(const int i) const {
      if (i < 0 || i >= majorDim_)
	throw CoinError("bad index", "vectorFirst", "CoinPackedMatrix");
      return start_[i];
    }
    /** The position of the last element (well, one entry <em>past</em> the
        last) in the i'th major-dimension vector. */
    CoinBigIndex getVectorLast(const int i) const {
      if (i < 0 || i >= majorDim_)
	throw CoinError("bad index", "vectorLast", "CoinPackedMatrix");
      return start_[i] + length_[i];
    }
    /** The length of i'th vector. */
    inline int getVectorSize(const int i) const {
      if (i < 0 || i >= majorDim_)
	throw CoinError("bad index", "vectorSize", "CoinPackedMatrix");
      return length_[i];
    }
#ifndef CLP_NO_VECTOR  
    /** Return the i'th vector in matrix. */
    const CoinShallowPackedVector getVector(int i) const {
      if (i < 0 || i >= majorDim_)
	throw CoinError("bad index", "vector", "CoinPackedMatrix");
      return CoinShallowPackedVector(length_[i],
  				    index_ + start_[i],
  				    element_ + start_[i],
  				    false);
    }
#endif
    /** Returns an array containing major indices.  The array is
	  getNumElements long and if getVectorStarts() is 0,2,5 then
	  the array would start 0,0,1,1,1,2...
	  This method is provided to go back from a packed format
	  to a triple format.  It returns NULL if there are gaps in
	  matrix so user should use removeGaps() if there are any gaps.
	  It does this as this array has to match getElements() and 
	  getIndices() and because it makes no sense otherwise.
	  The returned array is allocated with <code>new int[]</code>,
	  free it with  <code>delete[]</code>. */
    int * getMajorIndices() const;
  //@}

  //---------------------------------------------------------------------------
  /**@name Modifying members. */
  //@{
    /** Set the dimansions of the matrix. In effect, append new empty
	columns/rows to the matrix. A negative number for either dimension
	means that that dimension doesn't change. Otherwise the new dimensions
	MUST be at least as large as the current ones otherwise an exception
	is thrown. */
    void setDimensions(int numrows, int numcols);
   
    /** Set the extra gap to be allocated to the specified value. */
    void setExtraGap(const double newGap);
    /** Set the extra major to be allocated to the specified value. */
    void setExtraMajor(const double newMajor);
#ifndef CLP_NO_VECTOR
    /** Append a column to the end of the matrix. When libosi is compiled with
	a COIN_DEBUG defined then this method throws an exception if the new
	column contains an index that's larger than the number of rows (-1).
	Otherwise the method assumes that every index fits into the matrix. */
    void appendCol(const CoinPackedVectorBase& vec);
#endif
    /** Append a column to the end of the matrix. When libosi is compiled with
	a COIN_DEBUG defined then this method throws an exception if the new
	column contains an index that's larger than the number of rows (-1).
	Otherwise the method assumes that every index fits into the matrix. */
    void appendCol(const int vecsize,
  		  const int *vecind, const double *vecelem);
#ifndef CLP_NO_VECTOR
    /** Append a set of columns to the end of the matrix. When libosi is
	compiled with a COIN_DEBUG defined then this method throws an exception
	if any of the new columns contain an index that's larger than the
	number of rows (-1). Otherwise the method assumes that every index
	fits into the matrix. */
    void appendCols(const int numcols,
		    const CoinPackedVectorBase * const * cols);
#endif
    /** Append a set of columns to the end of the matrix. Returns number of errors
	i.e. if any of the new columns contain an index that's larger than the
	number of rows-1 (if numberRows>0) or duplicates (if numberRows>0).  */
    int appendCols(const int numcols,
		    const CoinBigIndex * columnStarts, const int * row,
                   const double * element, int numberRows=-1);
#ifndef CLP_NO_VECTOR
  /** Append a row to the end of the matrix. When libosi is compiled with
	a COIN_DEBUG defined then this method throws an exception if the new
	row contains an index that's larger than the number of columns (-1).
	Otherwise the method assumes that every index fits into the matrix. */
    void appendRow(const CoinPackedVectorBase& vec);
#endif
    /** Append a row to the end of the matrix. When libosi is compiled with
	a COIN_DEBUG defined then this method throws an exception if the new
	row contains an index that's larger than the number of columns (-1).
	Otherwise the method assumes that every index fits into the matrix. */
    void appendRow(const int vecsize,
  		  const int *vecind, const double *vecelem);
#ifndef CLP_NO_VECTOR
    /** Append a set of rows to the end of the matrix. When libosi is
	compiled with a COIN_DEBUG defined then this method throws an exception
	if any of the new rows contain an index that's larger than the
	number of columns (-1). Otherwise the method assumes that every index
	fits into the matrix. */
    void appendRows(const int numrows,
		    const CoinPackedVectorBase * const * rows);
#endif
    /** Append a set of rows to the end of the matrix. Returns number of errors
	i.e. if any of the new rows contain an index that's larger than the
	number of columns-1 (if numberColumns>0) or duplicates (if numberColumns>0).  */
    int appendRows(const int numrows,
		    const CoinBigIndex * rowStarts, const int * column,
                   const double * element, int numberColumns=-1);
  
    /** Append the argument to the "right" of the current matrix. Imagine this
        as adding new columns (don't worry about how the matrices are ordered,
        that is taken care of). An exception is thrown if the number of rows
        is different in the matrices. */
    void rightAppendPackedMatrix(const CoinPackedMatrix& matrix);
    /** Append the argument to the "bottom" of the current matrix. Imagine this
        as adding new rows (don't worry about how the matrices are ordered,
        that is taken care of). An exception is thrown if the number of columns
        is different in the matrices. */
    void bottomAppendPackedMatrix(const CoinPackedMatrix& matrix);
  
    /** Delete the columns whose indices are listed in <code>indDel</code>. */
    void deleteCols(const int numDel, const int * indDel);
    /** Delete the rows whose indices are listed in <code>indDel</code>. */
    void deleteRows(const int numDel, const int * indDel);

    /** Replace the elements of a vector.  The indices remain the same.
	At most the number specified will be replaced.
        The index is between 0 and major dimension of matrix */
    void replaceVector(const int index,
		       const int numReplace, const double * newElements);
    /** Modify one element of packed matrix.  An element may be added.
        This works for either ordering
	If the new element is zero it will be deleted unless
	keepZero true */
    void modifyCoefficient(int row, int column, double newElement,
			   bool keepZero=false);
    /** Return one element of packed matrix.
        This works for either ordering
	If it is not present will return 0.0 */
    double getCoefficient(int row, int column) const;

    /** Eliminate all elements in matrix whose 
	absolute value is less than threshold.
	The column starts are not affected.  Returns number of elements
	eliminated.  Elements eliminated are at end of each vector
    */
    int compress(double threshold);
    /** Eliminate all duplicate AND small elements in matrix 
	The column starts are not affected.  Returns number of elements
	eliminated.  
    */
    int eliminateDuplicates(double threshold);
    /** Sort all columns so indices are increasing.in each column */
    void orderMatrix();
  //@}

  //---------------------------------------------------------------------------
  /**@name Methods that reorganize the whole matrix */
  //@{
    /** Remove the gaps from the matrix if there were any */
    void removeGaps();
 
    /** Extract a submatrix from matrix. Those major-dimension vectors of
	the matrix comprise the submatrix whose indices are given in the
	arguments. Does not allow duplicates. */
    void submatrixOf(const CoinPackedMatrix& matrix,
		     const int numMajor, const int * indMajor);
    /** Extract a submatrix from matrix. Those major-dimension vectors of
	the matrix comprise the submatrix whose indices are given in the
	arguments. Allows duplicates and keeps order. */
    void submatrixOfWithDuplicates(const CoinPackedMatrix& matrix,
		     const int numMajor, const int * indMajor);
#if 0
    /** Extract a submatrix from matrix. Those major/minor-dimension vectors of
	the matrix comprise the submatrix whose indices are given in the
	arguments. */
    void submatrixOf(const CoinPackedMatrix& matrix,
		     const int numMajor, const int * indMajor,
		     const int numMinor, const int * indMinor);
#endif

    /** Copy method. This method makes an exact replica of the argument,
        including the extra space parameters. */
    void copyOf(const CoinPackedMatrix& rhs);
    /** Copy the arguments to the matrix. If <code>len</code> is a NULL pointer
        then the matrix is assumed to have no gaps in it and <code>len</code>
        will be created accordingly. */
    void copyOf(const bool colordered,
 	       const int minor, const int major, const CoinBigIndex numels,
 	       const double * elem, const int * ind,
 	       const CoinBigIndex * start, const int * len,
 	       const double extraMajor=0.0, const double extraGap=0.0);
    /** Reverse copy method. This method makes an exact replica of the
        argument, but the major ordering reversed. The extra space parameters
        are copied and reversed, too. */
    void reverseOrderedCopyOf(const CoinPackedMatrix& rhs);
    /** Assign the arguments to the matrix. If <code>len</code> is a NULL
	pointer then the matrix is assumed to have no gaps in it and
	<code>len</code> will be created accordingly. <br>
        <strong>NOTE 1</strong>: After this method returns the pointers
        passed to the method will be NULL pointers! <br>
        <strong>NOTE 2</strong>: When the matrix is eventually destructed the
        arrays will be deleted by <code>delete[]</code>. Hence one should use
        this method ONLY if all array swere allocated by <code>new[]</code>! */
    void assignMatrix(const bool colordered,
 		     const int minor, const int major, 
		      const CoinBigIndex numels,
 		     double *& elem, int *& ind,
 		     CoinBigIndex *& start, int *& len,
 		     const int maxmajor = -1, const CoinBigIndex maxsize = -1);
 
 
 
    /** Assignment operator. This copies out the data, but uses the current
        matrix's extra space parameters. */
    CoinPackedMatrix & operator=(const CoinPackedMatrix& rhs);
 
    /** Reverse the ordering of the packed matrix. */
    void reverseOrdering();
    /** Transpose the matrix. <br>
        NOTE: All this routine does is to flip the ordering! Of course, then
        the matrix describes the transposed matrix. To get the matrix
        physically transposed (e.g., for a column ordered matrix to get the
        transpose in column ordered format) one has to invoke this method AND
        <code>reverseOrdering()</code>. */
    void transpose();
 
    /** Swap the content of the two packed matrix. */
    void swap(CoinPackedMatrix& matrix);
   
  //@}

  //---------------------------------------------------------------------------
  /**@name Matrix times vector methods */
  //@{
    /** Return <code>A * x</code> in <code>y</code>.
        @pre <code>x</code> must be of size <code>numColumns()</code>
        @pre <code>y</code> must be of size <code>numRows()</code> */
    void times(const double * x, double * y) const;
#ifndef CLP_NO_VECTOR
    /** Return <code>A * x</code> in <code>y</code>. Same as the previous
        method, just <code>x</code> is given in the form of a packed vector. */
    void times(const CoinPackedVectorBase& x, double * y) const;
#endif
    /** Return <code>x * A</code> in <code>y</code>.
        @pre <code>x</code> must be of size <code>numRows()</code>
        @pre <code>y</code> must be of size <code>numColumns()</code> */
    void transposeTimes(const double * x, double * y) const;
#ifndef CLP_NO_VECTOR
    /** Return <code>x * A</code> in <code>y</code>. Same as the previous
        method, just <code>x</code> is given in the form of a packed vector. */
    void transposeTimes(const CoinPackedVectorBase& x, double * y) const;
#endif
  //@}

  //---------------------------------------------------------------------------
  /**@name Helper functions used internally, but maybe useful externally.

     These methods do not worry about testing whether the packed matrix is
     row or column major ordered; they operate under the assumption that the
     correct version is invoked. In fact, a number of other methods simply
     just call one of these after testing the ordering of the matrix. */
  //@{

    //-------------------------------------------------------------------------
    /**@name Queries */
    //@{
      /** Count the number of entries in every minor-dimension vector and
	  return an array containing these lengths. The returned array is
	  allocated with <code>new int[]</code>, free it with
	  <code>delete[]</code>. */
      int * countOrthoLength() const;
      /** Major dimension. For row ordered matrix this would be the number of
          rows. */
      int getMajorDim() const { return majorDim_; }
      /** Minor dimension. For row ordered matrix this would be the number of
	  columns. */
      int getMinorDim() const { return minorDim_; }
      /** Current maximum for major dimension. For row ordered matrix this many
          rows can be added without reallocating the vector related to the
	  major dimension (<code>start_</code> and <code>length_</code>). */
      int getMaxMajorDim() const { return maxMajorDim_; }

      /** Dump the matrix on stdout. When in dire straits this method can
	  help. */
      void dumpMatrix(const char* fname = NULL) const;

      /// Print a single matrix element.
      void printMatrixElement(const int row_val, const int col_val) const;
    //@}

    //-------------------------------------------------------------------------
    /**@name Append vectors. <br>
       When libosi is compiled with a COIN_DEBUG defined then these methods
       throws an exception if the major (minor) vector contains an index
       that's larger than the minor (major) dimension (-1). Otherwise the
       methods assume that every index fits into the matrix. */
    //@{
#ifndef CLP_NO_VECTOR
      /** Append a major-dimension vector to the end of the matrix. */
      void appendMajorVector(const CoinPackedVectorBase& vec);
#endif
      /** Append a major-dimension vector to the end of the matrix. */
      void appendMajorVector(const int vecsize, const int *vecind,
			     const double *vecelem);
#ifndef CLP_NO_VECTOR
      /** Append several major-dimensonvectors to the end of the matrix */
      void appendMajorVectors(const int numvecs,
			      const CoinPackedVectorBase * const * vecs);

      /** Append a minor-dimension vector to the end of the matrix. */
      void appendMinorVector(const CoinPackedVectorBase& vec);
#endif
      /** Append a minor-dimension vector to the end of the matrix. */
      void appendMinorVector(const int vecsize, const int *vecind,
			     const double *vecelem);
#ifndef CLP_NO_VECTOR
      /** Append several minor-dimensonvectors to the end of the matrix */
      void appendMinorVectors(const int numvecs,
			      const CoinPackedVectorBase * const * vecs);
#endif
    //@}

    //-------------------------------------------------------------------------
    /**@name Append matrices.

       We'll document these methods assuming that the current matrix is
       column major ordered (Hence in the <code>...SameOrdered()</code>
       methods the argument is column ordered, in the
       <code>OrthoOrdered()</code> methods the argument is row ordered.)
    */
    //@{
      /** Append the columns of the argument to the right end of this matrix.
	  @pre <code>minorDim_ == matrix.minorDim_</code> <br>
	  This method throws an exception if the minor dimensions are not the
	  same. */
      void majorAppendSameOrdered(const CoinPackedMatrix& matrix);
      /** Append the columns of the argument to the bottom end of this matrix.
	  @pre <code>majorDim_ == matrix.majorDim_</code> <br>
	  This method throws an exception if the major dimensions are not the
	  same. */
      void minorAppendSameOrdered(const CoinPackedMatrix& matrix);
      /** Append the rows of the argument to the right end of this matrix.
	  @pre <code>minorDim_ == matrix.majorDim_</code> <br>
	  This method throws an exception if the minor dimension of the
	  current matrix is not the same as the major dimension of the
	  argument matrix. */
      void majorAppendOrthoOrdered(const CoinPackedMatrix& matrix);
      /** Append the rows of the argument to the bottom end of this matrix.
	  @pre <code>majorDim_ == matrix.minorDim_</code> <br>
	  This method throws an exception if the major dimension of the
	  current matrix is not the same as the minor dimension of the
	  argument matrix. */
      void minorAppendOrthoOrdered(const CoinPackedMatrix& matrix);
      //@}

      //-----------------------------------------------------------------------
      /**@name Delete vectors */
      //@{
      /** Delete the major-dimension vectors whose indices are listed in
	  <code>indDel</code>. */
      void deleteMajorVectors(const int numDel, const int * indDel);
      /** Delete the minor-dimension vectors whose indices are listed in
	  <code>indDel</code>. */
      void deleteMinorVectors(const int numDel, const int * indDel);
      //@}

      //-----------------------------------------------------------------------
      /**@name Various dot products. */
      //@{
      /** Return <code>A * x</code> (multiplied from the "right" direction) in
	  <code>y</code>.
	  @pre <code>x</code> must be of size <code>majorDim()</code>
	  @pre <code>y</code> must be of size <code>minorDim()</code> */
      void timesMajor(const double * x, double * y) const;
#ifndef CLP_NO_VECTOR
      /** Return <code>A * x</code> (multiplied from the "right" direction) in
	  <code>y</code>. Same as the previous method, just <code>x</code> is
	  given in the form of a packed vector. */
      void timesMajor(const CoinPackedVectorBase& x, double * y) const;
#endif
      /** Return <code>A * x</code> (multiplied from the "right" direction) in
	  <code>y</code>.
	  @pre <code>x</code> must be of size <code>minorDim()</code>
	  @pre <code>y</code> must be of size <code>majorDim()</code> */
      void timesMinor(const double * x, double * y) const;
#ifndef CLP_NO_VECTOR
      /** Return <code>A * x</code> (multiplied from the "right" direction) in
	  <code>y</code>. Same as the previous method, just <code>x</code> is
	  given in the form of a packed vector. */
      void timesMinor(const CoinPackedVectorBase& x, double * y) const;
#endif
      //@}
   //@}

   //--------------------------------------------------------------------------
   /**@name Logical Operations. */
   //@{
#ifndef CLP_NO_VECTOR
   /** Equivalence.
       Two matrices are equivalent if they are both by rows or both by columns,
       they have the same dimensions, and each vector is equivalent. 
       In this method the FloatEqual function operator can be specified. 
   */
   template <class FloatEqual> bool 
   isEquivalent(const CoinPackedMatrix& rhs, const FloatEqual& eq) const
   {
      // Both must be column order or both row ordered and must be of same size
      if ((isColOrdered() ^ rhs.isColOrdered()) ||
	  (getNumCols() != rhs.getNumCols()) ||
	  (getNumRows() != rhs.getNumRows()) ||
	  (getNumElements() != rhs.getNumElements()))
	 return false;
     
      for (int i=getMajorDim()-1; i >= 0; --i) {
        CoinShallowPackedVector pv = getVector(i);
        CoinShallowPackedVector rhsPv = rhs.getVector(i);
        if ( !pv.isEquivalent(rhsPv,eq) )
          return false;
      }
      return true;
   }
   
  bool isEquivalent2(const CoinPackedMatrix& rhs) const;
#else
   /** Equivalence.
       Two matrices are equivalent if they are both by rows or both by columns,
       they have the same dimensions, and each vector is equivalent. 
       In this method the CoinRelFltEq operator is used. 
   */
  bool isEquivalent(const CoinPackedMatrix& rhs, const CoinRelFltEq & eq) const;
#endif
   ///The default equivalence test is that the entries are relatively equal.
   bool isEquivalent(const CoinPackedMatrix& rhs) const
   {
      return isEquivalent(rhs,  CoinRelFltEq());
   }
   //@}

   //--------------------------------------------------------------------------
   /**@name non const methods.
    This is to be used with great care when doing column generation etc */
   //@{
    /** A vector containing the elements in the packed matrix. Note that there
	might be gaps in this list, entries that do not belong to any
	major-dimension vector. To get the actual elements one should look at
	this vector together with vectorStarts and vectorLengths. */
    inline double * getMutableElements() const { return element_; }
    /** A vector containing the minor indices of the elements in the packed
        matrix. Note that there might be gaps in this list, entries that do not
        belong to any major-dimension vector. To get the actual elements one
        should look at this vector together with vectorStarts and
        vectorLengths. */
    inline int * getMutableIndices() const { return index_; }

    /** The positions where the major-dimension vectors start in elements and
        indices. */
    inline CoinBigIndex * getMutableVectorStarts() const { return start_; }
    /** The lengths of the major-dimension vectors. */
    inline int * getMutableVectorLengths() const { return length_; }
    /// NULLify element array - used when space is very tight
    inline void nullElementArray() {element_=NULL;};
    /// NULLify start array - used when space is very tight
    inline void nullStartArray() {start_=NULL;};
    /// NULLify length array - used when space is very tight
    inline void nullLengthArray() {length_=NULL;};
    /// NULLify index array - used when space is very tight
    inline void nullIndexArray() {index_=NULL;};
   //@}

   //--------------------------------------------------------------------------
   /**@name Constructors, destructors and major modifying methods*/
   //@{
   /// Default Constructor creates an empty column ordered packed matrix
   CoinPackedMatrix();

   /// A constructor where the ordering and the gaps are specified
   CoinPackedMatrix(const bool colordered,
		   const double extraMajor, const double extraGap);

   CoinPackedMatrix(const bool colordered,
		   const int minor, const int major, const CoinBigIndex numels,
		   const double * elem, const int * ind,
		   const CoinBigIndex * start, const int * len,
		   const double extraMajor, const double extraGap);

   CoinPackedMatrix(const bool colordered,
		   const int minor, const int major, const CoinBigIndex numels,
		   const double * elem, const int * ind,
		   const CoinBigIndex * start, const int * len);

   /** Create packed matrix from triples.
       If colordered is true then the created matrix will be column ordered.
       Duplicate matrix elements are allowed. The created matrix will have 
       the sum of the duplicates. <br>
       For example if: <br>
         rowIndices[0]=2; colIndices[0]=5; elements[0]=2.0 <br>
         rowIndices[1]=2; colIndices[1]=5; elements[1]=0.5 <br>
       then the created matrix will contain a value of 2.5 in row 2 and column 5.<br>
       The matrix is created without gaps.
   */
   CoinPackedMatrix(const bool colordered,
     const int * rowIndices, 
     const int * colIndices, 
     const double * elements, 
     CoinBigIndex numels ); 

   /// Copy constructor 
   CoinPackedMatrix(const CoinPackedMatrix& m);

  /** Subset constructor (without gaps).  Duplicates are allowed
      and order is as given */
  CoinPackedMatrix (const CoinPackedMatrix & wholeModel,
		    int numberRows, const int * whichRows,
		    int numberColumns, const int * whichColumns);

   /// Destructor 
   virtual ~CoinPackedMatrix();    
   //@}

   //--------------------------------------------------------------------------
protected:
   void gutsOfDestructor();
   void gutsOfCopyOf(const bool colordered,
		     const int minor, const int major, const CoinBigIndex numels,
		     const double * elem, const int * ind,
		     const CoinBigIndex * start, const int * len,
		     const double extraMajor=0.0, const double extraGap=0.0);
   /// When no gaps we can do faster
   void gutsOfCopyOfNoGaps(const bool colordered,
		     const int minor, const int major,
		     const double * elem, const int * ind,
                           const CoinBigIndex * start);
   void gutsOfOpEqual(const bool colordered,
		      const int minor, const int major, const CoinBigIndex numels,
		      const double * elem, const int * ind,
		      const CoinBigIndex * start, const int * len);
   void resizeForAddingMajorVectors(const int numVec, const int * lengthVec);
   void resizeForAddingMinorVectors(const int * addedEntries);
    /** Append a set of rows/columns to the end of the matrix. Returns number of errors
	i.e. if any of the new rows/columns contain an index that's larger than the
	number of columns-1/rows-1 (if numberOther>0) or duplicates
        This version is easy one i.e. adding columns to column ordered */
    int appendMajor(const int number,
		    const CoinBigIndex * starts, const int * index,
                    const double * element, int numberOther=-1);
    /** Append a set of rows/columns to the end of the matrix. Returns number of errors
	i.e. if any of the new rows/columns contain an index that's larger than the
	number of columns-1/rows-1 (if numberOther>0) or duplicates
        This version is harder one i.e. adding columns to row ordered */
    int appendMinor(const int number,
		    const CoinBigIndex * starts, const int * index,
                    const double * element, int numberOther=-1);
private:
   inline CoinBigIndex getLastStart() const {
      return majorDim_ == 0 ? 0 : start_[majorDim_];
   }

   //--------------------------------------------------------------------------
protected:
   /**@name Data members
      The data members are protected to allow access for derived classes. */
   //@{
   /** A flag indicating whether the matrix is column or row major ordered. */
   bool     colOrdered_;
   /** This much times more space should be allocated for each major-dimension
       vector (with respect to the number of entries in the vector) when the
       matrix is resized. The purpose of these gaps is to allow fast insertion
       of new minor-dimension vectors. */
   double   extraGap_;
   /** his much times more space should be allocated for major-dimension
       vectors when the matrix is resized. The purpose of these gaps is to
       allow fast addition of new major-dimension vectors. */
   double   extraMajor_;

   /** List of nonzero element values. The entries in the gaps between
       major-dimension vectors are undefined. */
   double  *element_;
   /** List of nonzero element minor-dimension indices. The entries in the gaps
       between major-dimension vectors are undefined. */
   int     *index_;
   /** Starting positions of major-dimension vectors. */
   CoinBigIndex     *start_;
   /** Lengths of major-dimension vectors. */
   int     *length_;

   /// number of vectors in matrix
   int majorDim_;
   /// size of other dimension
   int minorDim_;
   /// the number of nonzero entries
   CoinBigIndex size_;

   /// max space allocated for major-dimension
   int maxMajorDim_;
   /// max space allocated for entries
   CoinBigIndex maxSize_;
   //@}
};

//#############################################################################
/** A function that tests the methods in the CoinPackedMatrix class. The
    only reason for it not to be a member method is that this way it doesn't
    have to be compiled into the library. And that's a gain, because the
    library should be compiled with optimization on, but this method should be
    compiled with debugging. */
void
CoinPackedMatrixUnitTest();

#endif
