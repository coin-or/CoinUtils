// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
#ifndef CoinError_H
#define CoinError_H

#include <string>
#include <iostream>
#include <cassert>
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
  <li>name of class throwing exception or hint
  <li>name of file if assert
  <li>line number
  </ul>
  For asserts class=> optional hint
*/
class CoinError  {
   friend void CoinErrorUnitTest();

private:
    CoinError()
      :
      message_(),
      method_(),
      class_(),
      file_(),
      lineNumber_(-1)
    {
      // nothing to do here
    }

public:
    
  //-------------------------------------------------------------------
  // Get methods
  //-------------------------------------------------------------------   
  /**@name Get error attributes */
  //@{
    /// get message text
    inline const std::string & message() const 
    { return message_; }
    /// get name of method instantiating error
    inline const std::string & methodName() const 
    { return method_;  }
    /// get name of class instantiating error (or hint for assert)
    inline const std::string & className() const 
    { return class_;   }
    /// get name of file for assert
    inline const std::string & fileName() const 
    { return file_;  }
    /// get line number of assert (-1 if not assert)
    inline int lineNumber() const 
    { return lineNumber_;   }
    /// Just print (for asserts)
    void print() const;
  //@}
  
    
  /**@name Constructors and destructors */
  //@{
    /// Alternate Constructor 
    CoinError ( 
      std::string message, 
      std::string methodName, 
      std::string className)
      :
      message_(message),
      method_(methodName),
      class_(className),
      file_(),
      lineNumber_(-1)
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
      class_(className),
      file_(),
      lineNumber_(-1)
    {
      // nothing to do here
    }

    /// Other alternate Constructor for assert
    CoinError ( 
      const char * assertion, 
      const char * methodName, 
      const char * hint,
      const char * fileName, 
      int line);

    /// Copy constructor 
    CoinError (const CoinError & source);

    /// Assignment operator 
    CoinError & operator=(const CoinError& rhs);

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
    /// class name or hint
    std::string class_;
    /// file name
    std::string file_;
    /// Line number
    int lineNumber_;
  //@}
};

#ifndef __GNUC_PREREQ
# define __GNUC_PREREQ(maj, min) (0)
#endif 

#ifndef __STRING
#define __STRING(x)	#x
#endif

#ifndef COIN_ASSERT
#   define CoinAssertDebug(expression) assert(expression)
#   define CoinAssertDebugHint(expression,hint) assert(expression)
#   define CoinAssert(expression) assert(expression)
#   define CoinAssertHint(expression,hint) assert(expression)
#else
#   ifdef NDEBUG
#      define CoinAssertDebug(expression)		{}
#      define CoinAssertDebugHint(expression,hint)	{}
#   else
#      if defined(__GNUC__) && __GNUC_PREREQ(2, 6)
#         define CoinAssertDebug(expression) { 				   \
             if (!(expression)) {					   \
                throw CoinError(__STRING(expression), __PRETTY_FUNCTION__, \
                                "", __FILE__, __LINE__);		   \
             }								   \
          }
#         define CoinAssertDebugHint(expression,hint) {			   \
             if (!(expression)) {					   \
                throw CoinError(__STRING(expression), __PRETTY_FUNCTION__, \
                                hint, __FILE__,__LINE__);		   \
             }								   \
          }
#      else
#         define CoinAssertDebug(expression) {				   \
             if (!(expression)) {					   \
                throw CoinError(__STRING(expression), "",		   \
                                "", __FILE__,__LINE__);			   \
             }								   \
          }
#         define CoinAssertDebugHint(expression,hint) {			   \
             if (!(expression)) {					   \
                throw CoinError(__STRING(expression), "",		   \
                                hint, __FILE__,__LINE__);		   \
             }								   \
          }
#      endif
#   endif
#   if defined(__GNUC__) && __GNUC_PREREQ(2, 6)
#      define CoinAssert(expression) { 					\
          if (!(expression)) {						\
             throw CoinError(__STRING(expression), __PRETTY_FUNCTION__, \
                             "", __FILE__, __LINE__);			\
          }								\
       }
#      define CoinAssertHint(expression,hint) {				\
          if (!(expression)) {						\
             throw CoinError(__STRING(expression), __PRETTY_FUNCTION__, \
                             hint, __FILE__,__LINE__);			\
          }								\
       }
#   else
#      define CoinAssert(expression) {					\
          if (!(expression)) {						\
             throw CoinError(__STRING(expression), "",			\
                             "", __FILE__,__LINE__);			\
          }								\
       }
#      define CoinAssertHint(expression,hint) {				\
          if (!(expression)) {						\
             throw CoinError(__STRING(expression), "",			\
                             hint, __FILE__,__LINE__);			\
          }								\
       }
#   endif
#endif


//#############################################################################
/** A function that tests the methods in the CoinError class. The
    only reason for it not to be a member method is that this way it doesn't
    have to be compiled into the library. And that's a gain, because the
    library should be compiled with optimization on, but this method should be
    compiled with debugging. */
void
CoinErrorUnitTest();

#endif
