// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
#ifndef CoinIndexedVector_H
#define CoinIndexedVector_H

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <map>

#include "CoinPackedVectorBase.hpp"
#include "CoinSort.hpp"

#define COIN_INDEXED_TINY_ELEMENT 1.0e-50

/** Indexed Vector

This stores values unpacked but apart from that is like CoinPackedVector.  It
is designed to be lightweight in normal use.

Stores vector of indices and associated element values.
Supports sorting of indices.  

Does not support negative indices.

Does NOT support testing for duplicates

For a packed vector getElements is fast and [] is very slow.  Here it is other way round
although getElements is not too bad if used as double * elements = getElements and then used.

Here is a sample usage:
@verbatim
    const int ne = 4;
    int inx[ne] =   {  1,   4,  0,   2 };
    double el[ne] = { 10., 40., 1., 50. };

    // Create vector and set its valuex1
    CoinIndexedVector r(ne,inx,el);

    // access each index and element
    assert( r.getIndices ()[0]== 1  );
    assert( r.getElements()[0]==10. );
    assert( r.getIndices ()[1]== 4  );
    assert( r.getElements()[1]==40. );
    assert( r.getIndices ()[2]== 0  );
    assert( r.getElements()[2]== 1. );
    assert( r.getIndices ()[3]== 2  );
    assert( r.getElements()[3]==50. );

    // access as a full storage vector
    assert( r[ 0]==1. );
    assert( r[ 1]==10.);
    assert( r[ 2]==50.);
    assert( r[ 3]==0. );
    assert( r[ 4]==40.);

    // sort Elements in increasing order
    r.sortIncrElement();

    // access each index and element
    assert( r.getIndices ()[0]== 0  );
    assert( r.getElements()[0]== 1. );
    assert( r.getIndices ()[1]== 1  );
    assert( r.getElements()[1]==10. );
    assert( r.getIndices ()[2]== 4  );
    assert( r.getElements()[2]==40. );
    assert( r.getIndices ()[3]== 2  );
    assert( r.getElements()[3]==50. );    

    // access as a full storage vector
    assert( r[ 0]==1. );
    assert( r[ 1]==10.);
    assert( r[ 2]==50.);
    assert( r[ 3]==0. );
    assert( r[ 4]==40.);

    // Tests for equality and equivalence
    CoinIndexedVector r1;
    r1=r;
    assert( r==r1 );
    assert( r.equivalent(r1) );
    r.sortIncrElement();
    assert( r!=r1 );
    assert( r.equivalent(r1) );

    // Add indexed vectors.
    // Similarly for subtraction, multiplication,
    // and division.
    CoinIndexedVector add = r + r1;
    assert( add[0] ==  1.+ 1. );
    assert( add[1] == 10.+10. );
    assert( add[2] == 50.+50. );
    assert( add[3] ==  0.+ 0. );
    assert( add[4] == 40.+40. );

    assert( r.sum() == 10.+40.+1.+50. );
@endverbatim
*/
class CoinIndexedVector : public CoinPackedVectorBase {
   friend void CoinIndexedVectorUnitTest();
  
public:
   /**@name Get methods. */
   //@{
   /// Get the size
   virtual int getNumElements() const { return nElements_; }
   /// Get indices of elements
   virtual const int * getIndices() const { return indices_; }
   /// Get element values
   virtual const double * getElements() const ;
   /// Get indices of elements
   int * getIndices() { return indices_; }
   /// Get element values
   double * getElements();
   /** Get the vector as a dense vector. This is normal storage method.
       The user should not not delete [] this.
   */
   double * denseVector() const { return elements_; }
   /** Access the i'th element of the full storage vector.
   */
   double & operator[](int i) const; 

   //@}
 
   //-------------------------------------------------------------------
   // Set indices and elements
   //------------------------------------------------------------------- 
   /**@name Set methods */
   //@{
   /// Set the size
   void setNumElements(int value) { nElements_ = value; }
   /// Reset the vector (as if were just created an empty vector).  This leaves arrays!
   void clear();
   /// Reset the vector (as if were just created an empty vector)
   void empty();
   /** Assignment operator. */
   CoinIndexedVector & operator=(const CoinIndexedVector &);
   /** Assignment operator from a CoinPackedVectorBase. <br>
   <strong>NOTE</strong>: This assumes no duplicates */
   CoinIndexedVector & operator=(const CoinPackedVectorBase & rhs);

   /** Borrow ownership of the arguments to this vector.
       Size is the length of the unpacked elements vector. */
  void borrowVector(int size, int numberIndices, int* inds, double* elems);

   /** Return ownership of the arguments to this vector.
       State after is empty .
   */
   void returnVector();

   /** Set vector numberIndices, indices, and elements.
       NumberIndices is the length of both the indices and elements vectors.
       The indices and elements vectors are copied into this class instance's
       member data. Assumed to have no duplicates */
  void setVector(int numberIndices, const int * inds, const double * elems);
  
   /** Set vector size, indices, and elements.
       Size is the length of the unpacked elements vector.
       The indices and elements vectors are copied into this class instance's
       member data. We do not check for duplicate indices */
   void setVector(int size, int numberIndices, const int * inds, const double * elems);
  
   /** Elements set to have the same scalar value */
  void setConstant(int size, const int * inds, double elems);
  
   /** Indices are not specified and are taken to be 0,1,...,size-1 */
  void setFull(int size, const double * elems);

