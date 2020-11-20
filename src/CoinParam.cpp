// Copyright (C) 2006, International Business Machines
// Corporation and others.  All Rights Reserved.
// This code is licensed under the terms of the Eclipse Public License (EPL).

#include "CoinUtilsConfig.h"

#include <string>
#include <cassert>
#include <iostream>
#include <sstream>
#ifdef COINUTILS_HAS_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#endif

#include "CoinPragma.hpp"
#include "CoinParam.hpp"
#include "CoinFinite.hpp"

/*
  Constructors and destructors

  There's a generic constructor and one for integer, double, keyword, string,
  and action parameters.
*/

/*
  Default constructor.
*/
CoinParam::CoinParam()
  : type_(paramInvalid)
  , name_()
  , lengthName_(0)
  , lengthMatch_(0)
  , lowerDblValue_(-COIN_DBL_MAX)
  , upperDblValue_(COIN_DBL_MAX)
  , dblValue_(0.0)
  , lowerIntValue_(-COIN_INT_MAX)
  , upperIntValue_(COIN_INT_MAX)
  , intValue_(0)
  , strValue_()
  , definedKwds_()
  , currentMode_(-1)
  , currentKwd_("")
  , pushFunc_(0)
  , pullFunc_(0)
  , shortHelp_()
  , longHelp_()
  , display_(displayPriorityNone)
{
  /* Nothing to be done here */
}

/*
  Constructor for double parameter
*/
CoinParam::CoinParam(std::string name, std::string help,
                     double lower, double upper, 
                     double defaultValue, std::string longHelp,
                     CoinDisplayPriority displayPriority)
  : type_(paramDbl)
  , name_(name)
  , lengthName_(0)
  , lengthMatch_(0)
  , lowerDblValue_(lower)
  , upperDblValue_(upper)
  , dblValue_(defaultValue)
  , lowerIntValue_(0)
  , upperIntValue_(0)
  , intValue_(0)
  , strValue_()
  , definedKwds_()
  , currentMode_(-1)
  , currentKwd_("")
  , pushFunc_(0)
  , pullFunc_(0)
  , shortHelp_(help)
  , longHelp_(longHelp)
  , display_(displayPriority)
{
  processName();
}

/*
  Constructor for integer parameter
*/
CoinParam::CoinParam(std::string name, std::string help,
                     int lower, int upper, 
                     int defaultValue, std::string longHelp,
                     CoinDisplayPriority displayPriority)
  : type_(paramInt)
  , name_(name)
  , lengthName_(0)
  , lengthMatch_(0)
  , lowerDblValue_(0.0)
  , upperDblValue_(0.0)
  , dblValue_(0.0)
  , lowerIntValue_(lower)
  , upperIntValue_(upper)
  , intValue_(defaultValue)
  , strValue_()
  , definedKwds_()
  , currentMode_(-1)
  , currentKwd_("")
  , pushFunc_(0)
  , pullFunc_(0)
  , shortHelp_(help)
  , longHelp_(longHelp)
  , display_(displayPriority)
{
  processName();
}

/*
  Constructor for keyword parameter.
*/
CoinParam::CoinParam(std::string name, std::string help,
                     std::string defaultKwd, int defaultMode,
                     std::string longHelp, CoinDisplayPriority displayPriority)
  : type_(paramKwd)
  , name_(name)
  , lengthName_(0)
  , lengthMatch_(0)
  , lowerDblValue_(0.0)
  , upperDblValue_(0.0)
  , dblValue_(0.0)
  , lowerIntValue_(0)
  , upperIntValue_(0)
  , intValue_(0)
  , strValue_()
  , definedKwds_()
  , currentMode_(defaultMode)
  , currentKwd_(defaultKwd)
  , pushFunc_(0)
  , pullFunc_(0)
  , shortHelp_(help)
  , longHelp_(longHelp)
  , display_(displayPriority)
{
  processName();
  definedKwds_[defaultKwd] = defaultMode;
}

/*
  Constructor for string parameter.
*/
CoinParam::CoinParam(std::string name, std::string help,
                     std::string defaultValue, std::string longHelp,
                     CoinDisplayPriority displayPriority)
  : type_(paramStr)
  , name_(name)
  , lengthName_(0)
  , lengthMatch_(0)
  , lowerDblValue_(0.0)
  , upperDblValue_(0.0)
  , dblValue_(0.0)
  , lowerIntValue_(0)
  , upperIntValue_(0)
  , intValue_(0)
  , strValue_(defaultValue)
  , definedKwds_()
  , currentMode_(0)
  , currentKwd_("")
  , pushFunc_(0)
  , pullFunc_(0)
  , shortHelp_(help)
  , longHelp_(longHelp)
  , display_(displayPriority)
{
  processName();
}

