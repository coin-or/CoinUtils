// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinError.hpp"
#include <stdio.h>
// Just print (for asserts)
void 
CoinError::print() const
{ 
  if (lineNumber_<0) {
    std::cout<<message_<<" in "<<method_<<" class "<<class_<<std::endl;
  } else {
    std::cout<<method_<<":"<<lineNumber_<<" : assertion \'"<<message_<<"\' failed."<<std::endl;
    if(class_!="")
      std::cout<<"Possible reason: "<<class_<<std::endl;
  }
}
// Alternate Constructor for assert
CoinError::CoinError ( 
                      const char * assertion, 
                      const char * fileName, 
                      const char * hint,
                      int line)
  :
  message_(assertion),
  method_(fileName),
  class_(hint),
  lineNumber_(line)
{
}
// Copy constructor 
CoinError::CoinError (const CoinError & source)
  :
  message_(source.message_),
  method_(source.method_),
  class_(source.class_),
  lineNumber_(source.lineNumber_)
{
  // nothing to do here
}

// Assignment operator 
CoinError & 
CoinError::operator=(const CoinError& rhs)
{
  if (this != &rhs) {
    message_=rhs.message_;
    method_=rhs.method_;
    class_=rhs.class_;
    lineNumber_ = rhs.lineNumber_;
  }
  return *this;
}
