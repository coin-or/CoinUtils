// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinMessage.hpp"

typedef struct {
  COIN_Message internalNumber;
  int externalNumber; // or continuation
  char detail;
  const char * message;
} Coin_message;
static Coin_message us_english[]=
{
  {COIN_MPS_LINE,1,1,"At line %d %s"},
  {COIN_MPS_STATS,2,1,"Problem %s has %d rows, %d columns and %d elements"},
  {COIN_MPS_ILLEGAL,3001,0,"Illegal value for %s of %g"},
  {COIN_MPS_BADIMAGE,3002,0,"Bad image at line %d < %s >"},
  {COIN_MPS_DUPOBJ,3003,0,"Duplicate objective at line %d < %s >"},
  {COIN_MPS_DUPROW,3004,0,"Duplicate row %s at line %d < %s >"},
  {COIN_MPS_NOMATCHROW,3005,0,"No match for row %s at line %d < %s >"},
  {COIN_MPS_NOMATCHCOL,3006,0,"No match for column %s at line %d < %s >"},
  {COIN_MPS_FILE,6001,0,"Unable to open mps input file %s"},
  {COIN_MPS_BADFILE1,6002,0,"Unknown image %s at line 1 of file %s"},
  {COIN_MPS_BADFILE2,6003,0,"Consider the possibility of a compressed file\
 which zlib is unable to read"},
  {COIN_MPS_EOF,6004,0,"EOF on file %s"},
  {COIN_MPS_RETURNING,6005,0,"Returning as too many errors"},
  {COIN_SOLVER_MPS,8,1,"%s read with %d errors"},
  {COIN_DUMMY_END,999999,0,""}
};
// **** aiutami!
static Coin_message italian[]=
{
  {COIN_MPS_LINE,1,1,"al numero %d %s"},
  {COIN_MPS_STATS,2,1,"matrice %s ha %d file, %d colonne and %d elementi (diverso da zero)"},
  {COIN_DUMMY_END,999999,0,""}
};
/* Constructor */
CoinMessage::CoinMessage(Language language) :
  CoinMessages(sizeof(us_english)/sizeof(Coin_message))
{
  language_=language;
  strcpy(source_,"Coin");
  Coin_message * message = us_english;

  while (message->internalNumber!=COIN_DUMMY_END) {
    CoinOneMessage oneMessage(message->externalNumber,message->detail,
		message->message);
    addMessage(message->internalNumber,oneMessage);
    message ++;
  }

  // now override any language ones

  switch (language) {
  case it:
    message = italian;
    break;

  default:
    message=NULL;
    break;
  }

  // replace if any found
  if (message) {
    while (message->internalNumber!=COIN_DUMMY_END) {
      replaceMessage(message->internalNumber,message->message);
      message ++;
    }
  }
}
