// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// Test individual classes or groups of classes

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <cassert>
#include <iostream>

#include "CoinError.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinSort.hpp"
#include "CoinShallowPackedVector.hpp"
#include "CoinPackedVector.hpp"
#include "CoinDenseVector.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinMpsIO.hpp"

// Function Prototypes. Function definitions is in this file.
void testingMessage( const char * const msg );

//----------------------------------------------------------------
// unitTest [-mpsDir=V1] [-netlibDir=V2]
// 
// where:
//   -mpsDir: directory containing mps test files
//       Default value V1="../Mps/Sample"    
//   -netlibDir: directory containing netlib files
//       Default value V2="../Mps/Netlib"
//
// All parameters are optional.
//----------------------------------------------------------------

int main (int argc, const char *argv[])
{
  int i;

  // define valid parameter keywords
  std::set<std::string> definedKeyWords;
  definedKeyWords.insert("-mpsDir");
  definedKeyWords.insert("-netlibDir");

  // Create a map of parmater keys and associated data
  std::map<std::string,std::string> parms;
  for ( i=1; i<argc; i++ ) {
    std::string parm(argv[i]);
    std::string key,value;
    unsigned int  eqPos = parm.find('=');

    // Does parm contain and '='
    if ( eqPos==std::string::npos ) {
      //Parm does not contain '='
      key = parm;
    }
    else {
      key=parm.substr(0,eqPos);
      value=parm.substr(eqPos+1);
    }

    // Is specifed key valid?
    if ( definedKeyWords.find(key) == definedKeyWords.end() ) {
      // invalid key word.
      // Write help text
      std::cerr <<"Undefined parameter \"" <<key <<"\".\n";
      std::cerr <<"Correct usage: \n";
      std::cerr <<"  unitTest [-mpsDir=V1] [-netlibDir=V2]\n";
      std::cerr <<"  where:\n";
      std::cerr <<"    -mpsDir: directory containing mps test files\n";
      std::cerr <<"        Default value V1=\"../Mps/Sample\"\n";
      std::cerr <<"    -netlibDir: directory containing netlib files\n";
      std::cerr <<"        Default value V2=\"../Mps/Netlib\"\n";
      return 1;
    }
    parms[key]=value;
  }
  
  const char dirsep =  CoinFindDirSeparator();
  // Set directory containing mps data files.
  std::string mpsDir;
  if (parms.find("-mpsDir") != parms.end())
    mpsDir=parms["-mpsDir"] + dirsep;
  else 
    mpsDir = dirsep == '/' ? "../Mps/Sample/" : "..\\Mps\\Sample\\";
 
  // Set directory containing netlib data files.
  std::string netlibDir;
  if (parms.find("-netlibDir") != parms.end())
    netlibDir=parms["-netlibDir"] + dirsep;
  else 
    netlibDir = dirsep == '/' ? "../Mps/Netlib/" : "..\\Mps\\Netlib\\";

  // *FIXME* : these tests should be written... 
  //  testingMessage( "Testing CoinHelperFunctions\n" );
  //  CoinHelperFunctionsUnitTest();
  //  testingMessage( "Testing CoinSort\n" );
  //  tripleCompareUnitTest();

  testingMessage( "Testing CoinError\n" );
  CoinErrorUnitTest();

  testingMessage( "Testing CoinShallowPackedVector\n" );
  CoinShallowPackedVectorUnitTest();

  testingMessage( "Testing CoinPackedVector\n" );
  CoinPackedVectorUnitTest();

  testingMessage( "Testing CoinIndexedVector\n" );
  CoinIndexedVectorUnitTest();

  testingMessage( "Testing CoinPackedMatrix\n" );
  CoinPackedMatrixUnitTest();

  
// At moment CoinDenseVector is not compiling with MS V C++ V6
#if 1
  testingMessage( "Testing CoinDenseVector\n" );
  //CoinDenseVectorUnitTest<int>(0);
  CoinDenseVectorUnitTest<double>(0.0);
  CoinDenseVectorUnitTest<float>(0.0f);
#endif


  testingMessage( "Testing CoinMpsIO\n" );
  CoinMpsIOUnitTest(mpsDir);

  testingMessage( "All tests completed successfully\n" );
  return 0;
}

 
// Display message on stdout and stderr
void testingMessage( const char * const msg )
{
  std::cerr <<msg;
  //cout <<endl <<"*****************************************"
  //     <<endl <<msg <<endl;
}