/*
  Constructor for action parameter.
*/
CoinParam::CoinParam(std::string name, std::string help,
                     std::string longHelp, CoinDisplayPriority displayPriority)
  : type_(paramAct)
  , name_(name)
  , lengthName_(0)
  , lengthMatch_(0)
  , lowerDblValue_(0.0)
  , upperDblValue_(0.0)
  , dblValue_(0.0)
  , lowerIntValue_(0)
  , upperIntValue_(0)
  , intValue_(0)
  , strValue_()
  , definedKwds_()
  , currentMode_(0)
  , currentKwd_("")
  , pushFunc_(0)
  , pullFunc_(0)
  , shortHelp_(help)
  , longHelp_(longHelp)
  , display_(displayPriority)
{
  processName();
}

/*
  Copy constructor.
*/
CoinParam::CoinParam(const CoinParam &orig)
  : type_(orig.type_)
  , lengthName_(orig.lengthName_)
  , lengthMatch_(orig.lengthMatch_)
  , lowerDblValue_(orig.lowerDblValue_)
  , upperDblValue_(orig.upperDblValue_)
  , dblValue_(orig.dblValue_)
  , lowerIntValue_(orig.lowerIntValue_)
  , upperIntValue_(orig.upperIntValue_)
  , intValue_(orig.intValue_)
  , currentMode_(orig.currentMode_)
  , currentKwd_(orig.currentKwd_)
  , pushFunc_(orig.pushFunc_)
  , pullFunc_(orig.pullFunc_)
  , display_(orig.display_)
{
  name_ = orig.name_;
  strValue_ = orig.strValue_;
  definedKwds_ = orig.definedKwds_;
  shortHelp_ = orig.shortHelp_;
  longHelp_ = orig.longHelp_;
}

/*
  Clone
*/

CoinParam *CoinParam::clone()
{
  return (new CoinParam(*this));
}

CoinParam &CoinParam::operator=(const CoinParam &rhs)
{
  if (this != &rhs) {
    type_ = rhs.type_;
    name_ = rhs.name_;
    lengthName_ = rhs.lengthName_;
    lengthMatch_ = rhs.lengthMatch_;
    lowerDblValue_ = rhs.lowerDblValue_;
    upperDblValue_ = rhs.upperDblValue_;
    dblValue_ = rhs.dblValue_;
    lowerIntValue_ = rhs.lowerIntValue_;
    upperIntValue_ = rhs.upperIntValue_;
    intValue_ = rhs.intValue_;
    strValue_ = rhs.strValue_;
    definedKwds_ = rhs.definedKwds_;
    currentMode_ = rhs.currentMode_;
    currentKwd_ = rhs.currentKwd_;
    pushFunc_ = rhs.pushFunc_;
    pullFunc_ = rhs.pullFunc_;
    shortHelp_ = rhs.shortHelp_;
    longHelp_ = rhs.longHelp_;
    display_ = rhs.display_;
  }

  return *this;
}

/*
  Destructor
*/
CoinParam::~CoinParam()
{ /* Nothing more to do */
}

/*
  Methods to manipulate a CoinParam object.
*/

/* Methods to initialize a parameter that's already constructed */ 

/* Set up a double parameter */
void CoinParam::setup(std::string name, std::string help,
                      double lower, double upper,
                      double defaultValue, std::string longHelp,
                      CoinDisplayPriority display){
   name_ = name;
   processName();
   type_ = paramDbl;
   shortHelp_ = help;
   lowerDblValue_ = lower;
   upperDblValue_ = upper;
   dblValue_ = defaultValue;
   longHelp_ = longHelp;
   display_ = display;
}

/* Set up an integer parameter */
void CoinParam::setup(std::string name, std::string help,
                      int lower, int upper,
                      int defaultValue, std::string longHelp,
                      CoinDisplayPriority display){
   name_ = name;
   processName();
   type_ = paramInt;
   shortHelp_ = help;
   lowerIntValue_ = lower;
   upperIntValue_ = upper;
   intValue_ = defaultValue;
   longHelp_ = longHelp;
   display_ = display;
}