   /** Set an existing element in the indexed vector
       The first argument is the "index" into the elements() array
   */
   void setElement(int index, double element);

   /// Insert an element into the vector
   void insert(int index, double element);
   /** Insert or if exists add an element into the vector
       Any resulting zero elements will be made tiny */
   void add(int index, double element);
   /** Insert or if exists add an element into the vector
       Any resulting zero elements will be made tiny.
       This version does no checking and must be followed by
       stopQuickAdd */
   inline void quickAdd(int index, double element)
               {
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
	       };
   /** Makes nonzero tiny.
       This version does no checking and must be followed by
       stopQuickAdd */
   inline void zero(int index)
               {
		 if (elements_[index]) 
		   elements_[index] = 1.0e-100;
	       };
   /// Stops quickAdd or zero - so sorts etc will work
   void stopQuickAdd();
   /** set all small values to zero and return number remaining
      - < tolerance => 0.0 */
   int clean(double tolerance);
   /// For debug check vector is clear i.e. no elements
   void checkClear();
   /// For debug check vector is clean i.e. elements match indices
   void checkClean();
   /// Append a CoinPackedVector to the end
   void append(const CoinPackedVectorBase & caboose);

   /// Swap values in positions i and j of indices and elements
   void swap(int i, int j); 

   /// Throw away all entries in rows >= newSize
   void truncate(int newSize); 
   //@}
   /**@name Arithmetic operators. */
   //@{
   /// add <code>value</code> to every entry
   void operator+=(double value);
   /// subtract <code>value</code> from every entry
   void operator-=(double value);
   /// multiply every entry by <code>value</code>
   void operator*=(double value);
   /// divide every entry by <code>value</code> (** 0 vanishes)
   void operator/=(double value);
   //@}

   /**@name Sorting */
   //@{ 
   /** Sort the indexed storage vector (increasing indices). */
   void sort()
   { std::sort(indices_,indices_+nElements_); }

   void sortIncrIndex()
   { std::sort(indices_,indices_+nElements_); }

   void sortDecrIndex();
  
   void sortIncrElement();

   void sortDecrElement();

   //@}

  //#############################################################################

  /**@name Arithmetic operators on packed vectors.

   <strong>NOTE</strong>: These methods operate on those positions where at
   least one of the arguments has a value listed. At those positions the
   appropriate operation is executed, Otherwise the result of the operation is
   considered 0.<br>
   <strong>NOTE 2</strong>: Because these methods return an object (they can't
   return a reference, though they could return a pointer...) they are
   <em>very</em> inefficient...
 */
//@{
/// Return the sum of two indexed vectors
CoinIndexedVector operator+(
			   const CoinIndexedVector& op2);

/// Return the difference of two indexed vectors
CoinIndexedVector operator-(
			   const CoinIndexedVector& op2);

/// Return the element-wise product of two indexed vectors
CoinIndexedVector operator*(
			   const CoinIndexedVector& op2);

/// Return the element-wise ratio of two indexed vectors (0.0/0.0 => 0.0) (0 vanishes)
CoinIndexedVector operator/(
			   const CoinIndexedVector& op2);
//@}

   /**@name Memory usage */
   //@{
   /** Reserve space.
       If one knows the eventual size of the indexed vector,
       then it may be more efficient to reserve the space.
   */
   void reserve(int n);
   /** capacity returns the size which could be accomodated without
       having to reallocate storage.
   */
   int capacity() const { return capacity_; }
   //@}

   /**@name Constructors and destructors */
   //@{
   /** Default constructor */
   CoinIndexedVector();
   /** Alternate Constructors - set elements to vector of doubles */
  CoinIndexedVector(int size, const int * inds, const double * elems);
   /** Alternate Constructors - set elements to same scalar value */
  CoinIndexedVector(int size, const int * inds, double element);
   /** Alternate Constructors - construct full storage with indices 0 through
       size-1. */
  CoinIndexedVector(int size, const double * elements);
   /** Copy constructor. */
   CoinIndexedVector(const CoinIndexedVector &);
   /** Copy constructor.2 */
   CoinIndexedVector(const CoinIndexedVector *);
   /** Copy constructor <em>from a PackedVectorBase</em>. */
   CoinIndexedVector(const CoinPackedVectorBase & rhs);
   /** Destructor */
   virtual ~CoinIndexedVector ();
   //@}
    
private:
   /**@name Private methods */
   //@{  
   /// Copy internal date
   void gutsOfSetVector(int size,
			const int * inds, const double * elems);
   void gutsOfSetVector(int size, int numberIndices,
			const int * inds, const double * elems);
   ///
   void gutsOfSetConstant(int size,
			  const int * inds, double value);
   //@}

private:
   /**@name Private member data */
   //@{
   /// Vector indices
   int * indices_;
   ///Vector elements
   double * elements_;
   /// Size of indices and packed elements vectors
   int nElements_;
   ///Vector packed elements (shadow of full array)
   mutable double * packedElements_;
   /// Amount of memory allocated for indices_, and elements_.
   int capacity_;
   //@}
};

//#############################################################################
/** A function that tests the methods in the CoinIndexedVector class. The
    only reason for it not to be a member method is that this way it doesn't
    have to be compiled into the library. And that's a gain, because the
    library should be compiled with optimization on, but this method should be
    compiled with debugging. */
void
CoinIndexedVectorUnitTest();

#endif
