// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <cassert>
#include <cmath>
#include <cfloat>
#include <string>
#include <cstdio>
#include <iostream>

#include "CoinMpsIO.hpp"
#include "CoinMessage.hpp"

#ifdef COIN_USE_ZLIB
#include "zlib.h"
#else
/* just to make the code much nicer (no need for so many ifdefs */
typedef void* gzFile;
#endif

// Plus infinity
#ifndef COIN_DBL_MAX
#define COIN_DBL_MAX DBL_MAX
#endif

/// The following lengths are in decreasing order (for 64 bit etc)
/// Large enough to contain element index
/// This is already defined as CoinBigIndex
/// Large enough to contain column index
typedef int COINColumnIndex;

/// Large enough to contain row index (or basis)
typedef int COINRowIndex;

// We are allowing free format - but there is a limit!
#define MAX_FIELD_LENGTH 100
#define MAX_CARD_LENGTH 5*MAX_FIELD_LENGTH+80

enum COINSectionType { COIN_NO_SECTION, COIN_NAME_SECTION, COIN_ROW_SECTION,
  COIN_COLUMN_SECTION,
  COIN_RHS_SECTION, COIN_RANGES_SECTION, COIN_BOUNDS_SECTION,
  COIN_ENDATA_SECTION, COIN_EOF_SECTION, COIN_UNKNOWN_SECTION
};

enum COINMpsType { COIN_N_ROW, COIN_E_ROW, COIN_L_ROW, COIN_G_ROW,
  COIN_BLANK_COLUMN, COIN_S1_COLUMN, COIN_S2_COLUMN, COIN_S3_COLUMN,
  COIN_INTORG, COIN_INTEND, COIN_SOSEND, COIN_UNSET_BOUND,
  COIN_UP_BOUND, COIN_FX_BOUND, COIN_LO_BOUND, COIN_FR_BOUND,
  COIN_MI_BOUND, COIN_PL_BOUND, COIN_BV_BOUND, COIN_UI_BOUND,
  COIN_SC_BOUND, COIN_UNKNOWN_MPS_TYPE
};

//#############################################################################

static double osi_strtod(char * ptr, char ** output) 
{

  static const double fraction[]=
  {1.0,1.0e-1,1.0e-2,1.0e-3,1.0e-4,1.0e-5,1.0e-6,1.0e-7,1.0e-8,
   1.0e-9,1.0e-10,1.0e-11,1.0e-12,1.0e-13,1.0e-14,1.0e-15,1.0e-16,
   1.0e-17,1.0e-18,1.0e-19};

  static const double exponent[]=
  {1.0e-9,1.0e-8,1.0e-7,1.0e-6,1.0e-5,1.0e-4,1.0e-3,1.0e-2,1.0e-1,
   1.0,1.0e1,1.0e2,1.0e3,1.0e4,1.0e5,1.0e6,1.0e7,1.0e8,1.0e9};

  double value = 0.0;
  char * save = ptr;

  // take off leading white space
  while (*ptr==' '||*ptr=='\t')
    ptr++;
  double sign1=1.0;
  // do + or -
  if (*ptr=='-') {
    sign1=-1.0;
    ptr++;
  } else if (*ptr=='+') {
    ptr++;
  }
  // more white space
  while (*ptr==' '||*ptr=='\t')
    ptr++;
  char thisChar=0;
  while (value<1.0e30) {
    thisChar = *ptr;
    ptr++;
    if (thisChar>='0'&&thisChar<='9') 
      value = value*10.0+thisChar-'0';
    else
      break;
  }
  if (value<1.0e30) {
    if (thisChar=='.') {
      // do fraction
      double value2 = 0.0;
      int nfrac=0;
      while (nfrac<20) {
	thisChar = *ptr;
	ptr++;
	if (thisChar>='0'&&thisChar<='9') {
	  value2 = value2*10.0+thisChar-'0';
	  nfrac++;
	} else {
	  break;
	}
      }
      if (nfrac<20) {
	value += value2*fraction[nfrac];
      } else {
	thisChar='x'; // force error
      }
    }
    if (thisChar=='e'||thisChar=='E') {
      // exponent
      int sign2=1;
      // do + or -
      if (*ptr=='-') {
	sign2=-1;
	ptr++;
      } else if (*ptr=='+') {
	ptr++;
      }
      int value3 = 0;
      while (value3<100) {
	thisChar = *ptr;
	ptr++;
	if (thisChar>='0'&&thisChar<='9') {
	  value3 = value3*10+thisChar-'0';
	} else {
	  break;
	}
      }
      if (value3<200) {
	value3 *= sign2; // power of 10
	if (abs(value3)<10) {
	  // do most common by lookup (for accuracy?)
	  value *= exponent[value3+9];
	} else {
	  value *= pow(10.0,value3);
	}
      } else {
	thisChar='x'; // force error
      }
    } 
    if (thisChar==0||thisChar=='\t'||thisChar==' ') {
      // okay
      *output=ptr;
    } else {
      *output=save;
    }
  } else {
    // bad value
    *output=save;
  }
  return value*sign1;
} 
/// Very simple code for reading MPS data
class MpsCardReader {

public:

  /**@name Constructor and destructor */
  //@{
  /// Constructor expects file to be open - reads down to (and reads) NAME card
  MpsCardReader ( FILE * fp , CoinMpsIO * reader);
#ifdef COIN_USE_ZLIB
  /// This one takes gzFile if fp null
  MpsCardReader ( FILE * fp, gzFile gzfp, CoinMpsIO * reader );
#endif
  /// Destructor
  ~MpsCardReader (  );
  //@}


  /**@name card stuff */
  //@{
  /// Gets next field and returns section type e.g. COIN_COLUMN_SECTION
  COINSectionType nextField (  );
  /// Returns current section type
  inline COINSectionType whichSection (  ) const {
    return section_;
  };
  /// Only for first field on card otherwise BLANK_COLUMN
  /// e.g. COIN_E_ROW
  inline COINMpsType mpsType (  ) const {
    return mpsType_;
  };
  /// Reads and cleans card - taking out trailing blanks - return 1 if EOF
  int cleanCard();
  /// Returns row name of current field
  inline const char *rowName (  ) const {
    return rowName_;
  };
  /// Returns column name of current field
  inline const char *columnName (  ) const {
    return columnName_;
  };
  /// Returns value in current field
  inline double value (  ) const {
    return value_;
  };
  /// Whole card (for printing)
  inline const char *card (  ) const {
    return card_;
  };
  /// Returns card number
  inline CoinBigIndex cardNumber (  ) const {
    return cardNumber_;
  };
  //@}

////////////////// data //////////////////
private:

  /**@name data */
  //@{
  /// Current value
  double value_;
  /// Current card image
  char card_[MAX_CARD_LENGTH];
  /// Current position within card image
  char *position_;
  /// End of card
  char *eol_;
  /// Current COINMpsType
  COINMpsType mpsType_;
  /// Current row name
  char rowName_[MAX_FIELD_LENGTH];
  /// Current column name
  char columnName_[MAX_FIELD_LENGTH];
  /// File pointer
  FILE *fp_;
#ifdef COIN_USE_ZLIB
  /// Compressed file object
  gzFile gzfp_;
#endif
  /// Which section we think we are in
  COINSectionType section_;
  /// Card number
  CoinBigIndex cardNumber_;
  /// Whether free format.  Just for blank RHS etc
  bool freeFormat_;
  /// If all names <= 8 characters then allow embedded blanks
  bool eightChar_;
  /// MpsIO
  CoinMpsIO * reader_;
  /// Message handler
  CoinMessageHandler * handler_;
  /// Messages
  CoinMessages messages_;
  //@}
};

//#############################################################################
// sections
const static char *section[] = {
  "", "NAME", "ROW", "COLUMN", "RHS", "RANGES", "BOUNDS", "ENDATA", " "
};

// what is allowed in each section - must line up with COINSectionType
const static COINMpsType startType[] = {
  COIN_UNKNOWN_MPS_TYPE, COIN_UNKNOWN_MPS_TYPE,
  COIN_N_ROW, COIN_BLANK_COLUMN,
  COIN_BLANK_COLUMN, COIN_BLANK_COLUMN,
  COIN_UP_BOUND, COIN_UNKNOWN_MPS_TYPE,
  COIN_UNKNOWN_MPS_TYPE, COIN_UNKNOWN_MPS_TYPE
};
const static COINMpsType endType[] = {
  COIN_UNKNOWN_MPS_TYPE, COIN_UNKNOWN_MPS_TYPE,
  COIN_BLANK_COLUMN, COIN_UNSET_BOUND,
  COIN_S1_COLUMN, COIN_S1_COLUMN,
  COIN_UNKNOWN_MPS_TYPE, COIN_UNKNOWN_MPS_TYPE,
  COIN_UNKNOWN_MPS_TYPE, COIN_UNKNOWN_MPS_TYPE
};
const static int allowedLength[] = {
  0, 0,
  1, 2,
  0, 0,
  2, 0,
  0, 0
};

// names of types
const static char *mpsTypes[] = {
  "N", "E", "L", "G",
  "  ", "S1", "S2", "S3", "  ", "  ", "  ",
  "  ", "UP", "FX", "LO", "FR", "MI", "PL", "BV", "UI", "SC"
};

int MpsCardReader::cleanCard()
{
  char * getit;
#ifdef COIN_USE_ZLIB
  if (fp_) {
#endif
    // normal file
    getit = fgets ( card_, MAX_CARD_LENGTH, fp_ );
#ifdef COIN_USE_ZLIB
  } else {
    getit = gzgets ( gzfp_, card_, MAX_CARD_LENGTH );
  }
#endif
  
  if ( getit ) {
    cardNumber_++;
    unsigned char * lastNonBlank = (unsigned char *) card_-1;
    unsigned char * image = (unsigned char *) card_;
    while ( *image != '\0' ) {
      if ( *image != '\t' && *image < ' ' ) {
	break;
      } else if ( *image != '\t' && *image != ' ') {
	lastNonBlank = image;
      }
      image++;
    }
    *(lastNonBlank+1)='\0';
    return 0;
  } else {
    return 1;
  }
}

char *
nextBlankOr ( char *image )
{
  char * saveImage=image;
  while ( 1 ) {
    if ( *image == ' ' || *image == '\t' ) {
      break;
    }
    if ( *image == '\0' )
      return NULL;
    image++;
  }
  // Allow for floating - or +.  Will fail if user has that as row name!!
  if (image-saveImage==1&&(*saveImage=='+'||*saveImage=='-')) {
    while ( *image == ' ' || *image == '\t' ) {
      image++;
    }
    image=nextBlankOr(image);
  }
  return image;
}