/* Set up a keyword parameter */
void CoinParam::setup(std::string name, std::string help,
                      std::string defaultKwd, int defaultMode,
                      std::string longHelp,
                      CoinDisplayPriority display){
   name_ = name;
   processName();
   type_ = paramKwd;
   shortHelp_ = help;
   currentKwd_ = defaultKwd;
   currentMode_ = defaultMode;
   definedKwds_[defaultKwd] = defaultMode;
   longHelp_ = longHelp;
   display_ = display;
}

/* Set up a string parameter */
void CoinParam::setup(std::string name, std::string help,
                      std::string defaultValue, std::string longHelp,
                      CoinDisplayPriority display){
   name_ = name;
   processName();
   type_ = paramStr;
   shortHelp_ = help;
   strValue_ = defaultValue;
   longHelp_ = longHelp;
   display_ = display;
}

/* Set up an action parameter */
void CoinParam::setup(std::string name, std::string help, std::string longHelp,
                      CoinDisplayPriority display){
   name_ = name;
   processName();
   type_ = paramAct;
   shortHelp_ = help;
   longHelp_ = longHelp;
   display_ = display;
}

/*
  Process the parameter name.
  
  Process the name for efficient matching: determine if an `!' is present. If
  so, locate and record the position and remove the `!'.
*/

void CoinParam::processName()

{
  std::string::size_type shriekPos = name_.find('!');
  lengthName_ = name_.length();
  if (shriekPos == std::string::npos) {
    lengthMatch_ = lengthName_;
  } else {
    lengthMatch_ = shriekPos;
    name_ = name_.substr(0, shriekPos) + name_.substr(shriekPos + 1);
    lengthName_--;
  }

  return;
}

/*
  Check an input string to see if it matches the parameter name. The whole
  input string must match, and the length of the match must exceed the
  minimum match length. A match is impossible if the string is longer than
  the name.

  Returns: 0 for no match, 1 for a successful match, 2 if the match is short
*/
int CoinParam::matches(std::string input) const
{
  size_t inputLen = input.length();
  if (inputLen <= lengthName_) {
    size_t i;
    for (i = 0; i < inputLen; i++) {
      if (tolower(name_[i]) != tolower(input[i]))
        break;
    }
    if (i < inputLen) {
      return (0);
    } else if (i >= lengthMatch_) {
      return (1);
    } else {
      return (2);
    }
  }

  return (0);
}

/*
  Return the parameter name, formatted to indicate how it'll be matched.
  E.g., some!Name will come back as some(Name).
*/
std::string CoinParam::matchName() const
{
  if (lengthMatch_ == lengthName_) {
    return name_;
  } else {
    return name_.substr(0, lengthMatch_) + "(" + name_.substr(lengthMatch_) + ")";
  }
}

/*
  Return the length of the formatted paramter name for printing.
*/
int CoinParam::lengthMatchName() const {
  if (lengthName_ == lengthMatch_)
    return lengthName_;
  else
    return lengthName_ + 2;
}

/*
  Print the long help message and a message about appropriate values.
*/
std::string CoinParam::printLongHelp() const
{
  std::ostringstream buffer;
  if (longHelp_ != "") {
    CoinParamUtils::printIt(longHelp_.c_str());
  } else if (shortHelp_ != "") {
    CoinParamUtils::printIt(shortHelp_.c_str());
  } else {
    CoinParamUtils::printIt("No help provided.");
  }

  switch (type_) {
   case paramDbl: {
      buffer << "<Range of values is " << lowerDblValue_ << " to "
             << upperDblValue_ << ";\n\tcurrent " << dblValue_ << ">"
             << std::endl;
      assert(upperDblValue_ > lowerDblValue_);
      break;
   }
   case paramInt: {
      buffer << "<Range of values is " << lowerIntValue_ << " to "
             << upperIntValue_ << ";\n\tcurrent " << intValue_ << ">"
             << std::endl;
      assert(upperIntValue_ > lowerIntValue_);
      break;
   }
   case paramKwd: {
      printKwds();
      break;
   }
   case paramStr: {
      buffer << "<Current value is ";
      if (strValue_ == "") {
         buffer << "(unset)>";
      } else {
         buffer << "`" << strValue_ << "'>";
      }
      buffer << std::endl;
      break;
   }
   case paramAct: {
      break;
   }
   default: {
      buffer << "!! invalid parameter type !!" << std::endl;
      assert(false);
   }
  }
  return buffer.str();
}

