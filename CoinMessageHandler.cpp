// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinMessageHandler.hpp"
#include <cassert>
#include <map>
using std::min;
using std::max;

/* Default constructor. */
CoinOneMessage::CoinOneMessage()
{
  externalNumber_=-1;
  message_=NULL;
  severity_='I';
  detail_=-1;
}
/* Destructor */
CoinOneMessage::~CoinOneMessage()
{
  free(message_);
}
/* The copy constructor */
CoinOneMessage::CoinOneMessage(const CoinOneMessage & rhs)
{
  externalNumber_=rhs.externalNumber_;
  if (rhs.message_)
    message_=strdup(rhs.message_);
  else
    message_=NULL;
  severity_=rhs.severity_;
  detail_=rhs.detail_;
}
/* assignment operator. */
CoinOneMessage& 
CoinOneMessage::operator=(const CoinOneMessage & rhs)
{
  if (this != &rhs) {
    externalNumber_=rhs.externalNumber_;
    free(message_);
    if (rhs.message_)
      message_=strdup(rhs.message_);
    else
      message_=NULL;
    severity_=rhs.severity_;
    detail_=rhs.detail_;
  }
  return *this;
}
/* Normal constructor */
CoinOneMessage::CoinOneMessage(int externalNumber, char detail,
		const char * message)
{
  externalNumber_=externalNumber;
  message_=strdup(message);
  if (externalNumber<3000)
    severity_='I';
  else if (externalNumber<6000)
    severity_='W';
  else if (externalNumber<9000)
    severity_='E';
  else
    severity_='S';
  detail_=detail;
}
// Replaces messages (i.e. a different language)
void 
CoinOneMessage::replaceMessage( const char * message)
{
  free(message_);
  message_=strdup(message);
}


/* Constructor with number of messages. */
CoinMessages::CoinMessages(int numberMessages)
{
  numberMessages_=numberMessages;
  language_=us_en;
  strcpy(source_,"Unk");
  if (numberMessages_) {
    message_ = new CoinOneMessage * [numberMessages_];
    int i;
    for (i=0;i<numberMessages_;i++) 
      message_[i]=NULL;
  } else {
    message_=NULL;
  }
}
/* Destructor */
CoinMessages::~CoinMessages()
{
  int i;
  for (i=0;i<numberMessages_;i++) 
    delete message_[i];
  delete [] message_;
}
/* The copy constructor */
CoinMessages::CoinMessages(const CoinMessages & rhs)
{
  numberMessages_=rhs.numberMessages_;
  language_=rhs.language_;
  strcpy(source_,rhs.source_);
  if (numberMessages_) {
    message_ = new CoinOneMessage * [numberMessages_];
    int i;
    for (i=0;i<numberMessages_;i++) 
      if (rhs.message_[i])
	message_[i]=new CoinOneMessage(*(rhs.message_[i]));
      else
	message_[i] = NULL;
  } else {
    message_=NULL;
  }
}
/* assignment operator. */
CoinMessages& 
CoinMessages::operator=(const CoinMessages & rhs)
{
  if (this != &rhs) {
    language_=rhs.language_;
    strcpy(source_,rhs.source_);
    int i;
    for (i=0;i<numberMessages_;i++)
	delete message_[i];
    delete [] message_;
    numberMessages_=rhs.numberMessages_;
    if (numberMessages_) {
      message_ = new CoinOneMessage * [numberMessages_];
      int i;
      for (i=0;i<numberMessages_;i++) 
	if (rhs.message_[i])
	  message_[i]=new CoinOneMessage(*(rhs.message_[i]));
	else
	  message_[i] = NULL;
    } else {
      message_=NULL;
    }
  }
  return *this;
}
// Puts message in correct place
void 
CoinMessages::addMessage(int messageNumber, const CoinOneMessage & message)
{
  if (messageNumber>=numberMessages_) {
    // should not happen but allow for it
    CoinOneMessage ** temp = new CoinOneMessage * [messageNumber+1];
    int i;
    for (i=0;i<numberMessages_;i++) 
      temp[i] = message_[i];
    for (;i<=messageNumber;i++) 
      temp[i] = NULL;
    delete [] message_;
    message_ = temp;
  }
  delete message_[messageNumber];
  message_[messageNumber]=new CoinOneMessage(message);
}
// Replaces messages (i.e. a different language)
void 
CoinMessages::replaceMessage(int messageNumber, 
			     const char * message)
{
  assert(messageNumber<numberMessages_);
  message_[messageNumber]->replaceMessage(message);
}
// Changes detail level for one message
void 
CoinMessages::setDetailMessage(int newLevel, int messageNumber)
{
  int i;
  for (i=0;i<numberMessages_;i++) {
    if (message_[i]->externalNumber()==messageNumber) {
      message_[i]->setDetail(newLevel);
      break;
    }
  }
}
// Changes detail level for several messages
void 
CoinMessages::setDetailMessages(int newLevel, int numberMessages,
			       int * messageNumbers)
{
  int i;
  if (numberMessages<3) {
    // do one by one
    int j;
    for (j=0;j<numberMessages;j++) {
      int messageNumber = messageNumbers[j];
      for (i=0;i<numberMessages_;i++) {
	if (message_[i]->externalNumber()==messageNumber) {
	  message_[i]->setDetail(newLevel);
	  break;
	}
      }
    }
  } else {
    // do backward mapping
    int backward[10000];
    for (i=0;i<10000;i++) 
      backward[i]=-1;
    for (i=0;i<numberMessages_;i++) 
      backward[message_[i]->externalNumber()]=i;
    for (i=0;i<numberMessages;i++) {
      int iback = backward[messageNumbers[i]];
      if (iback>=0)
	message_[iback]->setDetail(newLevel);
    }
  }
}

