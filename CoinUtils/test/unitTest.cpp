// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
// Test individual classes or groups of classes

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <iostream>

#undef MY_C_FINITE

#include "CoinError.hpp"
#include "CoinHelperFunctions.hpp"
#include "CoinSort.hpp"
#include "CoinShallowPackedVector.hpp"
#include "CoinPackedVector.hpp"
#include "CoinDenseVector.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinMpsIO.hpp"
#include "CoinMessageHandler.hpp"
void CoinModelUnitTest(const std::string & mpsDir,
                       const std::string & netlibDir, const std::string & testModel);
// Function Prototypes. Function definitions is in this file.
void testingMessage( const char * const msg );

//----------------------------------------------------------------
// unitTest [-mpsDir=V1] [-netlibDir=V2] [-testModel=V3]
// 
// where:
//   -mpsDir: directory containing mps test files
//       Default value V1="../../Data/Sample"    
//   -netlibDir: directory containing netlib files
//       Default value V2="../../Data/Netlib"
//   -testModel: name of model in netlibdir for testing CoinModel
//       Default value V3="25fv47.mps"
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
  // allow for large named model for CoinModel
  definedKeyWords.insert("-testModel");

  // Create a map of parameter keys and associated data
  std::map<std::string,std::string> parms;
  for ( i=1; i<argc; i++ ) {
    std::string parm(argv[i]);
    std::string key,value;
    std::string::size_type eqPos = parm.find('=');

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
      std::cerr
	  <<"Undefined parameter \"" <<key <<"\".\n"
	  <<"Correct usage: \n"
	  <<"  unitTest [-mpsDir=V1] [-netlibDir=V2] [-testModel=V3]\n"
	  <<"  where:\n"
	  <<"    -mpsDir: directory containing mps test files\n"
	  <<"        Default value V1=\"../../Data/Sample\"\n"
	  <<"    -netlibDir: directory containing netlib files\n"
	  <<"        Default value V2=same as mpsDir\n"
	  <<"    -testModel: name of model in netlibdir for testing CoinModel\n"
	  <<"        Default value V3=\"adlittle.mps\"\n";
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
    mpsDir = dirsep == '/' ? "../../Data/Sample/" : "..\\..\\Data\\Sample\\";
 
  // Set directory containing netlib data files.
  std::string netlibDir;
  if (parms.find("-netlibDir") != parms.end())
    netlibDir=parms["-netlibDir"] + dirsep;
  else 
    netlibDir = mpsDir;

  // Set directory containing netlib data files.
  std::string testModel;
  if (parms.find("-testModel") != parms.end())
    testModel=parms["-testModel"] ;
  else 
    testModel = "p0033.mps";

  bool allOK = true ;

  // *FIXME* : these tests should be written... 
  //  testingMessage( "Testing CoinHelperFunctions\n" );
  //  CoinHelperFunctionsUnitTest();
  //  testingMessage( "Testing CoinSort\n" );
  //  tripleCompareUnitTest();

/*
  Check that finite and isnan are working.
*/
  double finiteVal = 1.0 ;
  double zero = 0.0 ;
  double checkVal ;

  testingMessage( "Testing CoinFinite ... " ) ;
# ifdef MY_C_FINITE
  checkVal = finiteVal/zero ;
# else
  checkVal = COIN_DBL_MAX ;
# endif
  testingMessage( " finite value: " ) ;
  if (CoinFinite(finiteVal))
  { testingMessage( "ok" ) ; }
  else
  { allOK = false ;
    testingMessage( "ERROR" ) ; }
  testingMessage( "; infinite value: " ) ;
  if (!CoinFinite(checkVal))
  { testingMessage( "ok.\n" ) ; }
  else
  { allOK = false ;
    testingMessage( "ERROR.\n" ) ; }

# ifdef MY_C_ISNAN
  testingMessage( "Testing CoinIsnan ... " ) ;
  testingMessage( " finite value: " ) ;
  if (!CoinIsnan(finiteVal))
  { testingMessage( "ok" ) ; }
  else
  { allOK = false ;
    testingMessage( "ERROR" ) ; }
  testingMessage( "; NaN value: " ) ;
  checkVal = checkVal/checkVal ;
  if (CoinIsnan(checkVal))
  { testingMessage( "ok.\n" ) ; }
  else
  { allOK = false ;
    testingMessage( "ERROR.\n" ) ; }
# else
  allOK = false ;
  testingMessage( "ERROR: No functional CoinIsnan.\n" ) ;
# endif


  testingMessage( "Testing CoinModel\n" );
  CoinModelUnitTest(mpsDir,netlibDir,testModel);

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

  testingMessage( "Testing CoinMessageHandler\n" );
  if (!CoinMessageHandlerUnitTest())
  { allOK = false ; }

  if (allOK)
  { testingMessage( "All tests completed successfully.\n" );
    return (0) ; }
  else
  { testingMessage( "\nERROR: " ) ;
    testingMessage(
  	"Errors occurred during testing; please check the output.\n\n");
    return (1) ; }
}

 
// Display message on stdout and stderr
void testingMessage( const char * const msg )
{
  std::cerr <<msg;
  //cout <<endl <<"*****************************************"
  //     <<endl <<msg <<endl;
}