/*
  Methods to manipulate the value of a parameter.
*/

/*
  Methods to manipulate the values associated with a keyword parameter.
*/

/* Set value of a parameter of any type */
int CoinParam::setVal(std::string value, std::string *message,
                        ParamPushMode pMode)
{
   switch(type_){
    case paramKwd:
      setKwdVal(value, message, pMode);
      return 0;
    case paramStr:
      setStrVal(value, message, pMode);
      return 0;
    default:
      std::cout << "setVal(): Parameter type must be set to string or "
                << "keyword for this function call!" << std::endl;
      return 1;
   }
}

int CoinParam::setVal(double value, std::string *message, ParamPushMode pMode)
{
   if (type_ == paramDbl){
      setDblVal(value, message, pMode);
      return 0;
   }else{
      std::cout << "setVal(): Parameter type must be set to double "
                << "for this function call!" << std::endl;
      return 1;
   }
}

int CoinParam::setVal(int value, std::string *message, ParamPushMode pMode)
{
   switch(type_){
    case paramInt:
      setIntVal(value, message, pMode);
      return 0;
    case paramKwd:
      setModeVal(value, message, pMode);
      return 0;
    default:
      std::cout << "setVal(): Parameter type must be set to int "
                << "for this function call!" << std::endl;
      return 1;
   }
}

/* Get value of a parameter of any type */
int CoinParam::getVal(std::string &value)
{
   switch(type_){
    case paramKwd:
      value = currentKwd_;
      return 0;
    case paramStr:
      value = strValue_;
      return 0;
    default:
      std::cout << "getVal(): Parameter type must be set to string or "
                << "keyword for this function call!" << std::endl;
      return 1;
   }      
}

int CoinParam::getVal(double &value)
{
   if (type_ == paramDbl){
      value = dblValue_;
      return 0;
   }else{
      std::cout << "getVal(): Parameter type must be set to double "
                << "for this function call!" << std::endl;
      return 1;
   }
}

int CoinParam::getVal(int &value)
{
   switch(type_){
    case paramInt:
      value = intValue_;
      return 0;
    case paramKwd:
      value = currentMode_;
      return 0;
    default:
      std::cout << "getVal(): Parameter type must be set to int "
                << "for this function call!" << std::endl;
      return 1;
   }

}

   /*
  Add a keyword to the list for a keyword parameter.

  Note, we don't check to make sure the index is not already used.
  The assumption is that we are mapping to an enum and the indices are
  thus automatically deconflicted.
*/
void CoinParam::appendKwd(std::string kwd, int mode)
{
  definedKwds_[kwd] = mode;
}

void CoinParam::appendKwd(std::string kwd)
{
  definedKwds_[kwd] = definedKwds_.size();
}

/*
  Scan the keywords of a keyword parameter and return the integer index of
  the keyword matching the input, or -1 for no match.
*/
int CoinParam::kwdToMode(std::string input) const
{
  assert(type_ == paramKwd);

  std::map<std::string, int>::const_iterator it;
  
  it = definedKwds_.find(input);

  if (it == definedKwds_.end()) {
      return -1;
   }else{
      return it->second;
   }

#if 0

  // This code scans through the list and allows for partial matches,
  // It's not finding much use right now and complicates matters.
  
  int whichItem = -1;
  size_t numberItems = definedKwds_.size();
  if (numberItems > 0) {
    size_t inputLen = input.length();
    /*
  Open a loop to check each keyword against the input string. We don't record
  the match length for keywords, so we need to check each one for an `!' and
  do the necessary preprocessing (record position and elide `!') before
  checking for a match of the required length.
*/
    for (it = definedKwds_.begin(); it != definedKwds_.end(); it++) {
      std::string kwd = definedKwds_[it];
      std::string::size_type shriekPos = kwd.find('!');
      size_t kwdLen = kwd.length();
      size_t matchLen = kwdLen;
      if (shriekPos != std::string::npos) {
        matchLen = shriekPos;
        kwd = kwd.substr(0, shriekPos) + kwd.substr(shriekPos + 1);
        kwdLen = kwd.length();
      }
      /*
  Match is possible only if input is shorter than the keyword. The entire input
  must match and the match must exceed the minimum length.
*/
      if (inputLen <= kwdLen) {
        unsigned int i;
        for (i = 0; i < inputLen; i++) {
          if (tolower(kwd[i]) != tolower(input[i]))
            break;
        }
        if (i >= inputLen && i >= matchLen) {
          whichItem = static_cast< int >(it);
          break;
        }
      }
    }
  }

  return (whichItem);
#endif
}

