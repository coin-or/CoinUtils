
/*
  A brief test of CoinMessageHandler. Tests that we can print basic messages,
  and checks that we can handle messages with parts. Does not attempt to test
  any of the various enbryonic features.
*/

#include "CoinPragma.hpp"
#include "CoinMessageHandler.hpp"

/*
  Define a set of test messages.
*/
enum COIN_TestMessage
{ COIN_TST_NOFIELDS,
  COIN_TST_INT,
  COIN_TST_DBL,
  COIN_TST_CHAR,
  COIN_TST_STRING,
  COIN_TST_MULTIPART,
  COIN_TST_NOCODES,
  COIN_TST_END
} ;

/*
  Convenient structure for doing static initialisation. Essentially, we want
  something that'll allow us to set up an array of structures with the
  information required by CoinOneMessage. Then we can easily walk the array,
  create a CoinOneMessage structure for each message, and add each message to
  a CoinMessages structure.
*/
typedef struct
{ COIN_TestMessage internalNumber;
  int externalNumber;
  char detail;
  const char * message;
} MsgDefn ;

static MsgDefn us_tstmsgs[] =
{ {COIN_TST_NOFIELDS,1,1,"This message has no parts and no fields."},
  {COIN_TST_INT,3,1,"This message has an integer field: (%d)"},
  {COIN_TST_DBL,4,1,"This message has a double field: (%g)"},
  {COIN_TST_CHAR,5,1,"This message has a char field: (%c)"},
  {COIN_TST_STRING,6,1,"This message has a string field: (%s)"},
  {COIN_TST_MULTIPART,7,1,
    "Prefix%? Part 1%? Part 2 with integer in hex %#.4x%? Part 3%? suffix"},
  {COIN_TST_NOCODES,8,1,""},
  {COIN_TST_END,7,1,"This is the dummy end marker."}
} ;


void CoinMessageHandlerUnitTest ()

{ CoinMessages::Language curLang = CoinMessages::us_en ;

/*
  Create a CoinMessages object to hold our messages. 
*/
  CoinMessages testMessages(sizeof(us_tstmsgs)/sizeof(MsgDefn)) ;
  strcpy(testMessages.source_,"Test") ;
/*
  Load with messages. This involves creating successive messages
  (CoinOneMessage) and loading them into the array. This is the usual copy
  operation; client is responsible for disposing of the original message
  (accomplished here by keeping the CoinOneMessage internal to the loop
  body).
*/
  MsgDefn *msgDef = us_tstmsgs ;
  while (msgDef->internalNumber != COIN_TST_END)
  { CoinOneMessage msg(msgDef->externalNumber,msgDef->detail,msgDef->message) ;
    testMessages.addMessage(msgDef->internalNumber,msg) ;
    msgDef++ ; }
/*
  Ok, we're loaded. Create a message handler and set the log level so we'll
  print.
*/
  CoinMessageHandler hdl ;
  hdl.setLogLevel(1) ;
/*
  Simple tests of one piece messages.
*/
  hdl.message(COIN_TST_NOFIELDS,testMessages) << CoinMessageEol ;
  hdl.message(COIN_TST_INT,testMessages) << 42 << CoinMessageEol ;
  hdl.message(COIN_TST_DBL,testMessages) << 42.42 << CoinMessageEol ;
  hdl.message(COIN_TST_CHAR,testMessages) << 'w' << CoinMessageEol ;
  hdl.message(COIN_TST_STRING,testMessages) << "forty-two" << CoinMessageEol ;
/*
  A multipart message, consisting of prefix, three optional parts, and a
  suffix. Note that we need four calls to printing() in order to process the
  four `%?' codes.
*/
  hdl.message(COIN_TST_MULTIPART,testMessages) ;
  hdl.printing(true) ;
  hdl.printing(true) << 42 ;
  hdl.printing(true) ;
  hdl.printing(true) << CoinMessageEol ;
  hdl.message(COIN_TST_MULTIPART,testMessages) ;
  hdl.printing(false) ;
  hdl.printing(false) << 42 ;
  hdl.printing(false) ;
  hdl.printing(true) << CoinMessageEol ;
  hdl.message(COIN_TST_MULTIPART,testMessages) ;
  hdl.printing(true) ;
  hdl.printing(false) << 42 ;
  hdl.printing(false) ;
  hdl.printing(true) << CoinMessageEol ;
  hdl.message(COIN_TST_MULTIPART,testMessages) ;
  hdl.printing(false) ;
  hdl.printing(true) << 42 ;
  hdl.printing(false) ;
  hdl.printing(true) << CoinMessageEol ;
  hdl.message(COIN_TST_MULTIPART,testMessages) ;
  hdl.printing(false) ;
  hdl.printing(false) << 42 ;
  hdl.printing(true) ;
  hdl.printing(true) << CoinMessageEol ;
/*
  Construct a message from scratch given an empty message. Parameters are
  printed with default format codes.
*/
  hdl.message(COIN_TST_NOCODES,testMessages) ;
  hdl.message() << "Standardised prefix, free form remainder:" ;
  hdl.message() << "An int" << 42
   << "A double" << 4.2
   << "a new line" << CoinMessageNewline
   << "and done." << CoinMessageEol ;
/*
  Construct a message from scratch given nothing at all.
*/
  hdl.message() << "No standardised prefix, free form reminder: integer ("
    << 42 << ")." ;
  hdl.finish() ;
/*
  And the transition mechanism, where we just dump the string we're given.
  It's not possible to augment this message, as printStatus_ is set to 2,
  which prevents the various operator<< methods from contributing to the
  output buffer, with the exception of operator<<(CoinMessageMarker).
*/
  hdl.message(27,"Tran","Transition message.",'I') << CoinMessageEol ;

  return ; }