// Print message, return 0 normally
int 
CoinMessageHandler::print() 
{
  if (messageOut_>messageBuffer_) {
    *messageOut_=0;
    //take off trailing spaces and commas
    messageOut_--;
    while (messageOut_>=messageBuffer_) {
      if (*messageOut_==' '||*messageOut_==',') {
	*messageOut_=0;
	messageOut_--;
      } else {
	break;
      } 
    } 
    fprintf(fp_,"%s\n",messageBuffer_);
    if (currentMessage_.severity_=='S') {
      fprintf(fp_,"Stopping due to previous errors.\n");
      //Should do walkback
      abort();
    } 
  }
  return 0;
}
/* Amount of print out:
   0 - none
   1 - minimal
   2 - normal low
   3 - normal high
   4 - verbose
   above that 8,16,32 etc just for selective debug and are for
   printf messages in code
*/
void 
CoinMessageHandler::setLogLevel(int value)
{
  if (value>=0)
    logLevel_=value;
}
void 
CoinMessageHandler::setPrefix(bool value)
{
  if (value)
    prefix_ = 255;
  else
    prefix_ =0;
}
// Constructor
CoinMessageHandler::CoinMessageHandler() :
  logLevel_(1),
  prefix_(255),
  currentMessage_(),
  internalNumber_(0),
  format_(NULL),
  numberDoubleFields_(0),
  numberIntFields_(0),
  numberCharFields_(0),
  numberStringFields_(0),
  printStatus_(0),
  highestNumber_(-1),
  fp_(stdout)
{
  messageBuffer_[0]='\0';
  messageOut_ = messageBuffer_;
  source_="Unk";
}
// Constructor
CoinMessageHandler::CoinMessageHandler(FILE * fp) :
  logLevel_(1),
  prefix_(255),
  currentMessage_(),
  internalNumber_(0),
  format_(NULL),
  numberDoubleFields_(0),
  numberIntFields_(0),
  numberCharFields_(0),
  numberStringFields_(0),
  printStatus_(0),
  highestNumber_(-1),
  fp_(fp)
{
  messageBuffer_[0]='\0';
  messageOut_ = messageBuffer_;
  source_="Unk";
}
/* Destructor */
CoinMessageHandler::~CoinMessageHandler()
{
}
/* The copy constructor */
CoinMessageHandler::CoinMessageHandler(const CoinMessageHandler& rhs)
{
  logLevel_=rhs.logLevel_;
  prefix_ = rhs.prefix_;
  currentMessage_=rhs.currentMessage_;
  internalNumber_=rhs.internalNumber_;
  int i;
  numberDoubleFields_ = rhs.numberDoubleFields_;
  for (i=0;i<numberDoubleFields_;i++) 
    doubleValue_[i]=rhs.doubleValue_[i];
  numberIntFields_ = rhs.numberIntFields_;
  for (i=0;i<numberIntFields_;i++) 
    longValue_[i]=rhs.longValue_[i];
  numberCharFields_ = rhs.numberCharFields_;
  for (i=0;i<numberCharFields_;i++) 
    charValue_[i]=rhs.charValue_[i];
  numberStringFields_ = rhs.numberStringFields_;
  for (i=0;i<numberStringFields_;i++) 
    stringValue_[i]=rhs.stringValue_[i];
  int offset = rhs.format_ - rhs.currentMessage_.message();
  format_ = currentMessage_.message()+offset;
  strcpy(messageBuffer_,rhs.messageBuffer_);
  offset = rhs.messageOut_-rhs.messageBuffer_;
  messageOut_= messageBuffer_+offset;
  printStatus_= rhs.printStatus_;
  highestNumber_= rhs.highestNumber_;
  fp_ = rhs.fp_;
  source_ = rhs.source_;
}
/* assignment operator. */
CoinMessageHandler & 
CoinMessageHandler::operator=(const CoinMessageHandler& rhs)
{
  if (this != &rhs) {
    logLevel_=rhs.logLevel_;
    prefix_ = rhs.prefix_;
    currentMessage_=rhs.currentMessage_;
    internalNumber_=rhs.internalNumber_;
    int i;
    numberDoubleFields_ = rhs.numberDoubleFields_;
    for (i=0;i<numberDoubleFields_;i++) 
      doubleValue_[i]=rhs.doubleValue_[i];
    numberIntFields_ = rhs.numberIntFields_;
    for (i=0;i<numberIntFields_;i++) 
      longValue_[i]=rhs.longValue_[i];
    numberCharFields_ = rhs.numberCharFields_;
    for (i=0;i<numberCharFields_;i++) 
      charValue_[i]=rhs.charValue_[i];
    numberStringFields_ = rhs.numberStringFields_;
    for (i=0;i<numberStringFields_;i++) 
      stringValue_[i]=rhs.stringValue_[i];
    int offset = rhs.format_ - rhs.currentMessage_.message();
    format_ = currentMessage_.message()+offset;
    strcpy(messageBuffer_,rhs.messageBuffer_);
    offset = rhs.messageOut_-rhs.messageBuffer_;
    messageOut_= messageBuffer_+offset;
    printStatus_= rhs.printStatus_;
    highestNumber_= rhs.highestNumber_;
    fp_ = rhs.fp_;
    source_ = rhs.source_;
  }
  return *this;
}
// Clone
CoinMessageHandler * 
CoinMessageHandler::clone() const
{
  return new CoinMessageHandler(*this);
}
// Start a message
CoinMessageHandler & 
CoinMessageHandler::message(int messageNumber,
			   const CoinMessages &normalMessage)
{
  if (messageOut_!=messageBuffer_) {
    // put out last message
    print();
  }
  numberDoubleFields_=0;
  numberIntFields_=0;
  numberCharFields_=0;
  numberStringFields_=0;
  internalNumber_=messageNumber;
  currentMessage_= *(normalMessage.message_[messageNumber]);
  source_ = normalMessage.source_;
  format_ = currentMessage_.message_;
  messageBuffer_[0]='\0';
  messageOut_=messageBuffer_;
  highestNumber_ = max(highestNumber_,currentMessage_.externalNumber_);
  // do we print
  int detail = currentMessage_.detail_;
  printStatus_=0;
  if (detail>=8) {
    // bit setting - debug
    if ((detail&logLevel_)==0)
      printStatus_ = 3;
  } else if (logLevel_<detail) {
    printStatus_ = 3;
  }
  if (!printStatus_) {
    if (prefix_) {
      sprintf(messageOut_,"%s%4.4d%c ",source_.c_str(),
	      currentMessage_.externalNumber_,
	      currentMessage_.severity_);
      messageOut_ += strlen(messageOut_);
    }
    format_ = nextPerCent(format_,true);
  }
  return *this;
}
/* The following is to help existing codes interface
   Starts message giving number and complete text
*/
CoinMessageHandler & 
CoinMessageHandler::message(int externalNumber,const char * source,
			   const char * msg, char severity)
{
  if (messageOut_!=messageBuffer_) {
    // put out last message
    print();
  }
  numberDoubleFields_=0;
  numberIntFields_=0;
  numberCharFields_=0;
  numberStringFields_=0;
  internalNumber_=externalNumber;
  currentMessage_= CoinOneMessage();
  currentMessage_.setExternalNumber(externalNumber);
  source_ = source;
  // mark so will not update buffer
  printStatus_=2;
  highestNumber_ = max(highestNumber_,externalNumber);
  // If we get here we always print
  if (prefix_) {
    sprintf(messageOut_,"%s%4.4d%c ",source_.c_str(),
	    externalNumber,
	    severity);
  }
  strcat(messageBuffer_,msg);
  messageOut_=messageBuffer_+strlen(messageBuffer_);
  return *this;
}
  /* Allows for skipping printing of part of message,
      but putting in data */
  CoinMessageHandler & 
