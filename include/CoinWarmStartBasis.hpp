/*! \legal
  Copyright (C) 2000 -- 2003, International Business Machines Corporation
  and others.  All Rights Reserved.
*/

/*! \file CoinWarmStart.hpp
    \brief Declaration of the generic simplex (basis-oriented) warm start
	   class.
*/

#ifndef CoinWarmStartBasis_H
#define CoinWarmStartBasis_H

#include "CoinHelperFunctions.hpp"
#include "CoinWarmStart.hpp"

//#############################################################################

/*! \class CoinWarmStartBasis
    \brief The default COIN simplex (basis-oriented) warm start class
    
    CoinWarmStartBasis provides for a warm start object which contains the
    status of each variable (structural and artificial).
*/

class CoinWarmStartBasis : public CoinWarmStart {
public:

  /** Status codes for variables

    The status vectors are currently packed using two bits per status code,
    four codes per byte. The location of the status information for
    variable \c i is in byte <code>i>>2</code> and occupies bits 0:1
    if <code>i%4 == 0</code>, bits 2:3 if <code>i%4 == 1</code>, etc.
    The functions getStatus(const char*,int) and
    setStatus(char*,int,CoinWarmStartBasis::Status) are provided to hide
    details of the packing.
  */
  enum Status {
    isFree = 0x00,		///< Nonbasic free variable
    basic = 0x01,		///< Basic variable
    atUpperBound = 0x02,	///< Nonbasic at upper bound
    atLowerBound = 0x03		///< Nonbasic at lower bound
  };

public:

/*! \name Methods to get and set information about variables

  The status of variables is kept in a pair of arrays, one for structural
  variables, and one for artificials (aka logicals and slacks). The status
  is coded using the values of the Status enum.

  \sa CoinWarmStartBasis::Status for a description of the packing used in
  the status arrays.
*/
//@{
  /// Return the number of structural variables
  int getNumStructural() const { return numStructural_; }

  /// Return the number of artificial variables
  int getNumArtificial() const { return numArtificial_; }

  /** Return the number of basic structurals
  
    A fast test for an all-slack basis.
  */
  int numberBasicStructurals();

  /// Return the status of the specified structural variable.
  Status getStructStatus(int i) const {
    const int st = (structuralStatus_[i>>2] >> ((i&3)<<1)) & 3;
    return static_cast<CoinWarmStartBasis::Status>(st);
  }

  /// Set the status of the specified structural variable.
  void setStructStatus(int i, Status st) {
    char& st_byte = structuralStatus_[i>>2];
    st_byte &= ~(3 << ((i&3)<<1));
    st_byte |= (st << ((i&3)<<1));
  }

  /** Return the status array for the structural variables
  
    The status information is stored using the codes defined in the
    Status enum, 2 bits per variable, packed 4 variables per byte.
  */
  char * getStructuralStatus() { return structuralStatus_; }

  /** \c const overload for
    \link CoinWarmStartBasis::getStructuralStatus()
    	  getStructuralStatus()
    \endlink
  */
  const char * getStructuralStatus() const { return structuralStatus_; }

  /** As for \link getStructuralStatus() getStructuralStatus \endlink,
      but returns the status array for the artificial variables.
  */
  char * getArtificialStatus() { return artificialStatus_; }

  /// Return the status of the specified artificial variable.
  Status getArtifStatus(int i) const {
    const int st = (artificialStatus_[i>>2] >> ((i&3)<<1)) & 3;
    return static_cast<CoinWarmStartBasis::Status>(st);
  }

  /// Set the status of the specified artificial variable.
  void setArtifStatus(int i, Status st) {
    char& st_byte = artificialStatus_[i>>2];
    st_byte &= ~(3 << ((i&3)<<1));
    st_byte |= (st << ((i&3)<<1));
  }

  /** \c const overload for
    \link CoinWarmStartBasis::getArtificialStatus()
    	  getArtificialStatus()
    \endlink
  */
  const char * getArtificialStatus() const { return artificialStatus_; }

  /** Assign the status vectors to be the warm start information.
  
      In this method the CoinWarmStartBasis object assumes ownership of the
      pointers and upon return the argument pointers will be NULL.
      If copying is desirable, use
      CoinWarmStartBasis(int,int,const char*,const char*).

      \note
      The pointers passed to this method will be
      freed using delete[], so they must be created using new[].
  */
  void assignBasisStatus(int ns, int na, char*& sStat, char*& aStat) ;

//@}

/*! \name Methods to modify the warm start object */
//@{
  void setSize(int ns, int na) ;

  /// Resizes 
  void resize (int newNumberRows, int newNumberColumns);
  /// Deletes rows
  void deleteRows(int number, const int * which);
  /// Deletes columns
  void deleteColumns(int number, const int * which);

  /// Prints in readable format (for debug)
  virtual void print() const;

//@}

/*! \name Constructors, destructors, and related functions */

//@{

  /** Default constructor

    Creates a warm start object representing an empty basis
    (0 rows, 0 columns).
  */
  CoinWarmStartBasis();

  /** Constructs a warm start object with the specified status vectors.

    The parameters are copied.
    Consider assignBasisStatus(int,int,char*&,char*&) if the object should
    assume ownership.

    \sa CoinWarmStartBasis::Status for a description of the packing used in
    the status arrays.
  */
  CoinWarmStartBasis(int ns, int na, const char* sStat, const char* aStat) ;

  /** Copy constructor */
  CoinWarmStartBasis(const CoinWarmStartBasis& ws) ;

  /** `Virtual constructor' */
  virtual CoinWarmStart *clone() const
  {
     return new CoinWarmStartBasis(*this);
  }

  virtual ~CoinWarmStartBasis();

  CoinWarmStartBasis& operator=(const CoinWarmStartBasis& rhs) ;
//@}

private:
  /** \name Private data members

    \sa CoinWarmStartBasis::Status for a description of the packing used in
    the status arrays.
  */
  //@{
    /// The number of structural variables
    int numStructural_;
    /// The number of artificial variables
    int numArtificial_;
    /** The status of the structural variables. */
    char * structuralStatus_;
    /** The status of the artificial variables. */
    char * artificialStatus_;
  //@}
};


/** Get the status of the specified variable in the given status array.
    \relates CoinWarmStartBasis
*/

inline CoinWarmStartBasis::Status getStatus(const char *array, int i)  {
  const int st = (array[i>>2] >> ((i&3)<<1)) & 3;
  return static_cast<CoinWarmStartBasis::Status>(st);
}

/** Set the status of the specified variable in the given status array.
    \relates CoinWarmStartBasis
*/

inline void setStatus(char * array, int i, CoinWarmStartBasis::Status st) {
  char& st_byte = array[i>>2];
  st_byte &= ~(3 << ((i&3)<<1));
  st_byte |= (st << ((i&3)<<1));
}

#endif