/*
  Set current value for a keyword parameter using a string.
*/
int CoinParam::setKwdVal(const std::string newKwd, std::string *message,
                         ParamPushMode pMode)
{
  assert(type_ == paramKwd);

  int newMode = kwdToMode(newKwd);
  if (newMode >= 0) {
     if (message){
        std::ostringstream buffer;
        buffer << "Option for " << name_ << " changed from ";
        buffer << currentKwd_ << " to " << newKwd << std::endl;
        *message = buffer.str();
     }
     currentKwd_ = newKwd;
     currentMode_ = newMode;
     if (pMode == pushOn){
        pushFunc_(*this);
     }
     return 0;
  }else{
     if (message){
        std::ostringstream buffer;
        buffer << "Illegal keyword. Options are" << std::endl;
        *message = buffer.str();
     }
     return 1;
  }
}

/*
  Set current value for keyword parameter using an integer. Echo the new value
  to cout if requested.
*/
int CoinParam::setModeVal(int newMode, std::string *message, ParamPushMode pMode)
{
  assert(type_ == paramKwd);

  std::map<std::string, int>::const_iterator it;
  std::string newKwd;
  for (it = definedKwds_.begin(); it != definedKwds_.end(); it++) {
     if (it->second == newMode){
        newKwd = it->first;
     }
  }
  if (it == definedKwds_.end()){
     if (message){
        std::ostringstream buffer;  
        buffer << "Illegal keyword." << printKwds() << std::endl;
        *message = buffer.str();
     }
     return 1;
  }else{
     if (message){
        std::ostringstream buffer;  
        buffer << "Option for " << name_ << " changed from ";
        buffer << currentKwd_ << " to " << newKwd << std::endl;
        *message = buffer.str();
     }
     currentKwd_ = newKwd;
     currentMode_ = newMode;
     if (pMode == pushOn){
        pushFunc_(*this);
     }
     return 0;
  }
}

/*
  Return the string corresponding to the current value.
*/
std::string CoinParam::kwdVal() const
{
  assert(type_ == paramKwd);

  return (currentKwd_);
}

/*
  Return the string corresponding to the current value.
*/
int CoinParam::modeVal() const
{
  assert(type_ == paramKwd);

  return (currentMode_);
}

/*
  Print the keywords for a keyword parameter, formatted to indicate how they'll
  be matched. (E.g., some!Name prints as some(Name).). Follow with current
  value.
*/
std::string CoinParam::printKwds() const
{
  assert(type_ == paramKwd);

  std::ostringstream buffer;
  buffer << "Possible options for " << name_ << " are:";
  int maxAcross = 5;
  std::map<std::string, int>::const_iterator it;
  int i;
  for (it = definedKwds_.begin(), i = 0; it != definedKwds_.end(); it++, i++) {
     std::string kwd = it->first;
    std::string::size_type shriekPos = kwd.find('!');
    if (shriekPos != std::string::npos) {
      kwd = kwd.substr(0, shriekPos) + "(" + kwd.substr(shriekPos + 1) + ")";
    }
    if (i % maxAcross == 0) {
      buffer << std::endl;
    }
    buffer << "  " << kwd;
  }
  buffer << std::endl;

  assert(currentMode_ >= 0);

  std::string current = currentKwd_;
  std::string::size_type shriekPos = current.find('!');
  if (shriekPos != std::string::npos) {
    current = current.substr(0, shriekPos) + "(" + current.substr(shriekPos + 1) + ")";
  }
  buffer << "  <current: " << current << ">" << std::endl;

  return buffer.str();
}

/*
  Methods to manipulate the value of a string parameter.
*/

int CoinParam::setStrVal(std::string value, std::string *message, ParamPushMode pMode)
{
  assert(type_ == paramStr);

  if (message){
     std::ostringstream buffer;  
     buffer << name_ << " was changed from ";
     buffer << strValue_ << " to " << value << std::endl;
     *message = buffer.str();
  }
  strValue_ = value;
  if (pMode == pushOn){
     pushFunc_(*this);
  }
  return 0;
}

