// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.
#ifndef CoinMessageHandler_H
#define CoinMessageHandler_H

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif


#include <iostream>
#include <stdio.h>
#include <string>

/** This is a first attempt at a message handler.

 I am strongly in favo(u)r of language support for an open
 source project, but I have tried to make it as lightweight as
 possible in the sense that only a subset of messages need to be
 defined - the rest default to US English.  There will be different
 sets of messages for each component - so at present there is a
 Clp component and an Coin component.

 The default handler at present just prints to stdout or to a FILE pointer

 Because messages are only used in a controlled environment and have no
 impact on code and are tested by other tests I have include tests such
 as language and derivation in other unit tests.

 I need help with iso character codes as I don't understand how
 it all works.

*/

/** Class for one massaged message.
    At this stage it is in the correct language.
    A message may have several parts of which all except the
    first may be omitted - see printing(bool) in CoinMessagehandler.
    Parts are delineated by %?
 */

class CoinOneMessage {

public:
  /**@name Constructors etc */
  //@{
  /** Default constructor. */
  CoinOneMessage();
  /** Normal constructor */
  CoinOneMessage(int externalNumber, char detail,
		const char * message);
  /** Destructor */
  ~CoinOneMessage();
  /** The copy constructor */
  CoinOneMessage(const CoinOneMessage&);
  /** assignment operator. */
  CoinOneMessage& operator=(const CoinOneMessage&);
  //@}

  /**@name Useful stuff */
  //@{
  /// Replaces messages (i.e. a different language)
  void replaceMessage(const char * message);
  /// Sets detail level
  inline void setDetail(int level)
  {detail_=level;};
  /// Gets detail level
  inline int detail() const
  {return detail_;};
  /// Returns message
   char * message() const
  {return message_;};
  //@}

   /**@name gets and sets methods */
   //@{
  /// number to print out (also determines severity)
    inline int externalNumber() const
  {return externalNumber_;};
    inline void setExternalNumber(int number) 
  {externalNumber_=number;};
  /// Severity
  inline char severity() const
  {return severity_;};
  //@}

  /**@name member data */
  //@{
  /// number to print out (also determines severity)
    int externalNumber_;
  /// Will only print if detail matches
    char detail_;
  /// Severity
    char severity_;
  /// Messages (in correct language)
  char * message_;
  //@}
};

/** Class for massaged messages.
    By now knows language and source
 */

class CoinMessages {

public:
  /** These are the languages that are supported.  At present only
      us_en is serious and the rest are for testing.  I can't
      really see uk_en being used seriously!
  */
  enum Language {
    us_en = 0,
    uk_en,
    it
  };
  /**@name Constructors etc */
  //@{
  /** Constructor with number of messages. */
  CoinMessages(int numberMessages=0);
  /** Destructor */
  ~CoinMessages();
  /** The copy constructor */
  CoinMessages(const CoinMessages&);
  /** assignment operator. */
  CoinMessages& operator=(const CoinMessages&);
  //@}

  /**@name Useful stuff */
  //@{
  /// Puts message in correct place
  void addMessage(int messageNumber, const CoinOneMessage & message);
  /// Replaces messages (i.e. a different language)
  void replaceMessage(int messageNumber, 
		const char * message);
  /** Language.  Need to think about iso codes */
  inline Language language() const
  {return language_;};
  void setLanguage(Language language)
  {language_ = language;};
  /// Changes detail level for one message
  void setDetailMessage(int newLevel, int messageNumber);
  /// Changes detail level for several messages
  void setDetailMessages(int newLevel, int numberMessages,
			 int * messageNumbers);
  //@}

  /**@name member data */
  //@{
  /// Number of messages
  int numberMessages_;
  /// Language 
  Language language_;
  /// Source e.g. Clp
  char source_[5];
  /// Messages
  CoinOneMessage ** message_;
  //@}
};
// for convenience eol
enum CoinMessageMarker {
  CoinMessageEol = 0,
  CoinMessageNewline = 1
};

/** Base class for message handling

    The default behavior is just to print. Inherit to change that
    This is a first attempt and it shows.
    messages <3000 are informational
             <6000 warnings
	     <9000 errors - but will try and carry on or return code
	     >=9000 stops
    Where there are derived classes I have started message numbers
    at 1001

    Messages can be printed with or without a prefix e.g. Clp0102I.
    A prefix makes the messages look less nimble but is very useful
    for "grep" etc.
*/

