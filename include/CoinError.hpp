// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
#ifndef CoinError_H
#define CoinError_H

#include <string>
//-------------------------------------------------------------------
//
// Error class used to throw exceptions
//
// Errors contain:
//
//-------------------------------------------------------------------

/** Error Class thrown by an exception

This class is used when exceptions are thrown.
It contains:
  <ul>
  <li>message text
  <li>name of method throwing exception
  <li>name of class throwing exception
  </ul>
*/
class CoinError  {
   friend void CoinErrorUnitTest();

public:
    
  //-------------------------------------------------------------------
  // Get methods
  //-------------------------------------------------------------------   
  /**@name Get error attributes */
  //@{
    /// get message text
    const std::string & message() const 
    { return message_; }
    /// get name of method instantiating error
    const std::string & methodName() const 
    { return method_;  }
    /// get name of class instantiating error
    const std::string & className() const 
    { return class_;   }
  //@}
  
    
  /**@name Constructors and destructors */
  //@{
    /// Default Constructor 
    CoinError ()
      :
      message_(),
      method_(),
      class_()
    {
      // nothing to do here
    }
  
    /// Alternate Constructor 
    CoinError ( 
      std::string message, 
      std::string methodName, 
      std::string className)
      :
      message_(message),
      method_(methodName),
      class_(className)
    {
      // nothing to do here
    }

    /// Other alternate Constructor 
    CoinError ( 
      const char * message, 
      const char * methodName, 
      const char * className)
      :
      message_(message),
      method_(methodName),
      class_(className)
    {
      // nothing to do here
    }

    /// Copy constructor 
    CoinError (const CoinError & source)
      :
      message_(source.message_),
      method_(source.method_),
      class_(source.class_)
    {
      // nothing to do here
    }

    /// Assignment operator 
    CoinError & operator=(const CoinError& rhs)
    {
      if (this != &rhs) {
	message_=rhs.message_;
	method_=rhs.method_;
	class_=rhs.class_;
      }
      return *this;
    }

    /// Destructor 
    virtual ~CoinError ()
    {
      // nothing to do here
    }
  //@}
    
private:
    
  /**@name Private member data */
  //@{
    /// message test
    std::string message_;
    /// method name
    std::string method_;
    /// class name
    std::string class_;
  //@}
};

//#############################################################################
/** A function that tests the methods in the CoinError class. The
    only reason for it not to be a member method is that this way it doesn't
    have to be compiled into the library. And that's a gain, because the
    library should be compiled with optimization on, but this method should be
    compiled with debugging. */
void
CoinErrorUnitTest();

#endif