std::string CoinParam::strVal() const
{
  assert(type_ == paramStr);

  return (strValue_);
}

/*
  Methods to manipulate the value of a double parameter.
*/

int CoinParam::setDblVal(double value, std::string *message, ParamPushMode pMode)
{
  assert(type_ == paramDbl);

  if (value < lowerDblValue_ || value > upperDblValue_) {
     if (message){
        std::ostringstream buffer;
        buffer << value << " was provided for " << name_;
        buffer << " - valid range is " << lowerDblValue_;
        buffer << " to " << upperDblValue_ << std::endl;
        *message = buffer.str();
     }
     return 1;
  } else {
     if (message){
        std::ostringstream buffer;
        buffer << name_ << " was changed from ";
        buffer << dblValue_ << " to " << value << std::endl;
        *message = buffer.str();
     }
     dblValue_ = value;
     if (pMode == pushOn){
        pushFunc_(*this);
     }
     return 0;
  }
}

double CoinParam::dblVal() const
{
  assert(type_ == paramDbl);

  return (dblValue_);
}

void CoinParam::setLowerDblVal(double value)
{
  assert(type_ == paramDbl);

  lowerDblValue_ = value;
}

double CoinParam::lowerDblVal() const
{
  assert(type_ == paramDbl);

  return(lowerDblValue_);
}

void CoinParam::setUpperDblVal(double value)
{
  assert(type_ == paramDbl);

  upperDblValue_ = value;
}

double CoinParam::upperDblVal() const
{
  assert(type_ == paramDbl);

  return(upperDblValue_);
}

/*
  Methods to manipulate the value of an integer parameter.
*/

int CoinParam::setIntVal(int value, std::string *message, ParamPushMode pMode)
{
  assert(type_ == paramInt);

  if (value < lowerIntValue_ || value > upperIntValue_) {
     if (message){
        std::ostringstream buffer;
        buffer << value << " was provided for " << name_;
        buffer << " - valid range is " << lowerIntValue_ << " to ";
        buffer << upperIntValue_ << std::endl;
        *message = buffer.str();
     }
     return 1;
  } else {
     if (message){
        std::ostringstream buffer;
        buffer << name_ << " was changed from " << intValue_;
        buffer << " to " << value << std::endl;
        *message = buffer.str();
     }
     intValue_ = value;
     if (pMode == pushOn){
        pushFunc_(*this);
     }
     return 0;
  }
}

int CoinParam::intVal() const
{
  assert(type_ == paramInt);

  return (intValue_);
}

void CoinParam::setLowerIntVal(int value)
{
  assert(type_ == paramInt);

  lowerIntValue_ = value;
}

int CoinParam::lowerIntVal() const
{
  assert(type_ == paramInt);

  return(lowerIntValue_);
}

void CoinParam::setUpperIntVal(int value)
{
  assert(type_ == paramInt);

  upperIntValue_ = value;
}

int CoinParam::upperIntVal() const
{
  assert(type_ == paramInt);

  return(upperIntValue_);
}

// Prints parameter options
void CoinParam::printOptions()
{
   std::cout << "<Possible options for " << name_ << " are:";
   std::map<std::string, int>::const_iterator it;
   for (it = definedKwds_.begin(); it != definedKwds_.end(); it++) {
      std::string thisOne = it->first;
      std::string::size_type shriekPos = thisOne.find('!');
      if (shriekPos != std::string::npos) {
         //contains '!'
         thisOne = thisOne.substr(0, shriekPos) + "(" +
            thisOne.substr(shriekPos + 1) + ")";
      }
      std::cout << " " << thisOne;
   }
   assert(currentMode_ >= 0 &&
          currentMode_ < static_cast< int >(definedKwds_.size()));
   std::string::size_type shriekPos = currentKwd_.find('!');
   std::string current;
   if (shriekPos != std::string::npos) {
      //contains '!'
      current = currentKwd_.substr(0, shriekPos) + "(" +
         currentKwd_.substr(shriekPos + 1) + ")";
   }
   std::cout << ";\n\tcurrent  " << current << ">" << std::endl;
}

/*
  A print function (friend of the class)
*/