class CoinMessageHandler  {
  
public:
   /**@name Virtual methods that the derived classes may provide */
   //@{
  /** Print message, return 0 normally.
      At present this is the only virtual method
  */
   virtual int print() ;
   //@}

  /**@name Constructors etc */
  //@{
  /// Constructor
  CoinMessageHandler();
  /// Constructor to put to file pointer (won't be closed)
  CoinMessageHandler(FILE *fp);
  /** Destructor */
  virtual ~CoinMessageHandler();
  /** The copy constructor */
  CoinMessageHandler(const CoinMessageHandler&);
  /** assignment operator. */
  CoinMessageHandler& operator=(const CoinMessageHandler&);
  /// Clone
  virtual CoinMessageHandler * clone() const;
  //@}
   /**@name gets and sets methods */
   //@{
  /** Amount of print out:
      0 - none
      1 - minimal
      2 - normal low
      3 - normal high
      4 - verbose
      above that 8,16,32 etc just for selective debug and are for
      printf messages in code
  */
  inline int logLevel() const
          { return logLevel_;};
  void setLogLevel(int value);
  /// Switch on or off prefix
  void setPrefix(bool yesNo);
  /// values in message
  inline double doubleValue(int position) const
  { return doubleValue_[position];};
  inline int numberDoubleFields() const
  {return numberDoubleFields_;};
  inline int intValue(int position) const
  { return longValue_[position];};
  inline int numberIntFields() const
  {return numberIntFields_;};
  inline char charValue(int position) const
  { return charValue_[position];};
  inline int numberCharFields() const
  {return numberCharFields_;};
  inline std::string stringValue(int position) const
  { return stringValue_[position];};
  inline int numberStringFields() const
  {return numberStringFields_;};
  /// Current message
  inline CoinOneMessage  currentMessage() const
  {return currentMessage_;};
  /// Source of current message
  inline std::string currentSource() const
  {return source_;};
  /// Output buffer
  inline const char * messageBuffer() const
  {return messageBuffer_;};
  /// Highest message number (indicates any errors)
  inline int highestNumber() const
  {return highestNumber_;};
  //@}
  
  /**@name Actions to create a message  */
  //@{
  /// Start a message
  CoinMessageHandler & message(int messageNumber,
			      const CoinMessages & messages);
  /// returns current (if you want to make more uniform in look)
  CoinMessageHandler & message();
  /** Gets position of next field in format
      if initial then copies until first % */
  char * nextPerCent(char * start , const bool initial=false);
  /// Adds into message
  CoinMessageHandler & operator<< (int intvalue);
  CoinMessageHandler & operator<< (long longvalue);
  CoinMessageHandler & operator<< (double doublevalue);
  CoinMessageHandler & operator<< (std::string stringvalue);
  CoinMessageHandler & operator<< (char charvalue);
  CoinMessageHandler & operator<< (const char *stringvalue);
  CoinMessageHandler & operator<< (CoinMessageMarker);
  /** Stop (and print) - or use eol
  */
  int finish();
  /** Allows for skipping printing of part of message,
      but putting in data */
  CoinMessageHandler & printing(bool onOff);

  /** The following is to help existing codes interface
      Starts message giving number and complete text
  */
  CoinMessageHandler & message(int externalNumber,const char * header,
			      const char * msg,char severity);
  
  //@}
  
private:
  /**@name Private member data */
  //@{
  /// values in message
  double doubleValue_[10];
  long longValue_[10];
  char charValue_[10];
  std::string stringValue_[10];
  /// Log level
  int logLevel_;
  /// Whether we want prefix (may get more subtle so is int)
  int prefix_;
  /// Current message
  CoinOneMessage  currentMessage_;
  /// Internal number for use with enums
  int internalNumber_;
  /// Format string for message (remainder)
  char * format_;
  /// Number fields filled in,  0 in constructor then incremented
  int numberDoubleFields_;
  int numberIntFields_;
  int numberCharFields_;
  int numberStringFields_;
  /// Output buffer
  char messageBuffer_[1000];
  /// Position in output buffer
  char * messageOut_;
  /// Current source of message
  std::string source_;
  /** 0 - normal,
      1 - put in values, move along format, no print
      2 - put in values, no print
      3 - skip message
  */
  int printStatus_;
  /// Highest message number (indicates any errors)
  int highestNumber_;
  /// File pointer
  FILE * fp_;
   //@}
};

#endif