//  MpsCardReader.  Constructor
MpsCardReader::MpsCardReader (  FILE * fp , CoinMpsIO * reader)
{
  memset ( card_, 0, MAX_CARD_LENGTH );
  position_ = card_;
  eol_ = card_;
  mpsType_ = COIN_UNKNOWN_MPS_TYPE;
  memset ( rowName_, 0, MAX_FIELD_LENGTH );
  memset ( columnName_, 0, MAX_FIELD_LENGTH );
  value_ = 0.0;
  fp_ = fp;
  section_ = COIN_EOF_SECTION;
  cardNumber_ = 0;
  freeFormat_ = false;
  eightChar_ = true;
  reader_ = reader;
  handler_ = reader_->messageHandler();
  messages_ = reader_->messages();
  bool found = false;

  while ( !found ) {
    // need new image

    if ( cleanCard() ) {
      section_ = COIN_EOF_SECTION;
      break;
    }
    if ( !strncmp ( card_, "NAME", 4 ) ) {
      section_ = COIN_NAME_SECTION;
      char *next = card_ + 4;
      handler_->message(COIN_MPS_LINE,messages_)<<cardNumber_
					       <<card_<<CoinMessageEol;
      while ( next != eol_ ) {
	if ( *next == ' ' || *next == '\t' ) {
	  next++;
	} else {
	  break;
	}
      }
      if ( next != eol_ ) {
	char *nextBlank = nextBlankOr ( next );
	char save;

	if ( nextBlank ) {
	  save = *nextBlank;
	  *nextBlank = '\0';
	  strcpy ( columnName_, next );
	  *nextBlank = save;
	  if ( strstr ( nextBlank, "FREE" ) )
	    freeFormat_ = true;
	} else {
	  strcpy ( columnName_, next );
	}
      } else {
	strcpy ( columnName_, "no_name" );
      }
      position_=eol_;
      break;
    } else if ( card_[0] != '*' && card_[0] != '#' ) {
      section_ = COIN_UNKNOWN_SECTION;
      break;
    }
  }
}
#ifdef COIN_USE_ZLIB
// This one takes gzFile if fp null
MpsCardReader::MpsCardReader (  FILE * fp , gzFile gzfp, CoinMpsIO * reader)
{
  memset ( card_, 0, MAX_CARD_LENGTH );
  position_ = card_;
  eol_ = card_;
  mpsType_ = COIN_UNKNOWN_MPS_TYPE;
  memset ( rowName_, 0, MAX_FIELD_LENGTH );
  memset ( columnName_, 0, MAX_FIELD_LENGTH );
  value_ = 0.0;
  fp_ = fp;
  gzfp_ = gzfp;
  section_ = COIN_EOF_SECTION;
  cardNumber_ = 0;
  freeFormat_ = false;
  eightChar_ = true;
  reader_ = reader;
  handler_ = reader_->messageHandler();
  messages_ = reader_->messages();
  bool found = false;

  while ( !found ) {
    // need new image

    if ( cleanCard() ) {
      section_ = COIN_EOF_SECTION;
      break;
    }
    if ( !strncmp ( card_, "NAME", 4 ) ) {
      section_ = COIN_NAME_SECTION;
      char *next = card_ + 4;

      handler_->message(COIN_MPS_LINE,messages_)<<cardNumber_
					       <<card_<<CoinMessageEol;
      while ( next != eol_ ) {
	if ( *next == ' ' || *next == '\t' ) {
	  next++;
	} else {
	  break;
	}
      }
      if ( next != eol_ ) {
	char *nextBlank = nextBlankOr ( next );
	char save;

	if ( nextBlank ) {
	  save = *nextBlank;
	  *nextBlank = '\0';
	  strcpy ( columnName_, next );
	  *nextBlank = save;
	  if ( strstr ( nextBlank, "FREE" ) )
	    freeFormat_ = true;
	} else {
	  strcpy ( columnName_, next );
	}
      } else {
	strcpy ( columnName_, "no_name" );
      }
      break;
    } else if ( card_[0] != '*' && card_[0] != '#' ) {
      section_ = COIN_UNKNOWN_SECTION;
      break;
    }
  }
}
#endif
//  ~MpsCardReader.  Destructor
MpsCardReader::~MpsCardReader (  )
{
#ifdef COIN_USE_ZLIB
  if (!fp_) {
    // no fp_ so must be compressed read
    gzclose(gzfp_);
  }
#endif
  if (fp_)
    fclose ( fp_ );
}

void
strcpyAndCompress ( char *to, const char *from )
{
  int n = strlen ( from );
  int i;
  int nto = 0;

  for ( i = 0; i < n; i++ ) {
    if ( from[i] != ' ' ) {
      to[nto++] = from[i];
    }
  }
  if ( !nto )
    to[nto++] = ' ';
  to[nto] = '\0';
}

//  nextField
COINSectionType
MpsCardReader::nextField (  )
{
  mpsType_ = COIN_BLANK_COLUMN;
  // find next non blank character
  char *next = position_;

  while ( next != eol_ ) {
    if ( *next == ' ' || *next == '\t' ) {
      next++;
    } else {
      break;
    }
  }
  bool gotCard;

  if ( next == eol_ ) {
    gotCard = false;
  } else {
    gotCard = true;
  }
  while ( !gotCard ) {
    // need new image

    if ( cleanCard() ) {
      return COIN_EOF_SECTION;
    }
    if ( card_[0] == ' ' ) {
      // not a section or comment
      position_ = card_;
      eol_ = card_ + strlen ( card_ );
      // get mps type and column name
      // scan to first non blank
      next = card_;
      while ( next != eol_ ) {
	if ( *next == ' ' || *next == '\t' ) {
	  next++;
	} else {
	  break;
	}
      }
      if ( next != eol_ ) {
	char *nextBlank = nextBlankOr ( next );
	int nchar;

	if ( nextBlank ) {
	  nchar = nextBlank - next;
	} else {
	  nchar = -1;
	}
	mpsType_ = COIN_BLANK_COLUMN;
	// special coding if RHS or RANGES, not free format and blanks
	if ( ( section_ != COIN_RHS_SECTION 
	       && section_ != COIN_RANGES_SECTION )
	     || freeFormat_ || strncmp ( card_ + 4, "        ", 8 ) ) {
	  // if columns section only look for first field if MARKER
	  if ( section_ == COIN_COLUMN_SECTION
	       && !strstr ( next, "'MARKER'" ) ) nchar = -1;
	  if ( nchar == allowedLength[section_] ) {
	    //could be a type
	    int i;

	    for ( i = startType[section_]; i < endType[section_]; i++ ) {
	      if ( !strncmp ( next, mpsTypes[i], nchar ) ) {
		mpsType_ = ( COINMpsType ) i;
		break;
	      }
	    }
	    if ( mpsType_ != COIN_BLANK_COLUMN ) {
	      //we know all we need so we can skip over
	      next = nextBlank;
	      while ( next != eol_ ) {
		if ( *next == ' ' || *next == '\t' ) {
		  next++;
		} else {
		  break;
		}
	      }
	      if ( next == eol_ ) {
		// error
		position_ = eol_;
		mpsType_ = COIN_UNKNOWN_MPS_TYPE;
	      } else {
		nextBlank = nextBlankOr ( next );
	      }
	    }
	  }
	  if ( mpsType_ != COIN_UNKNOWN_MPS_TYPE ) {
	    // special coding if BOUND, not free format and blanks
	    if ( section_ != COIN_BOUNDS_SECTION ||
		 freeFormat_ || strncmp ( card_ + 4, "        ", 8 ) ) {
	      char save = '?';

	      if ( !freeFormat_ && eightChar_ && next == card_ + 4 ) {
		if ( eol_ - next >= 8 ) {
		  if ( *( next + 8 ) != ' ' && *( next + 8 ) != '\0' ) {
		    eightChar_ = false;
		  } else {
		    nextBlank = next + 8;
		  }
		  save = *nextBlank;
		  *nextBlank = '\0';
		} else {
		  nextBlank = NULL;
		}
	      } else {
		if ( nextBlank ) {
		  save = *nextBlank;
		  *nextBlank = '\0';
		}
	      }
	      strcpyAndCompress ( columnName_, next );
	      if ( nextBlank ) {
		*nextBlank = save;
		// on to next
		next = nextBlank;
	      } else {
		next = eol_;
	      }
	    } else {
	      // blank bounds name
	      strcpy ( columnName_, "        " );
	    }
	    while ( next != eol_ ) {
	      if ( *next == ' ' || *next == '\t' ) {
		next++;
	      } else {
		break;
	      }
	    }
	    if ( next == eol_ ) {
	      // error unless row section
	      position_ = eol_;
	      value_ = -1.0e100;
	      if ( section_ != COIN_ROW_SECTION )
		mpsType_ = COIN_UNKNOWN_MPS_TYPE;
	    } else {
	      nextBlank = nextBlankOr ( next );
	    }
	    if ( section_ != COIN_ROW_SECTION ) {
	      char save = '?';

	      if ( !freeFormat_ && eightChar_ && next == card_ + 14 ) {
		if ( eol_ - next >= 8 ) {
		  if ( *( next + 8 ) != ' ' && *( next + 8 ) != '\0' ) {
		    eightChar_ = false;
		  } else {
		    nextBlank = next + 8;
		  }
		  save = *nextBlank;
		  *nextBlank = '\0';
		} else {
		  nextBlank = NULL;
		}
	      } else {
		if ( nextBlank ) {
		  save = *nextBlank;
		  *nextBlank = '\0';
		}
	      }
	      strcpyAndCompress ( rowName_, next );
	      if ( nextBlank ) {
		*nextBlank = save;
		// on to next
		next = nextBlank;
	      } else {
		next = eol_;
	      }
	      while ( next != eol_ ) {
		if ( *next == ' ' || *next == '\t' ) {
		  next++;
		} else {
		  break;
		}
	      }
	      // special coding for markers
	      if ( section_ == COIN_COLUMN_SECTION &&
		   !strncmp ( rowName_, "'MARKER'", 8 ) && next != eol_ ) {
		if ( !strncmp ( next, "'INTORG'", 8 ) ) {
		  mpsType_ = COIN_INTORG;
		} else if ( !strncmp ( next, "'INTEND'", 8 ) ) {
		  mpsType_ = COIN_INTEND;
		} else if ( !strncmp ( next, "'SOSORG'", 8 ) ) {
		  if ( mpsType_ == COIN_BLANK_COLUMN )
		    mpsType_ = COIN_S1_COLUMN;
		} else if ( !strncmp ( next, "'SOSEND'", 8 ) ) {
		  mpsType_ = COIN_SOSEND;
		} else {
		  mpsType_ = COIN_UNKNOWN_MPS_TYPE;
		}
		position_ = eol_;
		return section_;
	      }
	      if ( next == eol_ ) {
		// error unless bounds
		position_ = eol_;
		value_ = -1.0e100;
		if ( section_ != COIN_BOUNDS_SECTION )
		  mpsType_ = COIN_UNKNOWN_MPS_TYPE;
	      } else {
		nextBlank = nextBlankOr ( next );
		if ( nextBlank ) {
		  save = *nextBlank;
		  *nextBlank = '\0';
		}
		char * after;
		value_ = osi_strtod(next,&after);
		// see if error
		assert(after>next);
		if ( nextBlank ) {
		  *nextBlank = save;
		  position_ = nextBlank;
		} else {
		  position_ = eol_;
		}
	      }
	    }
	  }
	} else {
	  //blank name in RHS or RANGES
	  strcpy ( columnName_, "        " );
	  char save = '?';

	  if ( !freeFormat_ && eightChar_ && next == card_ + 14 ) {
	    if ( eol_ - next >= 8 ) {
	      if ( *( next + 8 ) != ' ' && *( next + 8 ) != '\0' ) {
		eightChar_ = false;
	      } else {
		nextBlank = next + 8;
	      }
	      save = *nextBlank;
	      *nextBlank = '\0';
	    } else {
	      nextBlank = NULL;
	    }
	  } else {
	    if ( nextBlank ) {
	      save = *nextBlank;
	      *nextBlank = '\0';
	    }
	  }
	  strcpyAndCompress ( rowName_, next );
	  if ( nextBlank ) {
	    *nextBlank = save;
	    // on to next
	    next = nextBlank;
	  } else {
	    next = eol_;
	  }
	  while ( next != eol_ ) {
	    if ( *next == ' ' || *next == '\t' ) {
	      next++;
	    } else {
	      break;
	    }
	  }
	  if ( next == eol_ ) {
	    // error 
	    position_ = eol_;
	    value_ = -1.0e100;
	    mpsType_ = COIN_UNKNOWN_MPS_TYPE;
	  } else {
	    nextBlank = nextBlankOr ( next );
	    value_ = -1.0e100;
	    if ( nextBlank ) {
	      save = *nextBlank;
	      *nextBlank = '\0';
	    }
	    char * after;
	    value_ = osi_strtod(next,&after);
	    // see if error
	    assert(after>next);
	    if ( nextBlank ) {
	      *nextBlank = save;
	      position_ = nextBlank;
	    } else {
	      position_ = eol_;
	    }
	  }
	}
      }
      return section_;
    } else if ( card_[0] != '*' ) {
      // not a comment
      int i;

      handler_->message(COIN_MPS_LINE,messages_)<<cardNumber_
					       <<card_<<CoinMessageEol;
      for ( i = COIN_ROW_SECTION; i < COIN_UNKNOWN_SECTION; i++ ) {
	if ( !strncmp ( card_, section[i], strlen ( section[i] ) ) ) {
	  break;
	}
      }
      position_ = card_;
      eol_ = card_;
      section_ = ( COINSectionType ) i;
      return section_;
    } else {
      // comment
    }
  }
  // we only get here for second field (we could even allow more???)
  {
    char save = '?';
    char *nextBlank = nextBlankOr ( next );

    if ( !freeFormat_ && eightChar_ && next == card_ + 39 ) {
      if ( eol_ - next >= 8 ) {
	if ( *( next + 8 ) != ' ' && *( next + 8 ) != '\0' ) {
	  eightChar_ = false;
	} else {
	  nextBlank = next + 8;
	}
	save = *nextBlank;
	*nextBlank = '\0';
      } else {
	nextBlank = NULL;
      }
    } else {
      if ( nextBlank ) {
	save = *nextBlank;
	*nextBlank = '\0';
      }
    }
    strcpyAndCompress ( rowName_, next );
    // on to next
    if ( nextBlank ) {
      *nextBlank = save;
      next = nextBlank;
    } else {
      next = eol_;
    }
    while ( next != eol_ ) {
      if ( *next == ' ' || *next == '\t' ) {
	next++;
      } else {
	break;
      }
    }
    if ( next == eol_ ) {
      // error
      position_ = eol_;
      mpsType_ = COIN_UNKNOWN_MPS_TYPE;
    } else {
      nextBlank = nextBlankOr ( next );
    }
    if ( nextBlank ) {
      save = *nextBlank;
      *nextBlank = '\0';
    }
    //value_ = -1.0e100;
    char * after;
    value_ = osi_strtod(next,&after);
    // see if error
    assert(after>next);
    if ( nextBlank ) {
      *nextBlank = save;
      position_ = nextBlank;
    } else {
      position_ = eol_;
    }
  }
  return section_;
}