std::ostream &operator<<(std::ostream &s, const CoinParam &param)
{
  switch (param.type()) {
  case CoinParam::paramDbl: {
    return (s << param.dblVal());
  }
  case CoinParam::paramInt: {
    return (s << param.intVal());
  }
  case CoinParam::paramKwd: {
    return (s << param.kwdVal());
  }
  case CoinParam::paramStr: {
    return (s << param.strVal());
  }
  case CoinParam::paramAct: {
    return (s << "<evokes action>");
  }
  default: {
    return (s << "!! invalid parameter type !!");
  }
  }
}

std::string CoinPrintString(const std::string input, int maxWidth)
{
   std::ostringstream buffer;
   std::istringstream iss(input);
   int currentWidth = 0;
   std::string word;
   while (iss >> word){
      currentWidth += word.length();
      if (currentWidth >= 65){
         buffer << std::endl << word;
      } else {
         buffer << word;
      }         
   }
   return buffer.str();
}

std::string CoinPrintString(const char *input, int maxWidth)
{
   return CoinPrintString(std::string(input));
}

void
CoinReadFromStream(std::vector<std::string> &inputVector,
                   std::istream &inputStream)
{
   inputVector.clear();
   std::string field;
   while (inputStream >> field){
      std::string::size_type found = field.find('=');
      if (found != std::string::npos) {
         inputVector.push_back(field.substr(0, found));
         inputVector.push_back(field.substr(found + 1));
      } else {
         inputVector.push_back(field);
      }
   }
}

void
CoinReadInteractiveInput(std::vector<std::string> &inputVector,
                         std::string prompt)
{
   std::string input;
   inputVector.clear();
  
#ifdef COINUTILS_HAS_READLINE
   // Get a line from the user.
   input = std::string(readline(prompt.c_str()));
   // If the line has any text in it, save it on the history.
   if (input.length() > 0) {
      add_history(input.c_str());
   }
#else
   std::cout << prompt;
   fflush(stdout);
   getline(std::cin, input); 
#endif
   if (!input.length()){
      return; 
   }

   std::istringstream inputStream(input);
   CoinReadFromStream(inputVector, inputStream);
}

std::string
CoinGetCommand(std::vector<std::string> &inputVector, int &whichField,
               bool &interactiveMode, std::string prompt)
{
  if ((whichField < 0 || whichField >= inputVector.size())){
     if (interactiveMode){
        CoinReadInteractiveInput(inputVector, prompt);
        if (inputVector.size()){
           whichField = 0;
        }else{
           whichField = -1;
           return "";
        }
     }else{
        return "";
     }
  }

  return inputVector[whichField++];
}

std::string
CoinGetString(std::vector<std::string> &inputVector, int &whichField,
              bool &interactiveMode, std::string prompt)
{
   std::string field = "";

   if (whichField < 0 || whichField >= inputVector.size()){
      if (interactiveMode) {
         // may be negative value so do not check for -
         CoinReadInteractiveInput(inputVector, prompt);
         whichField = 0;
      }else{
         return field;
      }
   }

  std::string value = inputVector[whichField++];
  
  if (value == "--" || value == "stdin"){
     field = "-";
  } else if (value == "stdin_lp"){
     field = "-lp";
  }else{
     field = value;
  }

  return field;
}

// status 0 - okay, 1 bad, 2 not there
int CoinGetInt(std::vector<std::string> &inputVector, int &whichField,
               int &status, bool &interactiveMode, std::string prompt)
{
  if (whichField < 0 || whichField >= inputVector.size()){
     if (interactiveMode) {
        CoinReadInteractiveInput(inputVector, prompt);
        whichField = 0;
     } else {
        // Nothing to read
        status = 2;
        return 0;
     }
  }

  std::string field = inputVector[whichField++];

  int value(0);
      
  std::stringstream ss(field);
  status =  (ss >> value) ? 0:1;
  
  return value;
}

double
CoinGetDouble(std::vector<std::string> &inputVector, int &whichField,
              int &status, bool &interactiveMode, std::string prompt)
{
  if (whichField < 0 || whichField >= inputVector.size()){
     if (interactiveMode) {
        CoinReadInteractiveInput(inputVector, prompt);
        whichField = 0;
     } else {
        // Nothing to read
        status = 2;
        return 0.0;
     }
  }

  std::string field = inputVector[whichField++];

  double value(0.0);
  
  std::stringstream ss(field);
  status =  (ss >> value) ? 0:1;

  return value;
}

/* vi: softtabstop=2 shiftwidth=2 expandtab tabstop=2
*/
