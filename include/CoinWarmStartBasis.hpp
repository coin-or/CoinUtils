// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.

#ifndef CoinWarmStartBasis_H
#define CoinWarmStartBasis_H

#include "CoinHelperFunctions.hpp"
#include "CoinWarmStart.hpp"

//#############################################################################

/** WarmStart information that contains the basic status of each variable
    (structural and artificial) */

class CoinWarmStartBasis : public CoinWarmStart {
public:
  enum Status {
    isFree = 0x00,
    basic = 0x01,
    atUpperBound = 0x02,
    atLowerBound = 0x03
  };

public:
  /// return the number of structural variables
  int getNumStructural() const { return numStructural_; }
  /// return the number of artificial variables
  int getNumArtificial() const { return numArtificial_; }

  /** return the basic status of the structural variables (4 variable per
      byte, 2 bits for each variable) */
  const char * getStructuralStatus() const { return structuralStatus_; }
  char * getStructuralStatus() { return structuralStatus_; }
  /** return the basic status of the artificial variables (4 variable per
      byte, 2 bits for each variable) */
  const char * getArtificialStatus() const { return artificialStatus_; }
  char * getArtificialStatus() { return artificialStatus_; }

  void setSize(int ns, int na) ;

  Status getStructStatus(int i) const {
    const int st = (structuralStatus_[i>>2] >> ((i&3)<<1)) & 3;
    return static_cast<CoinWarmStartBasis::Status>(st);
  }
  Status getArtifStatus(int i) const {
    const int st = (artificialStatus_[i>>2] >> ((i&3)<<1)) & 3;
    return static_cast<CoinWarmStartBasis::Status>(st);
  }
  void setStructStatus(int i, Status st) {
    char& st_byte = structuralStatus_[i>>2];
    st_byte &= ~(3 << ((i&3)<<1));
    st_byte |= (st << ((i&3)<<1));
  }
  void setArtifStatus(int i, Status st) {
    char& st_byte = artificialStatus_[i>>2];
    st_byte &= ~(3 << ((i&3)<<1));
    st_byte |= (st << ((i&3)<<1));
  }
  /** Assign the status vectors to be the warmstart information. In this
      method the object assumes ownership of the pointers and upon return
      the argument pointers will be a NULL pointers. If copying is desirable
      use the constructor. <br>
      <strong>NOTE 1:</strong> The pointers passed to this method will be
      freed using delete[], so they must be created using new[].
      <strong>NOTE 2:</strong> The status vectors must contain 4 basis
      information per byte. The location of the basis information of the i-th
      variable is in byte <code>(sStat[i>>2]</code>, bits 0,1 if i%4 == 0,
      bits 2,3 if i%4 == 1, etc.
  */
  void assignBasisStatus(int ns, int na, char*& sStat, char*& aStat) ;

  CoinWarmStartBasis();

  CoinWarmStartBasis(int ns, int na, const char* sStat, const char* aStat) ;

  CoinWarmStartBasis(const CoinWarmStartBasis& ws) ;

  virtual ~CoinWarmStartBasis();

  CoinWarmStartBasis& operator=(const CoinWarmStartBasis& rhs) ;

  /// Resizes 
  void resize (int newNumberRows, int newNumberColumns);
  /// Deletes rows
  void deleteRows(int number, const int * which);
  /// Deletes columns
  void deleteColumns(int number, const int * which);
  /// Returns number of basic structurals (so you can see if all slack)
  int numberBasicStructurals();

  /// Prints in readable format (for debug)
  void print() const;

private:
  ///@name Private data members
  //@{
    /// the number of structural variables
    int numStructural_;
    /// the number of artificial variables
    int numArtificial_;
    /** the basic status of the structural variables (4 variable per byte, 2
	bits for each variable) */
    char * structuralStatus_;
    /** the basic status of the artificial variables (4 variable per byte, 2
	bits for each variable) */
    char * artificialStatus_;
  //@}
};
// inline after getting array
inline CoinWarmStartBasis::Status getStatus(const char *array, int i)  {
  const int st = (array[i>>2] >> ((i&3)<<1)) & 3;
  return static_cast<CoinWarmStartBasis::Status>(st);
}
inline void setStatus(char * array, int i, CoinWarmStartBasis::Status st) {
  char& st_byte = array[i>>2];
  st_byte &= ~(3 << ((i&3)<<1));
  st_byte |= (st << ((i&3)<<1));
}

#endif