//#############################################################################

static int
hash ( const char *name, int maxsiz, int length )
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
  int n = 0;
  int j;

  for ( j = 0; j < length; ++j ) {
    int iname = name[j];

    n += mmult[j] * iname;
  }
  return ( abs ( n ) % maxsiz );	/* integer abs */
}

//  startHash.  Creates hash list for names
void
CoinMpsIO::startHash ( char **names, const COINColumnIndex number , int section )
{
  names_[section] = names;
  numberHash_[section] = number;
  startHash(section);
}
void
CoinMpsIO::startHash ( int section ) const
{
  char ** names = names_[section];
  COINColumnIndex number = numberHash_[section];
  COINColumnIndex i;
  COINColumnIndex maxhash = 4 * number;
  COINColumnIndex ipos, iput;

  //hash_=(CoinHashLink *) malloc(maxhash*sizeof(CoinHashLink));
  hash_[section] = new CoinHashLink[maxhash];
  
  CoinHashLink * hashThis = hash_[section];

  for ( i = 0; i < maxhash; i++ ) {
    hashThis[i].index = -1;
    hashThis[i].next = -1;
  }

  /*
   * Initialize the hash table.  Only the index of the first name that
   * hashes to a value is entered in the table; subsequent names that
   * collide with it are not entered.
   */
  for ( i = 0; i < number; ++i ) {
    char *thisName = names[i];
    int length = strlen ( thisName );

    ipos = hash ( thisName, maxhash, length );
    if ( hashThis[ipos].index == -1 ) {
      hashThis[ipos].index = i;
    }
  }

  /*
   * Now take care of the names that collided in the preceding loop,
   * by finding some other entry in the table for them.
   * Since there are as many entries in the table as there are names,
   * there must be room for them.
   */
  iput = -1;
  for ( i = 0; i < number; ++i ) {
    char *thisName = names[i];
    int length = strlen ( thisName );

    ipos = hash ( thisName, maxhash, length );

    while ( 1 ) {
      COINColumnIndex j1 = hashThis[ipos].index;

      if ( j1 == i )
	break;
      else {
	char *thisName2 = names[j1];

	if ( strcmp ( thisName, thisName2 ) == 0 ) {
	  printf ( "** duplicate name %s\n", names[i] );
	  break;
	} else {
	  COINColumnIndex k = hashThis[ipos].next;

	  if ( k == -1 ) {
	    while ( 1 ) {
	      ++iput;
	      if ( iput > number ) {
		printf ( "** too many names\n" );
		break;
	      }
	      if ( hashThis[iput].index == -1 ) {
		break;
	      }
	    }
	    hashThis[ipos].next = iput;
	    hashThis[iput].index = i;
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

//  stopHash.  Deletes hash storage
void
CoinMpsIO::stopHash ( int section )
{
  delete [] hash_[section];
  hash_[section] = NULL;
}

//  findHash.  -1 not found
COINColumnIndex
CoinMpsIO::findHash ( const char *name , int section ) const
{
  COINColumnIndex found = -1;

  char ** names = names_[section];
  CoinHashLink * hashThis = hash_[section];
  COINColumnIndex maxhash = 4 * numberHash_[section];
  COINColumnIndex ipos;

  /* default if we don't find anything */
  if ( !maxhash )
    return -1;
  int length = strlen ( name );

  ipos = hash ( name, maxhash, length );
  while ( 1 ) {
    COINColumnIndex j1 = hashThis[ipos].index;

    if ( j1 >= 0 ) {
      char *thisName2 = names[j1];

      if ( strcmp ( name, thisName2 ) != 0 ) {
	COINColumnIndex k = hashThis[ipos].next;

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

//------------------------------------------------------------------
// Get value for infinity
//------------------------------------------------------------------
double CoinMpsIO::getInfinity() const
{
  return infinity_;
}
//------------------------------------------------------------------
// Set value for infinity
//------------------------------------------------------------------
void CoinMpsIO::setInfinity(double value) 
{
  if ( value >= 1.020 ) {
    infinity_ = value;
  } else {
    handler_->message(COIN_MPS_ILLEGAL,messages_)<<"infinity"
						<<value
						<<CoinMessageEol;
  }

}
// Set file name
void CoinMpsIO::setFileName(const char * name)
{
  free(fileName_);
  fileName_=strdup(name);
}
// Get file name
const char * CoinMpsIO::getFileName() const
{
  return fileName_;
}
// Test if current file exists and readable
const bool CoinMpsIO::fileReadable() const
{
  // I am opening it to make sure not odd
  FILE *fp;
  if (strcmp(fileName_,"stdin")) {
    fp = fopen ( fileName_, "r" );
  } else {
    fp = stdin;
  }
  if (!fp) {
    return false;
  } else {
    fclose(fp);
    return true;
  }
}
/* objective offset - this is RHS entry for objective row */
double CoinMpsIO::objectiveOffset() const
{
  return objectiveOffset_;
}
#define MAX_INTEGER 1000000
// Sets default upper bound for integer variables
void CoinMpsIO::setDefaultBound(int value)
{
  if ( value >= 1 && value <=MAX_INTEGER ) {
    defaultBound_ = value;
  } else {
    handler_->message(COIN_MPS_ILLEGAL,messages_)<<"default integer bound"
						<<value
						<<CoinMessageEol;
  }
}
// gets default upper bound for integer variables
int CoinMpsIO::getDefaultBound() const
{
  return defaultBound_;
}
//------------------------------------------------------------------
// Read mps files
//------------------------------------------------------------------
int CoinMpsIO::readMps(const char * filename,  const char * extension)
{
  // clean up later as pointless using strings
  std::string f(filename);
  std::string fullname;
  free(fileName_);
  if (strcmp(filename,"stdin")&&strcmp(filename,"-")) {
    std::string e(extension);
    if (f.find('.')<=0&&strlen(extension)) 
      fullname = f + "." + e;
    else
      fullname = f;
    fileName_=strdup(fullname.c_str());    
  } else {
    fileName_=strdup("stdin");    
  }
  return readMps();
}
int CoinMpsIO::readMps()
{
  FILE *fp=NULL;
  bool goodFile=false;
#ifdef COIN_USE_ZLIB
  gzFile gzfp=NULL;
#endif
  if (strcmp(fileName_,"stdin")) {
#ifdef COIN_USE_ZLIB
    int length=strlen(fileName_);
    if (!strcmp(fileName_+length-3,".gz")) {
      gzfp = gzopen(fileName_,"rb");
      fp = NULL;
      goodFile = (gzfp!=NULL);
    } else {
#endif
      fp = fopen ( fileName_, "r" );
      goodFile = (fp!=NULL);
#ifdef COIN_USE_ZLIB
      if (!goodFile) {
	 std::string fname(fileName_);
	 fname += ".gz";
	 gzfp = gzopen(fname.c_str(),"rb");
	 printf("%s\n", fname.c_str());
	 goodFile = (gzfp!=NULL);
      }
    }
#endif
  } else {
    fp = stdin;
    goodFile = true;
  }
  if (!goodFile) {
    handler_->message(COIN_MPS_FILE,messages_)<<fileName_
					     <<CoinMessageEol;
    return -1;
  }
  bool ifmps;
#ifdef COIN_USE_ZLIB
  MpsCardReader mpsfile ( fp , gzfp, this);
#else
  MpsCardReader mpsfile ( fp , this);
#endif

  if ( mpsfile.whichSection (  ) == COIN_NAME_SECTION ) {
    ifmps = true;
    // save name of section
    free(problemName_);
    problemName_=strdup(mpsfile.columnName());
  } else if ( mpsfile.whichSection (  ) == COIN_UNKNOWN_SECTION ) {
    handler_->message(COIN_MPS_BADFILE1,messages_)<<mpsfile.card()
						 <<fileName_
						 <<CoinMessageEol;
#ifdef COIN_USE_ZLIB
    if (!fp) 
      handler_->message(COIN_MPS_BADFILE2,messages_)<<CoinMessageEol;

#endif
    return -2;
  } else if ( mpsfile.whichSection (  ) != COIN_EOF_SECTION ) {
    // save name of section
    free(problemName_);
    problemName_=strdup(mpsfile.card());
    ifmps = false;
  } else {
    handler_->message(COIN_MPS_EOF,messages_)<<fileName_
					    <<CoinMessageEol;
    return -3;
  }
  CoinBigIndex *start;
  COINRowIndex *row;
  double *element;
  objectiveOffset_ = 0.0;

  int numberErrors = 0;
  int i;

  if ( ifmps ) {
    // mps file - always read in free format
    bool gotNrow = false;

    //get ROWS
    mpsfile.nextField (  ) ;
    assert ( mpsfile.whichSection (  ) == COIN_ROW_SECTION );
    //use malloc etc as I don't know how to do realloc in C++
    numberRows_ = 0;
    numberColumns_ = 0;
    numberElements_ = 0;
    COINRowIndex maxRows = 1000;
    COINMpsType *rowType =

      ( COINMpsType * ) malloc ( maxRows * sizeof ( COINMpsType ) );
    char **rowName = ( char ** ) malloc ( maxRows * sizeof ( char * ) );

    // for discarded free rows
    COINRowIndex maxFreeRows = 100;
    COINRowIndex numberOtherFreeRows = 0;
    char **freeRowName =

      ( char ** ) malloc ( maxFreeRows * sizeof ( char * ) );
    while ( mpsfile.nextField (  ) == COIN_ROW_SECTION ) {
      switch ( mpsfile.mpsType (  ) ) {
      case COIN_N_ROW:
	if ( !gotNrow ) {
	  gotNrow = true;
	  // save name of section
	  free(objectiveName_);
	  objectiveName_=strdup(mpsfile.columnName());
	} else {
	  // add to discard list
	  if ( numberOtherFreeRows == maxFreeRows ) {
	    maxFreeRows = ( 3 * maxFreeRows ) / 2 + 100;
	    freeRowName =
	      ( char ** ) realloc ( freeRowName,

				    maxFreeRows * sizeof ( char * ) );
	  }
	  freeRowName[numberOtherFreeRows] =
	    strdup ( mpsfile.columnName (  ) );
	  numberOtherFreeRows++;
	}
	break;
      case COIN_E_ROW:
      case COIN_L_ROW:
      case COIN_G_ROW:
	if ( numberRows_ == maxRows ) {
	  maxRows = ( 3 * maxRows ) / 2 + 1000;
	  rowType =
	    ( COINMpsType * ) realloc ( rowType,
				       maxRows * sizeof ( COINMpsType ) );
	  rowName =

	    ( char ** ) realloc ( rowName, maxRows * sizeof ( char * ) );
	}
	rowType[numberRows_] = mpsfile.mpsType (  );
	rowName[numberRows_] = strdup ( mpsfile.columnName (  ) );
	numberRows_++;
	break;
      default:
	numberErrors++;
	if ( numberErrors < 100 ) {
	  handler_->message(COIN_MPS_BADIMAGE,messages_)<<mpsfile.cardNumber()
						       <<mpsfile.card()
						       <<CoinMessageEol;
	} else if (numberErrors > 100000) {
	  handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	  return numberErrors;
	}
      }
    }
    assert ( mpsfile.whichSection (  ) == COIN_COLUMN_SECTION );
    assert ( gotNrow );
    rowType =
      ( COINMpsType * ) realloc ( rowType,
				 numberRows_ * sizeof ( COINMpsType ) );
    // put objective and other free rows at end
    rowName =
      ( char ** ) realloc ( rowName,
			    ( numberRows_ + 1 +

			      numberOtherFreeRows ) * sizeof ( char * ) );
    rowName[numberRows_] = strdup(objectiveName_);
    memcpy ( rowName + numberRows_ + 1, freeRowName,
	     numberOtherFreeRows * sizeof ( char * ) );
    // now we can get rid of this array
    free(freeRowName);

    startHash ( rowName, numberRows_ + 1 + numberOtherFreeRows , 0 );
    COINColumnIndex maxColumns = 1000 + numberRows_ / 5;
    CoinBigIndex maxElements = 5000 + numberRows_ / 2;
    COINMpsType *columnType = ( COINMpsType * )
      malloc ( maxColumns * sizeof ( COINMpsType ) );
    char **columnName = ( char ** ) malloc ( maxColumns * sizeof ( char * ) );

    objective_ = ( double * ) malloc ( maxColumns * sizeof ( double ) );
    start = ( CoinBigIndex * )
      malloc ( ( maxColumns + 1 ) * sizeof ( CoinBigIndex ) );
    row = ( COINRowIndex * )
      malloc ( maxElements * sizeof ( COINRowIndex ) );
    element =
      ( double * ) malloc ( maxElements * sizeof ( double ) );
    // for duplicates
    CoinBigIndex *rowUsed = new CoinBigIndex[numberRows_];

    for (i=0;i<numberRows_;i++) {
      rowUsed[i]=-1;
    }
    bool objUsed = false;

    numberElements_ = 0;
    char lastColumn[200];

    memset ( lastColumn, '\0', 200 );
    COINColumnIndex column = -1;
    bool inIntegerSet = false;
    COINColumnIndex numberIntegers = 0;
    const double tinyElement = 1.0e-14;

    while ( mpsfile.nextField (  ) == COIN_COLUMN_SECTION ) {
      switch ( mpsfile.mpsType (  ) ) {
      case COIN_BLANK_COLUMN:
	if ( strcmp ( lastColumn, mpsfile.columnName (  ) ) ) {
	  // new column

	  // reset old column and take out tiny
	  if ( numberColumns_ ) {
	    objUsed = false;
	    CoinBigIndex i;
	    CoinBigIndex k = start[column];

	    for ( i = k; i < numberElements_; i++ ) {
	      COINRowIndex irow = row[i];
#if 0
	      if ( fabs ( element[i] ) > tinyElement ) {
		element[k++] = element[i];
	      }
#endif
	      rowUsed[irow] = -1;
	    }
	    //numberElements_ = k;
	  }
	  column = numberColumns_;
	  if ( numberColumns_ == maxColumns ) {
	    maxColumns = ( 3 * maxColumns ) / 2 + 1000;
	    columnType = ( COINMpsType * )
	      realloc ( columnType, maxColumns * sizeof ( COINMpsType ) );
	    columnName = ( char ** )
	      realloc ( columnName, maxColumns * sizeof ( char * ) );

	    objective_ = ( double * )
	      realloc ( objective_, maxColumns * sizeof ( double ) );
	    start = ( CoinBigIndex * )
	      realloc ( start,
			( maxColumns + 1 ) * sizeof ( CoinBigIndex ) );
	  }
	  if ( !inIntegerSet ) {
	    columnType[column] = COIN_UNSET_BOUND;
	  } else {
	    columnType[column] = COIN_INTORG;
	    numberIntegers++;
	  }
	  columnName[column] = strdup ( mpsfile.columnName (  ) );
	  strcpy ( lastColumn, mpsfile.columnName (  ) );
	  objective_[column] = 0.0;
	  start[column] = numberElements_;
	  numberColumns_++;
	}
	if ( fabs ( mpsfile.value (  ) ) > tinyElement ) {
	  if ( numberElements_ == maxElements ) {
	    maxElements = ( 3 * maxElements ) / 2 + 1000;
	    row = ( COINRowIndex * )
	      realloc ( row, maxElements * sizeof ( COINRowIndex ) );
	    element = ( double * )
	      realloc ( element, maxElements * sizeof ( double ) );
	  }
	  // get row number
	  COINRowIndex irow = findHash ( mpsfile.rowName (  ) , 0 );

	  if ( irow >= 0 ) {
	    double value = mpsfile.value (  );

	    // check for duplicates
	    if ( irow == numberRows_ ) {
	      // objective
	      if ( objUsed ) {
		numberErrors++;
		if ( numberErrors < 100 ) {
		  handler_->message(COIN_MPS_DUPOBJ,messages_)
		    <<mpsfile.cardNumber()<<mpsfile.card()
		    <<CoinMessageEol;
		} else if (numberErrors > 100000) {
		  handler_->message(COIN_MPS_RETURNING,messages_)
		    <<CoinMessageEol;
		  return numberErrors;
		}
	      } else {
		objUsed = true;
	      }
	      value += objective_[column];
	      if ( fabs ( value ) <= tinyElement )
		value = 0.0;
	      objective_[column] = value;
	    } else if ( irow < numberRows_ ) {
	      // other free rows will just be discarded so won't get here
	      if ( rowUsed[irow] >= 0 ) {
		element[rowUsed[irow]] += value;
		numberErrors++;
		if ( numberErrors < 100 ) {
		  handler_->message(COIN_MPS_DUPROW,messages_)
		    <<mpsfile.rowName()<<mpsfile.cardNumber()
		    <<mpsfile.card()
		    <<CoinMessageEol;
		} else if (numberErrors > 100000) {
		  handler_->message(COIN_MPS_RETURNING,messages_)
		    <<CoinMessageEol;
		  return numberErrors;
		}
	      } else {
		row[numberElements_] = irow;
		element[numberElements_] = value;
		rowUsed[irow] = numberElements_;
		numberElements_++;
	      }
	    }
	  } else {
	    numberErrors++;
	    if ( numberErrors < 100 ) {
		  handler_->message(COIN_MPS_NOMATCHROW,messages_)
		    <<mpsfile.rowName()<<mpsfile.cardNumber()<<mpsfile.card()
		    <<CoinMessageEol;
	    } else if (numberErrors > 100000) {
	      handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	      return numberErrors;
	    }
	  }
	}
	break;
      case COIN_INTORG:
	inIntegerSet = true;
	break;
      case COIN_INTEND:
	inIntegerSet = false;
	break;
      case COIN_S1_COLUMN:
      case COIN_S2_COLUMN:
      case COIN_S3_COLUMN:
      case COIN_SOSEND:
	std::cout << "** code sos etc later" << std::endl;
	abort (  );
	break;
      default:
	numberErrors++;
	if ( numberErrors < 100 ) {
	  handler_->message(COIN_MPS_BADIMAGE,messages_)<<mpsfile.cardNumber()
						       <<mpsfile.card()
						       <<CoinMessageEol;
	} else if (numberErrors > 100000) {
	  handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	  return numberErrors;
	}
      }
    }
    start[numberColumns_] = numberElements_;
    delete[]rowUsed;
    assert ( mpsfile.whichSection (  ) == COIN_RHS_SECTION );
    columnType =
      ( COINMpsType * ) realloc ( columnType,
				 numberColumns_ * sizeof ( COINMpsType ) );
    columnName =

      ( char ** ) realloc ( columnName, numberColumns_ * sizeof ( char * ) );
    objective_ = ( double * )
      realloc ( objective_, numberColumns_ * sizeof ( double ) );
    start = ( CoinBigIndex * )
      realloc ( start, ( numberColumns_ + 1 ) * sizeof ( CoinBigIndex ) );
    row = ( COINRowIndex * )
      realloc ( row, numberElements_ * sizeof ( COINRowIndex ) );
    element = ( double * )
      realloc ( element, numberElements_ * sizeof ( double ) );

    rowlower_ = ( double * ) malloc ( numberRows_ * sizeof ( double ) );
    rowupper_ = ( double * ) malloc ( numberRows_ * sizeof ( double ) );
    for (i=0;i<numberRows_;i++) {
      rowlower_[i]=-infinity_;
      rowupper_[i]=infinity_;
    }
    objUsed = false;
    memset ( lastColumn, '\0', 200 );
    bool gotRhs = false;

    // need coding for blank rhs
    while ( mpsfile.nextField (  ) == COIN_RHS_SECTION ) {
      COINRowIndex irow;

      switch ( mpsfile.mpsType (  ) ) {
      case COIN_BLANK_COLUMN:
	if ( strcmp ( lastColumn, mpsfile.columnName (  ) ) ) {

	  // skip rest if got a rhs
	  if ( gotRhs ) {
	    while ( mpsfile.nextField (  ) == COIN_RHS_SECTION ) {
	    }
	    break;
	  } else {
	    gotRhs = true;
	    strcpy ( lastColumn, mpsfile.columnName (  ) );
	    // save name of section
	    free(rhsName_);
	    rhsName_=strdup(mpsfile.columnName());
	  }
	}
	// get row number
	irow = findHash ( mpsfile.rowName (  ) , 0 );
	if ( irow >= 0 ) {
	  double value = mpsfile.value (  );

	  // check for duplicates
	  if ( irow == numberRows_ ) {
	    // objective
	    if ( objUsed ) {
	      numberErrors++;
	      if ( numberErrors < 100 ) {
		  handler_->message(COIN_MPS_DUPOBJ,messages_)
		    <<mpsfile.cardNumber()<<mpsfile.card()
		    <<CoinMessageEol;
	      } else if (numberErrors > 100000) {
		handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
		return numberErrors;
	      }
	    } else {
	      objUsed = true;
	    }
	    objectiveOffset_ += value;
	  } else if ( irow < numberRows_ ) {
	    if ( rowlower_[irow] != -infinity_ ) {
	      numberErrors++;
	      if ( numberErrors < 100 ) {
		handler_->message(COIN_MPS_DUPROW,messages_)
		  <<mpsfile.rowName()<<mpsfile.cardNumber()<<mpsfile.card()
		  <<CoinMessageEol;
	      } else if (numberErrors > 100000) {
		handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
		return numberErrors;
	      }
	    } else {
	      rowlower_[irow] = value;
	    }
	  }
	} else {
	  numberErrors++;
	  if ( numberErrors < 100 ) {
	    handler_->message(COIN_MPS_NOMATCHROW,messages_)
	      <<mpsfile.rowName()<<mpsfile.cardNumber()<<mpsfile.card()
	      <<CoinMessageEol;
	  } else if (numberErrors > 100000) {
	    handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	    return numberErrors;
	  }
	}
	break;
      default:
	numberErrors++;
	if ( numberErrors < 100 ) {
	  handler_->message(COIN_MPS_BADIMAGE,messages_)<<mpsfile.cardNumber()
						       <<mpsfile.card()
						       <<CoinMessageEol;
	} else if (numberErrors > 100000) {
	  handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	  return numberErrors;
	}
      }
    }
    if ( mpsfile.whichSection (  ) == COIN_RANGES_SECTION ) {
      memset ( lastColumn, '\0', 200 );
      bool gotRange = false;
      COINRowIndex irow;

      // need coding for blank range
      while ( mpsfile.nextField (  ) == COIN_RANGES_SECTION ) {
	switch ( mpsfile.mpsType (  ) ) {
	case COIN_BLANK_COLUMN:
	  if ( strcmp ( lastColumn, mpsfile.columnName (  ) ) ) {

	    // skip rest if got a range
	    if ( gotRange ) {
	      while ( mpsfile.nextField (  ) == COIN_RANGES_SECTION ) {
	      }
	      break;
	    } else {
	      gotRange = true;
	      strcpy ( lastColumn, mpsfile.columnName (  ) );
	      // save name of section
	      free(rangeName_);
	      rangeName_=strdup(mpsfile.columnName());
	    }
	  }
	  // get row number
	  irow = findHash ( mpsfile.rowName (  ) , 0 );
	  if ( irow >= 0 ) {
	    double value = mpsfile.value (  );

	    // check for duplicates
	    if ( irow == numberRows_ ) {
	      // objective
	      numberErrors++;
	      if ( numberErrors < 100 ) {
		  handler_->message(COIN_MPS_DUPOBJ,messages_)
		    <<mpsfile.cardNumber()<<mpsfile.card()
		    <<CoinMessageEol;
	      } else if (numberErrors > 100000) {
		handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
		return numberErrors;
	      }
	    } else {
	      if ( rowupper_[irow] != infinity_ ) {
		numberErrors++;
		if ( numberErrors < 100 ) {
		  handler_->message(COIN_MPS_DUPROW,messages_)
		    <<mpsfile.rowName()<<mpsfile.cardNumber()<<mpsfile.card()
		    <<CoinMessageEol;
		} else if (numberErrors > 100000) {
		  handler_->message(COIN_MPS_RETURNING,messages_)
		    <<CoinMessageEol;
		  return numberErrors;
		}
	      } else {
		rowupper_[irow] = value;
	      }
	    }
	  } else {
	    numberErrors++;
	    if ( numberErrors < 100 ) {
	      handler_->message(COIN_MPS_NOMATCHROW,messages_)
		<<mpsfile.rowName()<<mpsfile.cardNumber()<<mpsfile.card()
		<<CoinMessageEol;
	    } else if (numberErrors > 100000) {
	      handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	      return numberErrors;
	    }
	  }
	  break;
	default:
	  numberErrors++;
	  if ( numberErrors < 100 ) {
	  handler_->message(COIN_MPS_BADIMAGE,messages_)<<mpsfile.cardNumber()
						       <<mpsfile.card()
						       <<CoinMessageEol;
	  } else if (numberErrors > 100000) {
	    handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	    return numberErrors;
	  }
	}
      }
    }
    stopHash ( 0 );
    // massage ranges
    {
      COINRowIndex irow;

      for ( irow = 0; irow < numberRows_; irow++ ) {
	double lo = rowlower_[irow];
	double up = rowupper_[irow];
	double up2 = rowupper_[irow];	//range

	switch ( rowType[irow] ) {
	case COIN_E_ROW:
	  if ( lo == -infinity_ )
	    lo = 0.0;
	  if ( up == infinity_ ) {
	    up = lo;
	  } else if ( up > 0.0 ) {
	    up += lo;
	  } else {
	    up = lo;
	    lo += up2;
	  }
	  break;
	case COIN_L_ROW:
	  if ( lo == -infinity_ ) {
	    up = 0.0;
	  } else {
	    up = lo;
	    lo = -infinity_;
	  }
	  if ( up2 != infinity_ ) {
	    lo = up - fabs ( up2 );
	  }
	  break;
	case COIN_G_ROW:
	  if ( lo == -infinity_ ) {
	    lo = 0.0;
	    up = infinity_;
	  } else {
	    up = infinity_;
	  }
	  if ( up2 != infinity_ ) {
	    up = lo + fabs ( up2 );
	  }
	  break;
	default:
	  abort();
	}
	rowlower_[irow] = lo;
	rowupper_[irow] = up;
      }
    }
    free ( rowType );
    // default bounds
    collower_ = ( double * ) malloc ( numberColumns_ * sizeof ( double ) );
    colupper_ = ( double * ) malloc ( numberColumns_ * sizeof ( double ) );
    for (i=0;i<numberColumns_;i++) {
      collower_[i]=0.0;
      colupper_[i]=infinity_;
    }
    // set up integer region just in case
    integerType_ = (char *) malloc (numberColumns_*sizeof(char));

    for ( column = 0; column < numberColumns_; column++ ) {
      if ( columnType[column] == COIN_INTORG ) {
	columnType[column] = COIN_UNSET_BOUND;
	integerType_[column] = 1;
      } else {
	integerType_[column] = 0;
      }
    }
    // start hash even if no bound section - to make sure names survive
    startHash ( columnName, numberColumns_ , 1 );
    if ( mpsfile.whichSection (  ) == COIN_BOUNDS_SECTION ) {
      memset ( lastColumn, '\0', 200 );
      bool gotBound = false;

      while ( mpsfile.nextField (  ) == COIN_BOUNDS_SECTION ) {
	if ( strcmp ( lastColumn, mpsfile.columnName (  ) ) ) {

	  // skip rest if got a bound
	  if ( gotBound ) {
	    while ( mpsfile.nextField (  ) == COIN_BOUNDS_SECTION ) {
	    }
	    break;
	  } else {
	    gotBound = true;;
	    strcpy ( lastColumn, mpsfile.columnName (  ) );
	    // save name of section
	    free(boundName_);
	    boundName_=strdup(mpsfile.columnName());
	  }
	}
	// get column number
	COINColumnIndex icolumn = findHash ( mpsfile.rowName (  ) , 1 );

	if ( icolumn >= 0 ) {
	  double value = mpsfile.value (  );
	  bool ifError = false;

	  switch ( mpsfile.mpsType (  ) ) {
	  case COIN_UP_BOUND:
	    if ( value == -1.0e100 )
	      ifError = true;
	    if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	      if ( value < 0.0 ) {
		collower_[icolumn] = -infinity_;
	      }
	    } else if ( columnType[icolumn] == COIN_LO_BOUND ) {
	      if ( value < collower_[icolumn] ) {
		ifError = true;
	      } else if ( value < collower_[icolumn] + tinyElement ) {
		value = collower_[icolumn];
	      }
	    } else if ( columnType[icolumn] == COIN_MI_BOUND ) {
	    } else {
	      ifError = true;
	    }
	    colupper_[icolumn] = value;
	    columnType[icolumn] = COIN_UP_BOUND;
	    break;
	  case COIN_LO_BOUND:
	    if ( value == -1.0e100 )
	      ifError = true;
	    if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	    } else if ( columnType[icolumn] == COIN_UP_BOUND ||
			columnType[icolumn] == COIN_UI_BOUND ) {
	      if ( value > colupper_[icolumn] ) {
		ifError = true;
	      } else if ( value > colupper_[icolumn] - tinyElement ) {
		value = colupper_[icolumn];
	      }
	    } else {
	      ifError = true;
	    }
	    collower_[icolumn] = value;
	    columnType[icolumn] = COIN_LO_BOUND;
	    break;
	  case COIN_FX_BOUND:
	    if ( value == -1.0e100 )
	      ifError = true;
	    if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	    } else if ( columnType[icolumn] == COIN_UI_BOUND ||
			columnType[icolumn] == COIN_BV_BOUND) {
	      // Allow so people can easily put FX's at end
	      double value2 = floor(value);
	      if (fabs(value2-value)>1.0e-12||
		  value2<collower_[icolumn]||
		  value2>colupper_[icolumn]) {
		ifError=true;
	      } else {
		// take off integer list
		assert(integerType_[icolumn] );
		numberIntegers--;
		integerType_[icolumn] = 0;
	      }
	    } else {
	      ifError = true;
	    }
	    collower_[icolumn] = value;
	    colupper_[icolumn] = value;
	    columnType[icolumn] = COIN_FX_BOUND;
	    break;
	  case COIN_FR_BOUND:
	    if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	    } else {
	      ifError = true;
	    }
	    collower_[icolumn] = -infinity_;
	    colupper_[icolumn] = infinity_;
	    columnType[icolumn] = COIN_FR_BOUND;
	    break;
	  case COIN_MI_BOUND:
	    if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	      colupper_[icolumn] = 0.0;
	    } else if ( columnType[icolumn] == COIN_UP_BOUND ) {
	    } else {
	      ifError = true;
	    }
	    collower_[icolumn] = -infinity_;
	    columnType[icolumn] = COIN_MI_BOUND;
	    break;
	  case COIN_PL_BOUND:
	    if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	    } else {
	      ifError = true;
	    }
	    columnType[icolumn] = COIN_PL_BOUND;
	    break;
	  case COIN_UI_BOUND:
#if 0
	    if ( value == -1.0e100 ) 
	      ifError = true;
	    if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	    } else if ( columnType[icolumn] == COIN_LO_BOUND ) {
	      if ( value < collower_[icolumn] ) {
		ifError = true;
	      } else if ( value < collower_[icolumn] + tinyElement ) {
		value = collower_[icolumn];
	      }
	    } else {
	      ifError = true;
	    }
#else
	    if ( value == -1.0e100 ) {
	       value = infinity_;
	       if (columnType[icolumn] != COIN_UNSET_BOUND &&
		   columnType[icolumn] != COIN_LO_BOUND) {
		  ifError = true;
	       }
	    } else {
	       if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	       } else if ( columnType[icolumn] == COIN_LO_BOUND ) {
		  if ( value < collower_[icolumn] ) {
		     ifError = true;
		  } else if ( value < collower_[icolumn] + tinyElement ) {
		     value = collower_[icolumn];
		  }
	       } else {
		  ifError = true;
	       }
	    }
#endif
	    colupper_[icolumn] = value;
	    columnType[icolumn] = COIN_UI_BOUND;
	    if ( !integerType_[icolumn] ) {
	      numberIntegers++;
	      integerType_[icolumn] = 1;
	    }
	    break;
	  case COIN_BV_BOUND:
	    if ( columnType[icolumn] == COIN_UNSET_BOUND ) {
	    } else {
	      ifError = true;
	    }
	    collower_[icolumn] = 0.0;
	    colupper_[icolumn] = 1.0;
	    columnType[icolumn] = COIN_BV_BOUND;
	    if ( !integerType_[icolumn] ) {
	      numberIntegers++;
	      integerType_[icolumn] = 1;
	    }
	    break;
	  default:
	    ifError = true;
	    break;
	  }
	  if ( ifError ) {
	    numberErrors++;
	    if ( numberErrors < 100 ) {
	      handler_->message(COIN_MPS_BADIMAGE,messages_)
		<<mpsfile.cardNumber()
		<<mpsfile.card()
		<<CoinMessageEol;
	    } else if (numberErrors > 100000) {
	      handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	      return numberErrors;
	    }
	  }
	} else {
	  numberErrors++;
	  if ( numberErrors < 100 ) {
	    handler_->message(COIN_MPS_NOMATCHCOL,messages_)
	      <<mpsfile.rowName()<<mpsfile.cardNumber()<<mpsfile.card()
	      <<CoinMessageEol;
	  } else if (numberErrors > 100000) {
	    handler_->message(COIN_MPS_RETURNING,messages_)<<CoinMessageEol;
	    return numberErrors;
	  }
	}
      }
    }
    stopHash ( 1 );
    // clean up integers
    if ( !numberIntegers ) {
      free(integerType_);
      integerType_ = NULL;
    } else {
      COINColumnIndex icolumn;

      for ( icolumn = 0; icolumn < numberColumns_; icolumn++ ) {
	if ( integerType_[icolumn] ) {
	  assert ( collower_[icolumn] >= -MAX_INTEGER );
	  // if 0 infinity make 0-1 ???
	  if ( columnType[icolumn] == COIN_UNSET_BOUND ) 
	    colupper_[icolumn] = defaultBound_;
	  if ( colupper_[icolumn] > MAX_INTEGER ) 
	    colupper_[icolumn] = MAX_INTEGER;
	}
      }
    }
    free ( columnType );
    assert ( mpsfile.whichSection (  ) == COIN_ENDATA_SECTION );
  } else {
    // This is very simple format - what should we use?
    COINColumnIndex i;
    fscanf ( fp, "%d %d %d\n", &numberRows_, &numberColumns_, &i);
    numberElements_  = i; // done this way in case numberElements_ long

    rowlower_ = ( double * ) malloc ( numberRows_ * sizeof ( double ) );
    rowupper_ = ( double * ) malloc ( numberRows_ * sizeof ( double ) );
    for ( i = 0; i < numberRows_; i++ ) {
      int j;

      fscanf ( fp, "%d %lg %lg\n", &j, &rowlower_[i], &rowupper_[i] );
      assert ( i == j );
    }
    collower_ = ( double * ) malloc ( numberColumns_ * sizeof ( double ) );
    colupper_ = ( double * ) malloc ( numberColumns_ * sizeof ( double ) );
    objective_= ( double * ) malloc ( numberColumns_ * sizeof ( double ) );
    start = ( CoinBigIndex *) malloc ((numberColumns_ + 1) *
					sizeof (CoinBigIndex) );
    row = ( COINRowIndex * ) malloc (numberElements_ * sizeof (COINRowIndex));
    element = ( double * ) malloc (numberElements_ * sizeof (double) );

    start[0] = 0;
    numberElements_ = 0;
    for ( i = 0; i < numberColumns_; i++ ) {
      int j;
      int n;

      fscanf ( fp, "%d %d %lg %lg %lg\n", &j, &n, &collower_[i], &colupper_[i],
	       &objective_[i] );
      assert ( i == j );
      for ( j = 0; j < n; j++ ) {
	fscanf ( fp, "       %d %lg\n", &row[numberElements_],
		 &element[numberElements_] );
	numberElements_++;
      }
      start[i + 1] = numberElements_;
    }
  }
  // construct packed matrix
  matrixByColumn_ = 
    new CoinPackedMatrix(true,
			numberRows_,numberColumns_,numberElements_,
			element,row,start,NULL);
  free ( row );
  free ( start );
  free ( element );

  handler_->message(COIN_MPS_STATS,messages_)<<problemName_
					    <<numberRows_
					    <<numberColumns_
					    <<numberElements_
					    <<CoinMessageEol;
  return numberErrors;
}

//------------------------------------------------------------------

// Function to return number in most efficient way
// Also creates row name field
/* formatType is
   0 - normal and 8 character names
   1 - extra accuracy
   2 - IEEE hex
   4 - normal but free format
*/
static void
convertDouble(int formatType, double value, char outputValue[20],
	      const char * name, char outputRow[100])
{
  assert (formatType!=2);
  if ((formatType&3)==0) {
    if (value<1.0e20) {
      int power10, decimal;
      if (value>=0.0) {
	power10 =(int) log10(value);
	if (power10<9&&power10>-4) {
	  decimal = min(10,10-power10);
	  char format[7];
	  sprintf(format,"%%12.%df",decimal);
	  sprintf(outputValue,format,value);
	} else {
	  sprintf(outputValue,"%12.7g",value);
	}
      } else {
	power10 =(int) log10(-value)+1;
	if (power10<8&&power10>-3) {
	  decimal = min(9,9-power10);
	  char format[7];
	  sprintf(format,"%%12.%df",decimal);
	  sprintf(outputValue,format,value);
	} else {
	  sprintf(outputValue,"%12.6g",value);
	}
      }
      // take off trailing 0
      int j;
      for (j=11;j>=0;j--) {
	if (outputValue[j]=='0')
	  outputValue[j]=' ';
	else
	  break;
      }
    } else {
      outputValue[0]= '\0'; // needs no value
    }
  } else {
    if (value<1.0e20) {
      sprintf(outputValue,"%19g",value);
      // take out blanks
      int i=0;
      int j;
      for (j=0;j<19;j++) {
	if (outputValue[j]!=' ')
	  outputValue[i++]=outputValue[j];
      }
      outputValue[i]='\0';
    } else {
      outputValue[0]= '\0'; // needs no value
    }
  }
  strcpy(outputRow,name);
  if (!formatType) {
    int i;
    // pad out to 12 and 8
    for (i=0;i<12;i++) {
      if (outputValue[i]=='\0')
	break;
    }
    for (;i<12;i++) 
      outputValue[i]=' ';
    outputValue[12]='\0';
    for (i=0;i<8;i++) {
      if (outputRow[i]=='\0')
	break;
    }
    for (;i<8;i++) 
      outputRow[i]=' ';
    outputRow[8]='\0';
  }
}

static void
writeString(FILE* fp, gzFile gzfp, const char* str)
{
   if (fp) {
      fprintf(fp, "%s", str);
   }
#ifdef COIN_USE_ZLIB
   if (gzfp) {
      gzprintf(gzfp, "%s", str);
   }
#endif
}

// Put out card image
static void outputCard(int formatType,int numberFields,
		       FILE *fp, gzFile gzfp,
		       std::string head, const char * name,
		       const char outputValue[2][20],
		       const char outputRow[2][100])
{
   // fprintf(fp,"%s",head.c_str());
   std::string line = head;
   int i;
   if (!formatType) {
      char outputColumn[9];
      strcpy(outputColumn,name);
      for (i=0;i<8;i++) {
	 if (outputColumn[i]=='\0')
	    break;
      }
      for (;i<8;i++) 
	 outputColumn[i]=' ';
      outputColumn[8]='\0';
      // fprintf(fp,"%s  ",outputColumn);
      line += outputColumn;
      line += "  ";
      for (i=0;i<numberFields;i++) {
	 // fprintf(fp,"%s  %s",outputRow[i],outputValue[i]);
	 line += outputRow[i];
	 line += "  ";
	 line += outputValue[i];
	 if (i<numberFields-1) {
	    // fprintf(fp,"   ");
	    line += "  ";
	 }
      }
   } else {
      // fprintf(fp,"%s",name);
      line += name;
      for (i=0;i<numberFields;i++) {
	 // fprintf(fp," %s %s",outputRow[i],outputValue[i]);
	 line += " ";
	 line += outputRow[i];
	 line += " ";
	 line += outputValue[i];
      }
   }
   
   // fprintf(fp,"\n");
   line += "\n";
   writeString(fp, gzfp, line.c_str());
}

int
CoinMpsIO::writeMps(const char *filename, int compression,
		   int formatType, int numberAcross) const
{
   std::string line = filename;
   FILE * fp = NULL;
   gzFile gzfp = NULL;
   switch (compression) {
   case 1:
#ifdef COIN_USE_ZLIB
      {
	 if (strcmp(line.c_str() +(line.size()-3), ".gz") != 0) {
	    line += ".gz";
	 }
	 gzfp = gzopen(line.c_str(), "wb");
	 if (gzfp) {
	    break;
	 }
      }
#endif
      fp = fopen(filename,"w");
      if (!fp)
	 return -1;
      break;

   case 2: /* bzip2: to be implemented */
   case 0:
      fp = fopen(filename,"w");
      if (!fp)
	 return -1;
      break;
   }

   const char * const * const rowNames = names_[0];
   const char * const * const columnNames = names_[1];
   int i;
   unsigned int length = 8;
   bool freeFormat = (formatType!=0);
   for (i = 0 ; i < numberRows_; ++i) {
      if (strlen(rowNames[i]) > length) {
	 length = strlen(rowNames[i]);
	 break;
      }
   }
   if (length <= 8) {
      for (i = 0 ; i < numberColumns_; ++i) {
	 if (strlen(columnNames[i]) > length) {
	    length = strlen(columnNames[i]);
	    break;
	 }
      }
   }
   if (length > 8 && !freeFormat) {
      freeFormat = true;
      formatType = 4;
   }
   
   // NAME card

   line = "NAME          ";
   if (strcmp(problemName_,"")==0) {
      line.append("BLANK   ");
   } else {
      if (strlen(problemName_) >= 8) {
	 line.append(problemName_, 8);
      } else {
	 line.append(problemName_);
	 line.append(8-strlen(problemName_), ' ');
      }
   }
   if (freeFormat)
      line.append("  FREE");
   // finish off name and do ROWS card and objective 
   line.append("\nROWS\n N  OBJROW\n");
   writeString(fp, gzfp, line.c_str());

   // Rows section
   // Sense array
   const char * sense = getRowSense();
  
   for (i=0;i<numberRows_;i++) {
      line = " ";
      if (sense[i]!='R') {
	 line.append(1,sense[i]);
      } else {
	 line.append("L");
      }
      line.append("  ");
      line.append(rowNames[i]);
      line.append("\n");
      writeString(fp, gzfp, line.c_str());
   }

   // COLUMNS card
   writeString(fp, gzfp, "COLUMNS\n");

   bool ifBounds=false;
   double largeValue = infinity_;

   const double * columnLower = getColLower();
   const double * columnUpper = getColUpper();
   const double * objective = getObjCoefficients();
   const CoinPackedMatrix * matrix = getMatrixByCol();
   const double * elements = matrix->getElements();
   const int * rows = matrix->getIndices();
   const CoinBigIndex * starts = matrix->getVectorStarts();
   const int * lengths = matrix->getVectorLengths();

   char outputValue[2][20];
   char outputRow[2][100];

   // Through columns (only put out if elements or objective value)
   for (i=0;i<numberColumns_;i++) {
      if (objective[i]||lengths[i]) {
	 // see if bound will be needed
	 if (columnLower[i]||columnUpper[i]<largeValue)
	    ifBounds=true;
	 int numberFields=0;
	 if (objective[i]) {
	    convertDouble(formatType,objective[i],outputValue[0],
			  "OBJROW",outputRow[0]);
	    numberFields=1;
	 }
	 if (numberFields==numberAcross) {
	    // put out card
	    outputCard(formatType, numberFields,
		       fp, gzfp, "    ",
		       columnNames[i],
		       outputValue,
		       outputRow);
	    numberFields=0;
	 }
	 int j;
	 for (j=0;j<lengths[i];j++) {
	    convertDouble(formatType,elements[starts[i]+j],
			  outputValue[numberFields],
			  rowNames[rows[starts[i]+j]],
			  outputRow[numberFields]);
	    numberFields++;
	    if (numberFields==numberAcross) {
	       // put out card
	       outputCard(formatType, numberFields,
			  fp, gzfp, "    ",
			  columnNames[i],
			  outputValue,
			  outputRow);
	       numberFields=0;
	    }
	 }
	 if (numberFields) {
	    // put out card
	    outputCard(formatType, numberFields,
		       fp, gzfp, "    ",
		       columnNames[i],
		       outputValue,
		       outputRow);
	 }
      }
   }

   bool ifRange=false;
   // RHS
   writeString(fp, gzfp, "RHS\n");

   const double * rowLower = getRowLower();
   const double * rowUpper = getRowUpper();

   int numberFields = 0;
   for (i=0;i<numberRows_;i++) {
      double value;
      switch (sense[i]) {
      case 'E':
	 value=rowLower[i];
	 break;
      case 'R':
	 value=rowUpper[i];
	 ifRange=true;
	 break;
      case 'L':
	 value=rowUpper[i];
	 break;
      case 'G':
	 value=rowLower[i];
	 break;
      default:
	 value=0.0;
	 break;
      }
      if (value != 0.0) {
	 convertDouble(formatType,value,
		       outputValue[numberFields],
		       rowNames[i],
		       outputRow[numberFields]);
	 numberFields++;
	 if (numberFields==numberAcross) {
	    // put out card
	    outputCard(formatType, numberFields,
		       fp, gzfp, "    ",
		       "RHS",
		       outputValue,
		       outputRow);
	    numberFields=0;
	 }
      }
   }
   if (numberFields) {
      // put out card
      outputCard(formatType, numberFields,
		 fp, gzfp, "    ",
		 "RHS",
		 outputValue,
		 outputRow);
   }

   if (ifRange) {
      // RANGES
      writeString(fp, gzfp, "RANGES\n");

      numberFields = 0;
      for (i=0;i<numberRows_;i++) {
	 if (sense[i]=='R') {
	    double value =rowUpper[i]-rowLower[i];
	    convertDouble(formatType,value,
			  outputValue[numberFields],
			  rowNames[i],
			  outputRow[numberFields]);
	    numberFields++;
	    if (numberFields==numberAcross) {
	       // put out card
	       outputCard(formatType, numberFields,
			  fp, gzfp, "    ",
			  "RANGE",
			  outputValue,
			  outputRow);
	       numberFields=0;
	    }
	 }
      }
      if (numberFields) {
	 // put out card
	 outputCard(formatType, numberFields,
		    fp, gzfp, "    ",
		    "RANGE",
		    outputValue,
		    outputRow);
      }
   }

   if (ifBounds) {
      // BOUNDS
      writeString(fp, gzfp, "BOUNDS\n");

      for (i=0;i<numberColumns_;i++) {
	 if (objective[i]||lengths[i]) {
	    // see if bound will be needed
	    if (columnLower[i]||columnUpper[i]<largeValue) {
	       int numberFields=1;
	       std::string header[2];
	       double value[2];
	       if (columnLower[i]<=-largeValue) {
		  // FR or MI
		  if (columnUpper[i]>=largeValue) {
		     header[0]=" FR ";
		     value[0] = largeValue;
		  } else {
		     header[0]=" MI ";
		     value[0] = largeValue;
		     header[0]=" UP ";
		     value[0] = columnUpper[i];
		     numberFields=2;
		  }
	       } else if (fabs(columnUpper[i]-columnLower[i])<1.0e-8) {
		  header[0]=" FX ";
		  value[0] = columnLower[i];
	       } else {
		  // do LO if needed
		  if (columnLower[i]) {
		     // LO
		     header[0]=" LO ";
		     value[0] = columnLower[i];
		     if (isInteger(i)) {
			// Integer variable so UI
			header[1]=" UI ";
			value[1] = columnUpper[i];
			numberFields=2;
		     } else if (columnUpper[i]<largeValue) {
			// UP
			header[1]=" UP ";
			value[1] = columnUpper[i];
			numberFields=2;
		     }
		  } else {
		     if (isInteger(i)) {
			// Integer variable so BV or UI
			if (fabs(columnUpper[i]-1.0)<1.0e-8) {
			   // BV
			   header[0]=" BV ";
			   value[0] = largeValue;
			} else {
			   // UI
			   header[0]=" UI ";
			   value[0] = columnUpper[i];
			}
		     } else {
			// UP
			header[0]=" UP ";
			value[0] = columnUpper[i];
		     }
		  }
	       }
	       // put out fields
	       int j;
	       for (j=0;j<numberFields;j++) {
		  convertDouble(formatType,value[j],
				outputValue[0],
				columnNames[i],
				outputRow[0]);
		  // put out card
		  outputCard(formatType, 1,
			     fp, gzfp, header[j],
			     "BOUND",
			     outputValue,
			     outputRow);
	       }
	    }
	 }
      }
   }

   // and finish

   writeString(fp, gzfp, "ENDATA\n");

   if (fp) {
      fclose(fp);
   }
#ifdef COIN_USE_ZLIB
   if (gzfp) {
      gzclose(gzfp);
   }
#endif

   return 0;
}
   
//------------------------------------------------------------------
// Problem name
const char * CoinMpsIO::getProblemName() const
{
  return problemName_;
}
// Objective name
const char * CoinMpsIO::getObjectiveName() const
{
  return objectiveName_;
}
// Rhs name
const char * CoinMpsIO::getRhsName() const
{
  return rhsName_;
}
// Range name
const char * CoinMpsIO::getRangeName() const
{
  return rangeName_;
}
// Bound name
const char * CoinMpsIO::getBoundName() const
{
  return boundName_;
}

//------------------------------------------------------------------
// Get number of rows, columns and elements
//------------------------------------------------------------------
int CoinMpsIO::getNumCols() const
{
  return numberColumns_;
}
int CoinMpsIO::getNumRows() const
{
  return numberRows_;
}
int CoinMpsIO::getNumElements() const
{
  return numberElements_;
}

//------------------------------------------------------------------
// Get pointer to column lower and upper bounds.
//------------------------------------------------------------------  
const double * CoinMpsIO::getColLower() const
{
  return collower_;
}
const double * CoinMpsIO::getColUpper() const
{
  return colupper_;
}

//------------------------------------------------------------------
// Get pointer to row lower and upper bounds.
//------------------------------------------------------------------  
const double * CoinMpsIO::getRowLower() const
{
  return rowlower_;
}
const double * CoinMpsIO::getRowUpper() const
{
  return rowupper_;
}
 
/** A quick inlined function to convert from lb/ub style constraint
    definition to sense/rhs/range style */
inline void
CoinMpsIO::convertBoundToSense(const double lower, const double upper,
					char& sense, double& right,
					double& range) const
{
  range = 0.0;
  if (lower > -infinity_) {
    if (upper < infinity_) {
      right = upper;
      if (upper==lower) {
        sense = 'E';
      } else {
        sense = 'R';
        range = upper - lower;
      }
    } else {
      sense = 'G';
      right = lower;
    }
  } else {
    if (upper < infinity_) {
      sense = 'L';
      right = upper;
    } else {
      sense = 'N';
      right = 0.0;
    }
  }
}

//-----------------------------------------------------------------------------
/** A quick inlined function to convert from sense/rhs/range stryle constraint
    definition to lb/ub style */
inline void
CoinMpsIO::convertSenseToBound(const char sense, const double right,
					const double range,
					double& lower, double& upper) const
{
  switch (sense) {
  case 'E':
    lower = upper = right;
    break;
  case 'L':
    lower = -infinity_;
    upper = right;
    break;
  case 'G':
    lower = right;
    upper = infinity_;
    break;
  case 'R':
    lower = right - range;
    upper = right;
    break;
  case 'N':
    lower = -infinity_;
    upper = infinity_;
    break;
  }
}
//------------------------------------------------------------------
// Get sense of row constraints.
//------------------------------------------------------------------ 
const char * CoinMpsIO::getRowSense() const
{
  if ( rowsense_==NULL ) {

    int nr=numberRows_;
    rowsense_ = (char *) malloc(nr*sizeof(char));


    double dum1,dum2;
    int i;
    for ( i=0; i<nr; i++ ) {
      convertBoundToSense(rowlower_[i],rowupper_[i],rowsense_[i],dum1,dum2);
    }
  }
  return rowsense_;
}

//------------------------------------------------------------------
// Get the rhs of rows.
//------------------------------------------------------------------ 
const double * CoinMpsIO::getRightHandSide() const
{
  if ( rhs_==NULL ) {

    int nr=numberRows_;
    rhs_ = (double *) malloc(nr*sizeof(double));


    char dum1;
    double dum2;
    int i;
    for ( i=0; i<nr; i++ ) {
      convertBoundToSense(rowlower_[i],rowupper_[i],dum1,rhs_[i],dum2);
    }
  }
  return rhs_;
}

//------------------------------------------------------------------
// Get the range of rows.
// Length of returned vector is getNumRows();
//------------------------------------------------------------------ 
const double * CoinMpsIO::getRowRange() const
{
  if ( rowrange_==NULL ) {

    int nr=numberRows_;
    rowrange_ = (double *) malloc(nr*sizeof(double));
    std::fill(rowrange_,rowrange_+nr,0.0);

    char dum1;
    double dum2;
    int i;
    for ( i=0; i<nr; i++ ) {
      convertBoundToSense(rowlower_[i],rowupper_[i],dum1,dum2,rowrange_[i]);
    }
  }
  return rowrange_;
}

const double * CoinMpsIO::getObjCoefficients() const
{
  return objective_;
}
 
//------------------------------------------------------------------
// Create a row copy of the matrix ...
//------------------------------------------------------------------
const CoinPackedMatrix * CoinMpsIO::getMatrixByRow() const
{
  if ( matrixByRow_ == NULL && matrixByColumn_) {
    matrixByRow_ = new CoinPackedMatrix(*matrixByColumn_);
    matrixByRow_->reverseOrdering();
  }
  return matrixByRow_;
}

//------------------------------------------------------------------
// Create a column copy of the matrix ...
//------------------------------------------------------------------
const CoinPackedMatrix * CoinMpsIO::getMatrixByCol() const
{
  return matrixByColumn_;
}

//------------------------------------------------------------------
// Save the data ...
//------------------------------------------------------------------
void
CoinMpsIO::setMpsDataWithoutRowAndColNames(
                                  const CoinPackedMatrix& m, const double infinity,
                                  const double* collb, const double* colub,
                                  const double* obj, const char* integrality,
                                  const double* rowlb, const double* rowub)
{
  freeAll();
  if (m.isColOrdered()) {
    matrixByColumn_ = new CoinPackedMatrix(m);
  } else {
    matrixByColumn_ = new CoinPackedMatrix;
    matrixByColumn_->reverseOrderedCopyOf(m);
  }
  numberColumns_ = matrixByColumn_->getNumCols();
  numberRows_ = matrixByColumn_->getNumRows();
  numberElements_ = matrixByColumn_->getNumElements();
  defaultBound_ = 1;
  infinity_ = infinity;
  objectiveOffset_ = 0;
  
  rowlower_ = (double *) malloc (numberRows_ * sizeof(double));
  rowupper_ = (double *) malloc (numberRows_ * sizeof(double));
  collower_ = (double *) malloc (numberColumns_ * sizeof(double));
  colupper_ = (double *) malloc (numberColumns_ * sizeof(double));
  objective_ = (double *) malloc (numberColumns_ * sizeof(double));
  std::copy(rowlb, rowlb + numberRows_, rowlower_);
  std::copy(rowub, rowub + numberRows_, rowupper_);
  std::copy(collb, collb + numberColumns_, collower_);
  std::copy(colub, colub + numberColumns_, colupper_);
  std::copy(obj, obj + numberColumns_, objective_);
  if (integrality) {
    integerType_ = (char *) malloc (numberColumns_ * sizeof(char));
    std::copy(integrality, integrality + numberColumns_, integerType_);
  } else {
    integerType_ = 0;
  }
    
  problemName_ = strdup("");
  objectiveName_ = strdup("");
  rhsName_ = strdup("");
  rangeName_ = strdup("");
  boundName_ = strdup("");
}


void
CoinMpsIO::setMpsDataColAndRowNames(
		      char const * const * const colnames,
		      char const * const * const rownames)
{  
   // If long names free format
   names_[0] = (char **) malloc(numberRows_ * sizeof(char *));
   names_[1] = (char **) malloc (numberColumns_ * sizeof(char *));
   char** rowNames_ = names_[0];
   char** colNames_ = names_[1];
   int i;
   if (rownames) {
     for (i = 0 ; i < numberRows_; ++i) {
       rowNames_[i] = strdup(rownames[i]);
     }
   } else {
     for (i = 0; i < numberRows_; ++i) {
       rowNames_[i] = (char *) malloc (9 * sizeof(char));
       sprintf(rowNames_[i],"R%7.7d",i);
     }
   }
   if (colnames) {
     for (i = 0 ; i < numberColumns_; ++i) {
       colNames_[i] = strdup(colnames[i]);
     }
   } else {
     for (i = 0; i < numberColumns_; ++i) {
       colNames_[i] = (char *) malloc (9 * sizeof(char));
       sprintf(colNames_[i],"C%7.7d",i);
     }
   }
}

void
CoinMpsIO::setMpsDataColAndRowNames(
		      const std::vector<std::string> & colnames,
		      const std::vector<std::string> & rownames)
{  
   // If long names free format
   names_[0] = (char **) malloc(numberRows_ * sizeof(char *));
   names_[1] = (char **) malloc (numberColumns_ * sizeof(char *));
   char** rowNames_ = names_[0];
   char** colNames_ = names_[1];
   int i;
   if (rownames.size()!=0) {
     for (i = 0 ; i < numberRows_; ++i) {
       rowNames_[i] = strdup(rownames[i].c_str());
     }
   } else {
     for (i = 0; i < numberRows_; ++i) {
       rowNames_[i] = (char *) malloc (9 * sizeof(char));
       sprintf(rowNames_[i],"R%7.7d",i);
     }
   }
   if (colnames.size()!=0) {
     for (i = 0 ; i < numberColumns_; ++i) {
       colNames_[i] = strdup(colnames[i].c_str());
     }
   } else {
     for (i = 0; i < numberColumns_; ++i) {
       colNames_[i] = (char *) malloc (9 * sizeof(char));
       sprintf(colNames_[i],"C%7.7d",i);
     }
   }
}

void
CoinMpsIO::setMpsData(const CoinPackedMatrix& m, const double infinity,
                      const double* collb, const double* colub,
                      const double* obj, const char* integrality,
                      const double* rowlb, const double* rowub,
                      char const * const * const colnames,
                      char const * const * const rownames)
{
  setMpsDataWithoutRowAndColNames(m,infinity,collb,colub,obj,integrality,rowlb,rowub);
  setMpsDataColAndRowNames(colnames,rownames);
}

void
CoinMpsIO::setMpsData(const CoinPackedMatrix& m, const double infinity,
                      const double* collb, const double* colub,
                      const double* obj, const char* integrality,
                      const double* rowlb, const double* rowub,
                      const std::vector<std::string> & colnames,
                      const std::vector<std::string> & rownames)
{
  setMpsDataWithoutRowAndColNames(m,infinity,collb,colub,obj,integrality,rowlb,rowub);
  setMpsDataColAndRowNames(colnames,rownames);
}

void
CoinMpsIO::setMpsData(const CoinPackedMatrix& m, const double infinity,
		      const double* collb, const double* colub,
		      const double* obj, const char* integrality,
		      const char* rowsen, const double* rowrhs,
		      const double* rowrng,
		      char const * const * const colnames,
		      char const * const * const rownames)
{
   const int numrows = matrixByColumn_->getNumRows();

   double * rlb = numrows ? new double[numrows] : 0;
   double * rub = numrows ? new double[numrows] : 0;

   for (int i = 0; i < numrows; ++i) {
      convertSenseToBound(rowsen[i], rowrhs[i], rowrng[i], rlb[i], rub[i]);
   }
   setMpsData(m, infinity, collb, colub, obj, integrality, rlb, rub,
	      colnames, rownames);
}

void
CoinMpsIO::setMpsData(const CoinPackedMatrix& m, const double infinity,
		      const double* collb, const double* colub,
		      const double* obj, const char* integrality,
		      const char* rowsen, const double* rowrhs,
		      const double* rowrng,
		      const std::vector<std::string> & colnames,
		      const std::vector<std::string> & rownames)
{
   const int numrows = matrixByColumn_->getNumRows();

   double * rlb = numrows ? new double[numrows] : 0;
   double * rub = numrows ? new double[numrows] : 0;

   for (int i = 0; i < numrows; ++i) {
      convertSenseToBound(rowsen[i], rowrhs[i], rowrng[i], rlb[i], rub[i]);
   }
   setMpsData(m, infinity, collb, colub, obj, integrality, rlb, rub,
	      colnames, rownames);
}


//------------------------------------------------------------------
// Return true if column is a continuous, binary, ...
//------------------------------------------------------------------
bool CoinMpsIO::isContinuous(int columnNumber) const
{
  const char * intType = integerType_;
  if ( intType==NULL ) return true;
  assert (columnNumber>=0 && columnNumber < numberColumns_);
  if ( intType[columnNumber]==0 ) return true;
  return false;
}

/* Return true if column is integer.
   Note: This function returns true if the the column
   is binary or a general integer.
*/
bool CoinMpsIO::isInteger(int columnNumber) const
{
  const char * intType = integerType_;
  if ( intType==NULL ) return false;
  assert (columnNumber>=0 && columnNumber < numberColumns_);
  if ( intType[columnNumber]!=0 ) return true;
  return false;
}
// if integer
const char * CoinMpsIO::integerColumns() const
{
  return integerType_;
}
// names - returns NULL if out of range
const char * CoinMpsIO::rowName(int index) const
{
  if (index>=0&&index<numberRows_) {
    return names_[0][index];
  } else {
    return NULL;
  }
}
const char * CoinMpsIO::columnName(int index) const
{
  if (index>=0&&index<numberColumns_) {
    return names_[1][index];
  } else {
    return NULL;
  }
}
// names - returns -1 if name not found
int CoinMpsIO::rowIndex(const char * name) const
{
  if (!hash_[0]) {
    if (numberRows_) {
      startHash(0);
    } else {
      return -1;
    }
  }
  return findHash ( name , 0 );
}
    int CoinMpsIO::columnIndex(const char * name) const
{
  if (!hash_[1]) {
    if (numberColumns_) {
      startHash(1);
    } else {
      return -1;
    }
  }
  return findHash ( name , 1 );
}

// Release all row information (lower, upper)
void CoinMpsIO::releaseRowInformation()
{
  free(rowlower_);
  free(rowupper_);
  rowlower_=NULL;
  rowupper_=NULL;
}
// Release all column information (lower, upper, objective)
void CoinMpsIO::releaseColumnInformation()
{
  free(collower_);
  free(colupper_);
  free(objective_);
  collower_=NULL;
  colupper_=NULL;
  objective_=NULL;
}
// Release integer information
void CoinMpsIO::releaseIntegerInformation()
{
  free(integerType_);
  integerType_=NULL;
}
// Release row names
void CoinMpsIO::releaseRowNames()
{
  releaseRedundantInformation();
  int i;
  for (i=0;i<numberHash_[0];i++) {
    free(names_[0][i]);
  }
  free(names_[0]);
  names_[0]=NULL;
  numberHash_[0]=0;
}
// Release column names
void CoinMpsIO::releaseColumnNames()
{
  releaseRedundantInformation();
  int i;
  for (i=0;i<numberHash_[1];i++) {
    free(names_[1][i]);
  }
  free(names_[1]);
  names_[1]=NULL;
  numberHash_[1]=0;
}
// Release matrix information
void CoinMpsIO::releaseMatrixInformation()
{
  releaseRedundantInformation();
  delete matrixByColumn_;
  matrixByColumn_=NULL;
}
  


//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinMpsIO::CoinMpsIO ()
:
rowsense_(NULL),
rhs_(NULL),
rowrange_(NULL),
matrixByRow_(NULL),
matrixByColumn_(NULL),
rowlower_(NULL),
rowupper_(NULL),
collower_(NULL),
colupper_(NULL),
objective_(NULL),
integerType_(NULL),
fileName_(strdup("stdin")),
numberRows_(0),
numberColumns_(0),
numberElements_(0),
defaultBound_(1),
infinity_(COIN_DBL_MAX),
objectiveOffset_(0.0),
problemName_(strdup("")),
objectiveName_(strdup("")),
rhsName_(strdup("")),
rangeName_(strdup("")),
boundName_(strdup("")),
defaultHandler_(true)
{
  numberHash_[0]=0;
  hash_[0]=NULL;
  names_[0]=NULL;
  numberHash_[1]=0;
  hash_[1]=NULL;
  names_[1]=NULL;
  handler_ = new CoinMessageHandler();
  messages_ = CoinMessage();
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinMpsIO::CoinMpsIO(const CoinMpsIO & rhs)
:
rowsense_(NULL),
rhs_(NULL),
rowrange_(NULL),
matrixByRow_(NULL),
matrixByColumn_(NULL),
rowlower_(NULL),
rowupper_(NULL),
collower_(NULL),
colupper_(NULL),
objective_(NULL),
integerType_(NULL),
fileName_(strdup("stdin")),
numberRows_(0),
numberColumns_(0),
numberElements_(0),
defaultBound_(1),
infinity_(COIN_DBL_MAX),
objectiveOffset_(0.0),
problemName_(strdup("")),
objectiveName_(strdup("")),
rhsName_(strdup("")),
rangeName_(strdup("")),
boundName_(strdup("")),
defaultHandler_(true)
{
  numberHash_[0]=0;
  hash_[0]=NULL;
  names_[0]=NULL;
  numberHash_[1]=0;
  hash_[1]=NULL;
  names_[1]=NULL;
  if ( rhs.rowlower_ !=NULL || rhs.collower_ != NULL) {
    gutsOfCopy(rhs);
    // OK and proper to leave rowsense_, rhs_, and
    // rowrange_ (also row copy and hash) to NULL.  They will be constructed
    // if they are required.
  }
  defaultHandler_ = rhs.defaultHandler_;
  if (defaultHandler_)
    handler_ = new CoinMessageHandler(*rhs.handler_);
  else
    handler_ = rhs.handler_;
  messages_ = CoinMessage();
}

void CoinMpsIO::gutsOfCopy(const CoinMpsIO & rhs)
{
  defaultHandler_ = rhs.defaultHandler_;
  if (rhs.matrixByColumn_)
    matrixByColumn_=new CoinPackedMatrix(*(rhs.matrixByColumn_));
  numberElements_=rhs.numberElements_;
  numberRows_=rhs.numberRows_;
  numberColumns_=rhs.numberColumns_;
  if (rhs.rowlower_) {
    rowlower_ = (double *) malloc(numberRows_*sizeof(double));
    rowupper_ = (double *) malloc(numberRows_*sizeof(double));
    memcpy(rowlower_,rhs.rowlower_,numberRows_*sizeof(double));
    memcpy(rowupper_,rhs.rowupper_,numberRows_*sizeof(double));
  }
  if (rhs.collower_) {
    collower_ = (double *) malloc(numberColumns_*sizeof(double));
    colupper_ = (double *) malloc(numberColumns_*sizeof(double));
    objective_ = (double *) malloc(numberColumns_*sizeof(double));
    memcpy(collower_,rhs.collower_,numberColumns_*sizeof(double));
    memcpy(colupper_,rhs.colupper_,numberColumns_*sizeof(double));
    memcpy(objective_,rhs.objective_,numberColumns_*sizeof(double));
  }
  if (rhs.integerType_) {
    integerType_ = (char *) malloc (numberColumns_*sizeof(char));
    memcpy(integerType_,rhs.integerType_,numberColumns_*sizeof(char));
  }
  free(fileName_);
  free(problemName_);
  free(objectiveName_);
  free(rhsName_);
  free(rangeName_);
  free(boundName_);
  fileName_ = strdup(rhs.fileName_);
  problemName_ = strdup(rhs.problemName_);
  objectiveName_ = strdup(rhs.objectiveName_);
  rhsName_ = strdup(rhs.rhsName_);
  rangeName_ = strdup(rhs.rangeName_);
  boundName_ = strdup(rhs.boundName_);
  numberHash_[0]=rhs.numberHash_[0];
  numberHash_[1]=rhs.numberHash_[1];
  defaultBound_=rhs.defaultBound_;
  infinity_=rhs.infinity_;
  objectiveOffset_=rhs.objectiveOffset_;
  int section;
  for (section=0;section<2;section++) {
    if (numberHash_[section]) {
      char ** names2 = rhs.names_[section];
      names_[section] = (char **) malloc(numberHash_[section]*
					 sizeof(char *));
      char ** names = names_[section];
      int i;
      for (i=0;i<numberHash_[section];i++) {
	names[i]=strdup(names2[i]);
      }
    }
  }
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinMpsIO::~CoinMpsIO ()
{
  gutsOfDestructor();
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinMpsIO &
CoinMpsIO::operator=(const CoinMpsIO& rhs)
{
  if (this != &rhs) {    
    gutsOfDestructor();
    if ( rhs.rowlower_ !=NULL || rhs.collower_ != NULL) {
      gutsOfCopy(rhs);
    }
    defaultHandler_ = rhs.defaultHandler_;
    if (defaultHandler_)
      handler_ = new CoinMessageHandler(*rhs.handler_);
    else
      handler_ = rhs.handler_;
    messages_ = CoinMessage();
  }
  return *this;
}

//-------------------------------------------------------------------
void CoinMpsIO::gutsOfDestructor()
{  
  freeAll();
  if (defaultHandler_) {
    delete handler_;
    handler_ = NULL;
  }
}


void CoinMpsIO::freeAll()
{  
  releaseRedundantInformation();
  releaseRowNames();
  releaseColumnNames();
  delete matrixByRow_;
  delete matrixByColumn_;
  matrixByRow_=NULL;
  matrixByColumn_=NULL;
  free(rowlower_);
  free(rowupper_);
  free(collower_);
  free(colupper_);
  free(objective_);
  free(integerType_);
  free(fileName_);
  rowlower_=NULL;
  rowupper_=NULL;
  collower_=NULL;
  colupper_=NULL;
  objective_=NULL;
  integerType_=NULL;
  fileName_=NULL;
  free(problemName_);
  free(objectiveName_);
  free(rhsName_);
  free(rangeName_);
  free(boundName_);
  problemName_=NULL;
  objectiveName_=NULL;
  rhsName_=NULL;
  rangeName_=NULL;
  boundName_=NULL;
}

/* Release all information which can be re-calculated e.g. rowsense
    also any row copies OR hash tables for names */
void CoinMpsIO::releaseRedundantInformation()
{  
  free( rowsense_);
  free( rhs_);
  free( rowrange_);
  rowsense_=NULL;
  rhs_=NULL;
  rowrange_=NULL;
  free (hash_[0]);
  free (hash_[1]);
  hash_[0]=0;
  hash_[1]=0;
  delete matrixByRow_;
  matrixByRow_=NULL;
}
// Pass in Message handler (not deleted at end)
void 
CoinMpsIO::passInMessageHandler(CoinMessageHandler * handler)
{
  defaultHandler_=false;
  handler_=handler;
}
// Set language
void 
CoinMpsIO::newLanguage(CoinMessages::Language language)
{
  messages_ = CoinMessage(language);
}

//#############################################################################
