// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.



#include <cmath>
#include <cassert>
#include <cfloat>
#include <string>
#include <cstdio>
#include <iostream>


#include "CoinPragma.hpp"

#include "CoinHelperFunctions.hpp"

#include "CoinModelUseful.hpp"


//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinModelLink::CoinModelLink () 
 : row_(-1),
   column_(-1),
   value_(0.0),
   position_(-1),
   onRow_(true)
{
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinModelLink::CoinModelLink (const CoinModelLink & rhs) 
  : row_(rhs.row_),
    column_(rhs.column_),
    value_(rhs.value_),
    position_(rhs.position_),
    onRow_(rhs.onRow_)
{
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinModelLink::~CoinModelLink ()
{
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinModelLink &
CoinModelLink::operator=(const CoinModelLink& rhs)
{
  if (this != &rhs) {
    row_ = rhs.row_;
    column_ = rhs.column_;
    value_ = rhs.value_;
    position_ = rhs.position_;
    onRow_ = rhs.onRow_;
  }
  return *this;
}
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinModelHash::CoinModelHash () 
  : names_(NULL),
    hash_(NULL),
    numberItems_(0),
    maximumItems_(0),
    lastSlot_(-1)
{
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinModelHash::CoinModelHash (const CoinModelHash & rhs) 
  : names_(NULL),
    hash_(NULL),
    numberItems_(rhs.numberItems_),
    maximumItems_(rhs.maximumItems_),
    lastSlot_(rhs.lastSlot_)
{
  if (maximumItems_) {
    names_ = new char * [maximumItems_];
    for (int i=0;i<maximumItems_;i++) {
      if (rhs.names_[i])
        names_[i]=strdup(rhs.names_[i]);
      else
        names_[i]=NULL;
    }
    hash_ = CoinCopyOfArray(rhs.hash_,maximumItems_);
  }
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinModelHash::~CoinModelHash ()
{
  for (int i=0;i<maximumItems_;i++) 
    free(names_[i]);
  delete [] names_;
  delete [] hash_;
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinModelHash &
CoinModelHash::operator=(const CoinModelHash& rhs)
{
  if (this != &rhs) {
    for (int i=0;i<maximumItems_;i++) 
      free(names_[i]);
    delete [] names_;
    delete [] hash_;
    numberItems_ = rhs.numberItems_;
    maximumItems_ = rhs.maximumItems_;
    lastSlot_ = rhs.lastSlot_;
    if (maximumItems_) {
      names_ = new char * [maximumItems_];
      for (int i=0;i<maximumItems_;i++) {
        if (rhs.names_[i])
          names_[i]=strdup(rhs.names_[i]);
        else
          names_[i]=NULL;
      }
      hash_ = CoinCopyOfArray(rhs.hash_,maximumItems_);
    } else {
      names_ = NULL;
      hash_ = NULL;
    }
  }
  return *this;
}
// Resize hash (also re-hashs)
void 
CoinModelHash::resize(int maxItems)
{
  assert (numberItems_<=maximumItems_);
  if (maxItems<=maximumItems_)
    return;
  int n=maximumItems_;
  maximumItems_=maxItems;
  char ** names = new char * [maximumItems_];
  int i;
  for ( i=0;i<n;i++) 
    names[i]=names_[i];
  for ( ;i<maximumItems_;i++) 
    names[i]=NULL;
  delete [] names_;
  names_ = names;
  delete [] hash_;
  int maxHash = 4 * maximumItems_;
  hash_ = new CoinModelHashLink [maxHash];
  int ipos;

  for ( i = 0; i < maxHash; i++ ) {
    hash_[i].index = -1;
    hash_[i].next = -1;
  }

  /*
   * Initialize the hash table.  Only the index of the first name that
   * hashes to a value is entered in the table; subsequent names that
   * collide with it are not entered.
   */
  for ( i = 0; i < numberItems_; ++i ) {
    ipos = hashValue ( names_[i]);
    if ( hash_[ipos].index == -1 ) {
      hash_[ipos].index = i;
    }
  }

  /*
   * Now take care of the names that collided in the preceding loop,
   * by finding some other entry in the table for them.
   * Since there are as many entries in the table as there are names,
   * there must be room for them.
   */
  lastSlot_ = -1;
  for ( i = 0; i < numberItems_; ++i ) {
    char *thisName = names[i];
    ipos = hashValue ( thisName);

    while ( true ) {
      int j1 = hash_[ipos].index;

      if ( j1 == i )
	break;
      else {
	char *thisName2 = names[j1];

	if ( strcmp ( thisName, thisName2 ) == 0 ) {
	  printf ( "** duplicate name %s\n", names[i] );
          abort();
	  break;
	} else {
	  int k = hash_[ipos].next;

	  if ( k == -1 ) {
	    while ( true ) {
	      ++lastSlot_;
	      if ( lastSlot_ > numberItems_ ) {
		printf ( "** too many names\n" );
                abort();
		break;
	      }
	      if ( hash_[lastSlot_].index == -1 ) {
		break;
	      }
	    }
	    hash_[ipos].next = lastSlot_;
	    hash_[lastSlot_].index = i;
	    break;
	  } else {
	    ipos = k;
	    /* nothing worked - try it again */
	  }
	}
      }
    }
  }
  
}
// Returns index or -1
int 
CoinModelHash::hash(const char * name) const
{
  int found = -1;

  int ipos;

  /* default if we don't find anything */
  if ( !numberItems_ )
    return -1;

  ipos = hashValue ( name );
  while ( true ) {
    int j1 = hash_[ipos].index;

    if ( j1 >= 0 ) {
      char *thisName2 = names_[j1];

      if ( strcmp ( name, thisName2 ) != 0 ) {
	int k = hash_[ipos].next;

	if ( k != -1 )
	  ipos = k;
	else
	  break;
      } else {
	found = j1;
	break;
      }
    } else {
      found = -1;
      break;
    }
  }
  return found;
}
// Adds to hash
void 
CoinModelHash::addHash(int index, const char * name)
{
  // resize if necessary
  if (numberItems_>=maximumItems_) 
    resize(1000+3*numberItems_/2);
  assert (!names_[index]);
  names_[index]=strdup(name);
  int ipos = hashValue ( name);
  if ( hash_[ipos].index == -1 ) {
    hash_[ipos].index = index;
  } else {
    while ( true ) {
      int j1 = hash_[ipos].index;
      
      if ( j1 == index )
	break;
      else {
	char *thisName2 = names_[j1];

	if ( strcmp ( name, thisName2 ) == 0 ) {
	  printf ( "** duplicate name %s\n", names_[index] );
          abort();
	  break;
	} else {
	  int k = hash_[ipos].next;

	  if ( k == -1 ) {
	    while ( true ) {
	      ++lastSlot_;
	      if ( lastSlot_ > numberItems_ ) {
		printf ( "** too many names\n" );
                abort();
		break;
	      }
	      if ( hash_[lastSlot_].index == -1 ) {
		break;
	      }
	    }
	    hash_[ipos].next = lastSlot_;
	    hash_[lastSlot_].index = index;
	    break;
	  } else {
	    ipos = k;
	    /* nothing worked - try it again */
	  }
	}
      }
    }
  }
  numberItems_++;
}
// Deletes from hash
void 
CoinModelHash::deleteHash(int index)
{
  if (index<numberItems_&&names_[index]) {
    
    int ilast=-1;
    int ipos = hashValue ( names_[index] );

    while ( ipos>=0 ) {
      int j1 = hash_[ipos].index;
      if ( j1!=index) {
        ilast = ipos;
        ipos = hash_[ipos].next;
      } else {
        break;
      }
    }
    assert (ipos>=0);
    free(names_[index]);
    names_[index]=NULL;
    if (ilast>=0) {
      hash_[ilast].next=hash_[ipos].next;
    } else {
      int inext = hash_[ipos].next;
      if (inext>=0) {
        hash_[ipos].index=hash_[inext].index;
        hash_[ipos].next=hash_[inext].next;
        ipos=inext;
      }        
    }
    hash_[ipos].index=-1;
    hash_[ipos].next=-1;
  }
}
// Returns name at position (or NULL)
const char * 
CoinModelHash::name(int which) const
{
  if (which<numberItems_)
    return names_[which];
  else
    return NULL;
}
// Returns a hash value
int 
CoinModelHash::hashValue(const char * name) const
{
  static int mmult[] = {
    262139, 259459, 256889, 254291, 251701, 249133, 246709, 244247,
    241667, 239179, 236609, 233983, 231289, 228859, 226357, 223829,
    221281, 218849, 216319, 213721, 211093, 208673, 206263, 203773,
    201233, 198637, 196159, 193603, 191161, 188701, 186149, 183761,
    181303, 178873, 176389, 173897, 171469, 169049, 166471, 163871,
    161387, 158941, 156437, 153949, 151531, 149159, 146749, 144299,
    141709, 139369, 136889, 134591, 132169, 129641, 127343, 124853,
    122477, 120163, 117757, 115361, 112979, 110567, 108179, 105727,
    103387, 101021, 98639, 96179, 93911, 91583, 89317, 86939, 84521,
    82183, 79939, 77587, 75307, 72959, 70793, 68447, 66103
  };
  static int lengthMult = (int) (sizeof(mmult) / sizeof(int));
  int n = 0;
  int j;
  int length =  (int) strlen(name);
  while (length) {
    int length2 = CoinMin( length,lengthMult);
    for ( j = 0; j < length2; ++j ) {
      int iname = name[j];
      n += mmult[j] * iname;
    }
    name+=length2;
    length-=length2;
  }
  int maxHash = 4 * maximumItems_;
  return ( abs ( n ) % maxHash );	/* integer abs */
}
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################
//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinModelHash2::CoinModelHash2 () 
 :   hash_(NULL),
    numberItems_(0),
    maximumItems_(0),
    lastSlot_(-1)
{
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinModelHash2::CoinModelHash2 (const CoinModelHash2 & rhs) 
  : hash_(NULL),
    numberItems_(rhs.numberItems_),
    maximumItems_(rhs.maximumItems_),
    lastSlot_(rhs.lastSlot_)
{
  if (maximumItems_) {
    hash_ = CoinCopyOfArray(rhs.hash_,maximumItems_);
  }
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinModelHash2::~CoinModelHash2 ()
{
  delete [] hash_;
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinModelHash2 &
CoinModelHash2::operator=(const CoinModelHash2& rhs)
{
  if (this != &rhs) {
    delete [] hash_;
    numberItems_ = rhs.numberItems_;
    maximumItems_ = rhs.maximumItems_;
    lastSlot_ = rhs.lastSlot_;
    if (maximumItems_) {
      hash_ = CoinCopyOfArray(rhs.hash_,maximumItems_);
    } else {
      hash_ = NULL;
    }
  }
  return *this;
}
// Resize hash (also re-hashs)
void 
CoinModelHash2::resize(int maxItems, const CoinModelTriple * triples)
{
  assert (numberItems_<=maximumItems_);
  if (maxItems<=maximumItems_)
    return;
  maximumItems_=maxItems;
  delete [] hash_;
  int maxHash = 2 * maximumItems_;
  hash_ = new CoinModelHashLink [maxHash];
  int ipos;
  int i;
  for ( i = 0; i < maxHash; i++ ) {
    hash_[i].index = -1;
    hash_[i].next = -1;
  }

  /*
   * Initialize the hash table.  Only the index of the first name that
   * hashes to a value is entered in the table; subsequent names that
   * collide with it are not entered.
   */
  for ( i = 0; i < numberItems_; ++i ) {
    int row = (int) triples[i].row;
    int column = triples[i].column;
    if (column>=0) {
      ipos = hashValue ( row, column);
      if ( hash_[ipos].index == -1 ) {
        hash_[ipos].index = i;
      }
    }
  }

  /*
   * Now take care of the entries that collided in the preceding loop,
   * by finding some other entry in the table for them.
   * Since there are as many entries in the table as there are entries,
   * there must be room for them.
   */
  lastSlot_ = -1;
  for ( i = 0; i < numberItems_; ++i ) {
    int row = (int) triples[i].row;
    int column = triples[i].column;
    if (column>=0) {
      ipos = hashValue ( row, column);

      while ( true ) {
        int j1 = hash_[ipos].index;
        
        if ( j1 == i )
          break;
        else {
          int row2 = (int) triples[j1].row;
          int column2 = triples[j1].column;
          if ( row==row2&&column==column2 ) {
            printf ( "** duplicate entry %d %d\n", row,column );
            abort();
            break;
          } else {
            int k = hash_[ipos].next;
            
            if ( k == -1 ) {
              while ( true ) {
                ++lastSlot_;
                if ( lastSlot_ > numberItems_ ) {
                  printf ( "** too many entries\n" );
                  abort();
                  break;
                }
                if ( hash_[lastSlot_].index == -1 ) {
                  break;
                }
              }
              hash_[ipos].next = lastSlot_;
              hash_[lastSlot_].index = i;
              break;
            } else {
              ipos = k;
              /* nothing worked - try it again */
            }
          }
	}
      }
    }
  }
  
}
// Returns index or -1
int 
CoinModelHash2::hash(int row, int column, const CoinModelTriple * triples) const
{
  int found = -1;

  int ipos;

  /* default if we don't find anything */
  if ( !numberItems_ )
    return -1;

  ipos = hashValue ( row, column );
  while ( true ) {
    int j1 = hash_[ipos].index;

    if ( j1 >= 0 ) {
      int row2 = (int) triples[j1].row;
      int column2 = triples[j1].column;
      if ( row!=row2||column!=column2 ) {
	int k = hash_[ipos].next;
        
	if ( k != -1 )
	  ipos = k;
	else
	  break;
      } else {
	found = j1;
	break;
      }
    } else {
      found = -1;
      break;
    }
  }
  return found;
}
// Adds to hash
void 
CoinModelHash2::addHash(int index, int row, int column, const CoinModelTriple * triples)
{
  // resize if necessary
  if (numberItems_>=maximumItems_) 
    resize(1000+3*numberItems_/2, triples);
  int ipos = hashValue ( row, column);
  if ( hash_[ipos].index == -1 ) {
    hash_[ipos].index = index;
  } else {
    while ( true ) {
      int j1 = hash_[ipos].index;
      
      if ( j1 == index )
	break;
      else {
        int row2 = (int) triples[j1].row;
        int column2 = triples[j1].column;
        if ( row==row2&&column==column2 ) {
	  printf ( "** duplicate entry %d %d\n", row, column );
          abort();
	  break;
	} else {
	  int k = hash_[ipos].next;

	  if ( k == -1 ) {
	    while ( true ) {
	      ++lastSlot_;
	      if ( lastSlot_ > numberItems_ ) {
		printf ( "** too many entrys\n" );
                abort();
		break;
	      }
	      if ( hash_[lastSlot_].index == -1 ) {
		break;
	      }
	    }
	    hash_[ipos].next = lastSlot_;
	    hash_[lastSlot_].index = index;
	    break;
	  } else {
	    ipos = k;
	    /* nothing worked - try it again */
	  }
	}
      }
    }
  }
  numberItems_++;
}
// Deletes from hash
void 
CoinModelHash2::deleteHash(int index,int row, int column)
{
  if (index<numberItems_) {
    
    int ilast=-1;
    int ipos = hashValue ( row, column );

    while ( ipos>=0 ) {
      int j1 = hash_[ipos].index;
      if ( j1!=index) {
        ilast = ipos;
        ipos = hash_[ipos].next;
      } else {
        break;
      }
    }
    if (ipos>=0) {
      if (ilast>=0) {
        hash_[ilast].next=hash_[ipos].next;
      } else {
        int inext = hash_[ipos].next;
        if (inext>=0) {
          hash_[ipos].index=hash_[inext].index;
          hash_[ipos].next=hash_[inext].next;
          ipos=inext;
        }        
      }
      hash_[ipos].index=-1;
      hash_[ipos].next=-1;
    }
  }
}
// Returns a hash value
int 
CoinModelHash2::hashValue(int row, int column) const
{
  static int mmult[] = {
    262139, 259459, 256889, 254291, 251701, 249133, 246709, 244247,
    241667, 239179, 236609, 233983, 231289, 228859, 226357, 223829
  };
  char tempChar[8];
  
  int n = 0;
  unsigned int j;
  int * temp = (int *) tempChar;
  *temp=row;
  for ( j = 0; j < sizeof(int); ++j ) {
    int itemp = temp[j];
    n += mmult[j] * itemp;
  }
  *temp=column;
  for ( j = 0; j < sizeof(int); ++j ) {
    int itemp = temp[j];
    n += mmult[j+8] * itemp;
  }
  int maxHash = 2 * maximumItems_;
  return ( abs ( n ) % maxHash );	/* integer abs */
}
//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinModelLinkedList::CoinModelLinkedList () 
  : previous_(NULL),
    next_(NULL),
    first_(NULL),
    last_(NULL),
    numberMajor_(0),
    maximumMajor_(0),
    numberElements_(0),
    maximumElements_(0),
    type_(-1)
{
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinModelLinkedList::CoinModelLinkedList (const CoinModelLinkedList & rhs) 
  : numberMajor_(rhs.numberMajor_),
    maximumMajor_(rhs.maximumMajor_),
    numberElements_(rhs.numberElements_),
    maximumElements_(rhs.maximumElements_),
    type_(rhs.type_)
{
  if (maximumMajor_) {
    previous_ = CoinCopyOfArray(rhs.previous_,maximumElements_);
    next_ = CoinCopyOfArray(rhs.next_,maximumElements_);
    first_ = CoinCopyOfArray(rhs.first_,maximumMajor_+1);
    last_ = CoinCopyOfArray(rhs.last_,maximumMajor_+1);
  } else {
    previous_ = NULL;
    next_ = NULL;
    first_ = NULL;
    last_ = NULL;
  }
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinModelLinkedList::~CoinModelLinkedList ()
{
  delete [] previous_;
  delete [] next_;
  delete [] first_;
  delete [] last_;
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinModelLinkedList &
CoinModelLinkedList::operator=(const CoinModelLinkedList& rhs)
{
  if (this != &rhs) {
    delete [] previous_;
    delete [] next_;
    delete [] first_;
    delete [] last_;
    numberMajor_ = rhs.numberMajor_;
    maximumMajor_ = rhs.maximumMajor_;
    numberElements_ = rhs.numberElements_;
    maximumElements_ = rhs.maximumElements_;
    type_ = rhs.type_;
    if (maximumMajor_) {
      previous_ = CoinCopyOfArray(rhs.previous_,maximumElements_);
      next_ = CoinCopyOfArray(rhs.next_,maximumElements_);
      first_ = CoinCopyOfArray(rhs.first_,maximumMajor_+1);
      last_ = CoinCopyOfArray(rhs.last_,maximumMajor_+1);
    } else {
      previous_ = NULL;
      next_ = NULL;
      first_ = NULL;
      last_ = NULL;
    }
  }
  return *this;
}
// Resize list - for row list maxMajor is maximum rows
void 
CoinModelLinkedList::resize(int maxMajor,int maxElements)
{
  maxMajor=CoinMax(maxMajor,maximumMajor_);
  maxElements=CoinMax(maxElements,maximumElements_);
  if (maxMajor>maximumMajor_) {
    assert (maximumMajor_);
    int * first = new int [maxMajor+1];
    memcpy(first,first_,maximumMajor_*sizeof(int));
    int free = first_[maximumMajor_];
    first[maximumMajor_]=-1;
    first[maxMajor]=free;
    delete [] first_;
    first_=first;
    int * last = new int [maxMajor+1];
    memcpy(last,last_,maximumMajor_*sizeof(int));
    free = last_[maximumMajor_];
    last[maximumMajor_]=-1;
    last[maxMajor]=free;
    delete [] last_;
    last_=last;
    maximumMajor_ = maxMajor;
  }
  if ( maxElements>maximumElements_) {
    int * previous = new int [maxElements];
    memcpy(previous,previous_,numberElements_*sizeof(int));
    delete [] previous_;
    previous_=previous;
    int * next = new int [maxElements];
    memcpy(next,next_,numberElements_*sizeof(int));
    delete [] next_;
    next_=next;
    maximumElements_=maxElements;
  }
}
// Create list - for row list maxMajor is maximum rows
void 
CoinModelLinkedList::create(int maxMajor,int maxElements,
                            int numberMajor,int numberMinor, int type,
                            int numberElements, const CoinModelTriple * triples)
{
  maxMajor=CoinMax(maxMajor,maximumMajor_);
  maxElements=CoinMax(maxElements,maximumElements_);
  type_=type;
  assert (!previous_);
  previous_ = new int [maxElements];
  next_ = new int [maxElements];
  maximumElements_=maxElements;
  assert (maxElements>=numberElements);
  assert (maxMajor>0&&!maximumMajor_);
  first_ = new int[maxMajor+1];
  last_ = new int[maxMajor+1];
  assert (numberElements>=0);
  numberElements_=numberElements;
  maximumMajor_ = maxMajor;
  // do lists
  int i;
  for (i=0;i<numberMajor+1;i++) {
    first_[i]=-1;
    last_[i]=-1;
  }
  int freeChain=-1;
  for (i=0;i<numberElements;i++) {
    if (triples[i].column>=0) {
      int iMajor;
      int iMinor;
      if (!type_) {
        // for rows
        iMajor=(int) triples[i].row;
        iMinor=triples[i].column;
      } else {
        iMinor=(int) triples[i].row;
        iMajor=triples[i].column;
      }
      if (first_[iMajor]>=0) {
        // not first
        int j=last_[iMajor];
#ifndef NDEBUG
        // make sure in order - I think this will be true
        int jMajor;
        int jMinor;
        if (!type_) {
          // for rows
          jMajor=(int) triples[j].row;
          jMinor=triples[j].column;
        } else {
          jMinor=(int) triples[j].row;
          jMajor=triples[j].column;
        }
        assert (iMajor==jMajor);
        assert (iMinor>jMinor);
#endif        
        next_[j]=i;
        previous_[i]=j;
      } else {
        // first
        first_[iMajor]=i;
        previous_[i]=-1;
      }
      last_[iMajor]=i;
    } else {
      // on deleted list
      if (freeChain>=0) {
        next_[freeChain]=i;
        previous_[i]=freeChain;
      } else {
        first_[maximumMajor_]=i;
        previous_[i]=-1;
      }
      freeChain=i;
    }
  }
  // Now clean up
  if (freeChain>=0) {
    next_[freeChain]=-1;
    last_[maximumMajor_]=freeChain;
  }
  for (i=0;i<numberMajor;i++) {
    int k=last_[i];
    if (k>=0) {
      next_[k]=-1;
      last_[i]=k;
    }
  }
}
/* Adds to list - easy case i.e. add row to row list
   Returns where chain starts
*/
int 
CoinModelLinkedList::addEasy(int majorIndex, int numberOfElements, const int * indices,
                             const double * elements, CoinModelTriple * triples,
                             CoinModelHash2 & hash)
{
  assert (majorIndex<maximumMajor_);
  int first=-1;
  if (majorIndex==numberMajor_) {
    first_[majorIndex]=-1;
    last_[majorIndex]=-1;
  }
  if (numberOfElements) {
    bool doHash = hash.numberItems()!=0;
    int lastFree=last_[maximumMajor_];
    int last=last_[majorIndex];
    for (int i=0;i<numberOfElements;i++) {
      int put;
      if (lastFree>=0) {
        put=lastFree;
        lastFree=previous_[lastFree];
      } else {
        put=numberElements_;
        assert (put<maximumElements_);
        numberElements_++;
      }
      if (type_==0) {
        // row
        triples[put].row=majorIndex;
        triples[put].string=0;
        triples[put].column=indices[i];
      } else {
        // column
        triples[put].row=indices[i];
        triples[put].string=0;
        triples[put].column=majorIndex;
      }
      triples[put].value=elements[i];
      if (doHash)
        hash.addHash(put,(int) triples[put].row,triples[put].column,triples);
      if (last>=0) {
        next_[last]=put;
      } else {
        first_[majorIndex]=put;
      }
      previous_[put]=last;
      last=put;
    }
    next_[last]=-1;
    if (last_[majorIndex]<0) {
      // first in row
      first = first_[majorIndex];
    } else {
      first = next_[last_[majorIndex]];
    }
    last_[majorIndex]=last;
    if (lastFree>=0) {
      next_[lastFree]=-1;
      last_[maximumMajor_]=lastFree;
    } else {
      first_[maximumMajor_]=-1;
      last_[maximumMajor_]=-1;
    }
  }
  numberMajor_=CoinMax(numberMajor_,majorIndex+1);
  return first;
}
/* Adds to list - hard case i.e. add row to column list
   Returns where first was put
 */
void 
CoinModelLinkedList::addHard(int minorIndex, int numberOfElements, const int * indices,
                             const double * elements, CoinModelTriple * triples,
                             CoinModelHash2 & hash)
{
  int lastFree=last_[maximumMajor_];
  bool doHash = hash.numberItems()!=0;
  for (int i=0;i<numberOfElements;i++) {
    int put;
    if (lastFree>=0) {
      put=lastFree;
      lastFree=previous_[lastFree];
    } else {
      put=numberElements_;
      assert (put<maximumElements_);
      numberElements_++;
    }
    int other=indices[i];
    if (type_==0) {
      // row
      triples[put].row=other;
      triples[put].string=0;
      triples[put].column=minorIndex;
    } else {
      // column
      triples[put].row=minorIndex;
      triples[put].string=0;
      triples[put].column=other;
    }
    triples[put].value=elements[i];
    if (doHash)
      hash.addHash(put,(int) triples[put].row,triples[put].column,triples);
    int last=last_[other];
    if (last>=0) {
      next_[last]=put;
    } else {
      first_[other]=put;
    }
    previous_[put]=last;
    next_[put]=-1;
    last_[other]=put;
  }
  if (lastFree>=0) {
    next_[lastFree]=-1;
    last_[maximumMajor_]=lastFree;
  } else {
    first_[maximumMajor_]=-1;
    last_[maximumMajor_]=-1;
  }
}
/* Adds to list - hard case i.e. add row to column list
   This is when elements have been added to other copy
*/
void 
CoinModelLinkedList::addHard(int first, const CoinModelTriple * triples,
                             int firstFree, int lastFree,const int * next)
{
  first_[maximumMajor_]=firstFree;
  last_[maximumMajor_]=lastFree;
  int put=first;
  int minorIndex=-1;
  while (put>=0) {
    assert (put<maximumElements_);
    numberElements_=CoinMax(numberElements_,put+1);
    int other;
    if (type_==0) {
      // row
      other=triples[put].row;
      if (minorIndex>=0)
        assert(triples[put].column==minorIndex);
      else
        minorIndex=triples[put].column;
    } else {
      // column
      other=triples[put].column;
      if (minorIndex>=0)
        assert((int) triples[put].row==minorIndex);
      else
        minorIndex=triples[put].row;
    }
    int last=last_[other];
    if (last>=0) {
      next_[last]=put;
    } else {
      first_[other]=put;
    }
    previous_[put]=last;
    next_[put]=-1;
    last_[other]=put;
    put = next[put];
  }
}