CoinMessageHandler::printing(bool onOff)
{
  // has no effect if skipping or whole message in
  if (printStatus_<2) {
    assert(format_[1]=='?');
    if (onOff)
      printStatus_=0;
    else
      printStatus_=1;
    format_ = nextPerCent(format_+2,true);
  }
  return *this;
}
/* Stop (and print) 
*/
int 
CoinMessageHandler::finish()
{
  if (messageOut_!=messageBuffer_) {
    // put out last message
    print();
  }
  numberDoubleFields_=0;
  numberIntFields_=0;
  numberCharFields_=0;
  numberStringFields_=0;
  internalNumber_=-1;
  format_ = NULL;
  messageBuffer_[0]='\0';
  messageOut_=messageBuffer_;
  printStatus_=true;
  return 0;
}
/* Gets position of next field in format
   if initial then copies until first % */
char * 
CoinMessageHandler::nextPerCent(char * start , const bool initial)
{
  if (start) {
    bool foundNext=false;
    while (!foundNext) {
      char * nextPerCent = strchr(start,'%');
      if (nextPerCent) {
	// %? is skipped over as it is just a separator
	if (nextPerCent[1]!='?') {
	  if (initial&&!printStatus_) {
	    int numberToCopy=nextPerCent-start;
	    strncpy(messageOut_,start,numberToCopy);
	    messageOut_+=numberToCopy;
	  } 
	  start=nextPerCent;
	  if (start[1]!='%') {
	    foundNext=true;
	    if (!initial) 
	      *start='\0'; //zap
	  } else {
	    start+=2;
	    if (initial) {
	      *messageOut_='%';
	      messageOut_++;
	    } 
	  }
	} else {
	  foundNext=true;
	  // skip to % and zap
	  start=nextPerCent;
	  *start='\0'; 
	}
      } else {
        if (initial&&!printStatus_) {
          strcpy(messageOut_,start);
          messageOut_+=strlen(messageOut_);
        } 
        start=0;
        foundNext=true;
      } 
    } 
  } 
  return start;
}
// Adds into message
CoinMessageHandler & 
CoinMessageHandler::operator<< (int intvalue)
{
  if (printStatus_==3)
    return *this; // not doing this message
  longValue_[numberIntFields_++] = intvalue;
  if (printStatus_<2) {
    if (format_) {
      //format is at % (but may be changed to null)
      *format_='%';
      char * next = nextPerCent(format_+1);
      // could check
      if (!printStatus_) {
	sprintf(messageOut_,format_,intvalue);
	messageOut_+=strlen(messageOut_);
      }
      format_=next;
    } else {
      sprintf(messageOut_," %d",intvalue);
      messageOut_+=strlen(messageOut_);
    } 
  }
  return *this;
}
CoinMessageHandler & 
CoinMessageHandler::operator<< (double doublevalue)
{
  if (printStatus_==3)
    return *this; // not doing this message
  doubleValue_[numberDoubleFields_++] = doublevalue;
  if (printStatus_<2) {
    if (format_) {
      //format is at % (but changed to 0)
      *format_='%';
      char * next = nextPerCent(format_+1);
      // could check
      if (!printStatus_) {
	sprintf(messageOut_,format_,doublevalue);
	messageOut_+=strlen(messageOut_);
      }
      format_=next;
    } else {
      sprintf(messageOut_," %g",doublevalue);
      messageOut_+=strlen(messageOut_);
    } 
  }
  return *this;
}
CoinMessageHandler & 
CoinMessageHandler::operator<< (long longvalue)
{
  if (printStatus_==3)
    return *this; // not doing this message
  longValue_[numberIntFields_++] = longvalue;
  if (printStatus_<2) {
    if (format_) {
      //format is at % (but may be changed to null)
      *format_='%';
      char * next = nextPerCent(format_+1);
      // could check
      if (!printStatus_) {
	sprintf(messageOut_,format_,longvalue);
	messageOut_+=strlen(messageOut_);
      }
      format_=next;
    } else {
      sprintf(messageOut_," %ld",longvalue);
      messageOut_+=strlen(messageOut_);
    } 
  }
  return *this;
}
CoinMessageHandler & 
CoinMessageHandler::operator<< (std::string stringvalue)
{
  if (printStatus_==3)
    return *this; // not doing this message
  stringValue_[numberStringFields_++] = stringvalue;
  if (printStatus_<2) {
    if (format_) {
      //format is at % (but changed to 0)
      *format_='%';
      char * next = nextPerCent(format_+1);
      // could check
      if (!printStatus_) {
	sprintf(messageOut_,format_,stringvalue.c_str());
	messageOut_+=strlen(messageOut_);
      }
      format_=next;
    } else {
      sprintf(messageOut_," %s",stringvalue.c_str());
      messageOut_+=strlen(messageOut_);
    } 
  }
  return *this;
}
CoinMessageHandler & 
CoinMessageHandler::operator<< (char charvalue)
{
  if (printStatus_==3)
    return *this; // not doing this message
  longValue_[numberCharFields_++] = charvalue;
  if (printStatus_<2) {
    if (format_) {
      //format is at % (but changed to 0)
      *format_='%';
      char * next = nextPerCent(format_+1);
      // could check
      if (!printStatus_) {
	sprintf(messageOut_,format_,charvalue);
	messageOut_+=strlen(messageOut_);
      }
      format_=next;
    } else {
      sprintf(messageOut_," %c",charvalue);
      messageOut_+=strlen(messageOut_);
    } 
  }
  return *this;
}
CoinMessageHandler & 
CoinMessageHandler::operator<< (const char *stringvalue)
{
  if (printStatus_==3)
    return *this; // not doing this message
  stringValue_[numberStringFields_++] = stringvalue;
  if (printStatus_<2) {
    if (format_) {
      //format is at % (but changed to 0)
      *format_='%';
      char * next = nextPerCent(format_+1);
      // could check
      if (!printStatus_) {
	sprintf(messageOut_,format_,stringvalue);
	messageOut_+=strlen(messageOut_);
      }
      format_=next;
    } else {
      sprintf(messageOut_," %s",stringvalue);
      messageOut_+=strlen(messageOut_);
    } 
  }
  return *this;
}
CoinMessageHandler & 
CoinMessageHandler::operator<< (CoinMessageMarker marker)
{
  if (printStatus_!=3) {
    switch (marker) {
    case CoinMessageEol:
      finish();
      break;
    case CoinMessageNewline:
      strcat(messageOut_,"\n");
      messageOut_++;
      break;
    }
  } else {
    // skipping - tidy up
    format_ = NULL;
  }
  return *this;
}

// returns current 
CoinMessageHandler & 
CoinMessageHandler::message()
{
  return * this;
}
