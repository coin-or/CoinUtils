// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <iostream>
#include <cassert>

#include "CoinRead.hpp"

#ifdef COIN_HAS_CBC
// from CoinSolve
static char coin_prompt[]="Coin:";
#else
static char coin_prompt[]="Clp:";
#endif
static char line[1000];
static char * where=NULL;
std::string afterEquals="";

// Simple read stuff
std::string
CoinReadNextField(FILE *ReadCommand, int ReadMode)
{
  std::string field;
  if (!where) {
    // need new line
#ifdef COIN_HAS_READLINE     
    if (ReadCommand==stdin) {
      // Get a line from the user. 
      where = readline (coin_prompt);
      
      // If the line has any text in it, save it on the history.
      if (where) {
	if ( *where)
	  add_history (where);
	strcpy(line,where);
	free(where);
      }
    } else {
      where = fgets(line,1000,ReadCommand);
    }
#else
    if (ReadCommand==stdin) {
      fprintf(stdout,coin_prompt);
      fflush(stdout);
    }
    where = fgets(line,1000,ReadCommand);
#endif
    if (!where)
      return field; // EOF
    where = line;
    // clean image
    char * lastNonBlank = line-1;
    while ( *where != '\0' ) {
      if ( *where != '\t' && *where < ' ' ) {
	break;
      } else if ( *where != '\t' && *where != ' ') {
	lastNonBlank = where;
      }
      where++;
    }
    where=line;
    *(lastNonBlank+1)='\0';
  }
  // munch white space
  while(*where==' '||*where=='\t')
    where++;
  char * saveWhere = where;
  while (*where!=' '&&*where!='\t'&&*where!='\0')
    where++;
  if (where!=saveWhere) {
    char save = *where;
    *where='\0';
    //convert to string
    field=saveWhere;
    *where=save;
  } else {
    where=NULL;
    field="EOL";
  }
  return field;
}

std::string
CoinReadGetCommand(int argc, const char *argv[], FILE *ReadCommand, 
		   int ReadMode)
{
  std::string field="EOL";
  // say no =
  afterEquals="";
  while (field=="EOL") {
    if (ReadMode>0) {
      if (ReadMode<argc) {
	field = argv[ReadMode++];
	if (field=="-") {
	  std::cout<<"Switching to line mode"<<std::endl;
	  ReadMode=-1;
	  field=CoinReadNextField(ReadCommand, ReadMode);
	} else if (field[0]!='-') {
	  if (ReadMode!=2) {
	    // now allow std::cout<<"skipping non-command "<<field<<std::endl;
	    // field="EOL"; // skip
	  } else {
	    // special dispensation - taken as -import name
	    ReadMode--;
	    field="import";
	  }
	} else {
	  if (field!="--") {
	    // take off -
	    field = field.substr(1);
	  } else {
	    // special dispensation - taken as -import --
	    ReadMode--;
	    field="import";
	  }
	}
      } else {
	field="";
      }
    } else {
      field=CoinReadNextField(ReadCommand, ReadMode);
    }
  }
  // if = then modify and save
  std::string::size_type found = field.find('=');
  if (found!=std::string::npos) {
    afterEquals = field.substr(found+1);
    field = field.substr(0,found);
  }
  //std::cout<<field<<std::endl;
  return field;
}
std::string
CoinReadGetString(int argc, const char *argv[], FILE *ReadCommand, 
		  int ReadMode)
{
  std::string field="EOL";
  if (afterEquals=="") {
    if (ReadMode>0) {
      if (ReadMode<argc) {
        if (argv[ReadMode][0]!='-') { 
          field = argv[ReadMode++];
        } else if (!strcmp(argv[ReadMode],"--")) {
          field = argv[ReadMode++];
          // -- means import from stdin
          field = "-";
        }
      }
    } else {
      field=CoinReadNextField(ReadCommand, ReadMode);
    }
  } else {
    field=afterEquals;
    afterEquals = "";
  }
  //std::cout<<field<<std::endl;
  return field;
}
// valid 0 - okay, 1 bad, 2 not there
int
CoinReadGetIntField(int argc, const char *argv[],int * valid, 
		    FILE *ReadCommand, int ReadMode)
{
  std::string field="EOL";
  if (afterEquals=="") {
    if (ReadMode>0) {
      if (ReadMode<argc) {
        // may be negative value so do not check for -
        field = argv[ReadMode++];
      }
    } else {
      field=CoinReadNextField(ReadCommand, ReadMode);
    }
  } else {
    field=afterEquals;
    afterEquals = "";
  }
  int value=0;
  //std::cout<<field<<std::endl;
  if (field!="EOL") {
    // how do I check valid
    value =  atoi(field.c_str());
    *valid=0;
  } else {
    *valid=2;
  }
  return value;
}
double
CoinReadGetDoubleField(int argc, const char *argv[],int * valid, 
		       FILE *ReadCommand, int ReadMode)
{
  std::string field="EOL";
  if (afterEquals=="") {
    if (ReadMode>0) {
      if (ReadMode<argc) {
        // may be negative value so do not check for -
        field = argv[ReadMode++];
      }
    } else {
      field=CoinReadNextField(ReadCommand, ReadMode);
    }
  } else {
    field=afterEquals;
    afterEquals = "";
  }
  double value=0.0;
  //std::cout<<field<<std::endl;
  if (field!="EOL") {
    // how do I check valid
    value = atof(field.c_str());
    *valid=0;
  } else {
    *valid=2;
  }
  return value;
}
