// Copyright (C) 2000, International Business Machines
// Corporation and others.  All Rights Reserved.
#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include <cassert>

#include "CoinMpsIO.hpp"

//#############################################################################

#ifdef NDEBUG
#undef NDEBUG
#endif

//--------------------------------------------------------------------------
// test import methods
void
CoinMpsIOUnitTest(const std::string & mpsDir)
{
  
  // Test default constructor
  {
    CoinMpsIO m;
    assert( m.rowsense_==NULL );
    assert( m.rhs_==NULL );
    assert( m.rowrange_==NULL );
    assert( m.matrixByRow_==NULL );
    assert( m.matrixByColumn_==NULL );
    assert( m.integerType_==NULL);
    assert( !strcmp( m.getFileName() , "????"));
    assert( !strcmp( m.getProblemName() , ""));
    assert( !strcmp( m.objectiveName_ , ""));
    assert( !strcmp( m.rhsName_ , ""));
    assert( !strcmp( m.rangeName_ , ""));
    assert( !strcmp( m.boundName_ , ""));
  }
  
  
  {    
    CoinRelFltEq eq;
    CoinMpsIO m;
    std::string fn = mpsDir+"exmip1";
    int numErr = m.readMps(fn.c_str(),"mps");
    assert( numErr== 0 );

    assert( !strcmp( m.problemName_ , "EXAMPLE"));
    assert( !strcmp( m.objectiveName_ , "OBJ"));
    assert( !strcmp( m.rhsName_ , "RHS1"));
    assert( !strcmp( m.rangeName_ , "RNG1"));
    assert( !strcmp( m.boundName_ , "BND1"));
    
     // Test language and re-use
    m.newLanguage(CoinMessages::it);
    m.messageHandler()->setPrefix(false);

    // This call should return an error indicating that the 
    // end-of-file was reached.
    // This is because the file remains open to possibly read
    // a quad. section.
    numErr = m.readMps(fn.c_str(),"mps");
    assert( numErr < 0 );

    // Test copy constructor and assignment operator
    {
      CoinMpsIO lhs;
      {      
        CoinMpsIO im(m);        
	
        CoinMpsIO imC1(im);
        assert( imC1.getNumCols() == im.getNumCols() );
        assert( imC1.getNumRows() == im.getNumRows() );   
        
        CoinMpsIO imC2(im);
        assert( imC2.getNumCols() == im.getNumCols() );
        assert( imC2.getNumRows() == im.getNumRows() );  
        
        lhs=imC2;
      }
      // Test that lhs has correct values even though rhs has gone out of scope
      
      assert( lhs.getNumCols() == m.getNumCols() );
      assert( lhs.getNumRows() == m.getNumRows() );      
    }
    
    
    {    
      CoinMpsIO dumSi(m);
      int nc = dumSi.getNumCols();
      int nr = dumSi.getNumRows();
      const double * cl = dumSi.getColLower();
      const double * cu = dumSi.getColUpper();
      const double * rl = dumSi.getRowLower();
      const double * ru = dumSi.getRowUpper();
      assert( nc == 8 );
      assert( nr == 5 );
      assert( eq(cl[0],2.5) );
      assert( eq(cl[1],0.0) );
      assert( eq(cu[1],4.1) );
      assert( eq(cu[2],1.0) );
      assert( eq(rl[0],2.5) );
      assert( eq(rl[4],3.0) );
      assert( eq(ru[1],2.1) );
      assert( eq(ru[4],15.0) );
      
      assert( !eq(cl[3],1.2345) );
      
      assert( !eq(cu[4],10.2345) );
      
      assert( eq( dumSi.getObjCoefficients()[0],  1.0) );
      assert( eq( dumSi.getObjCoefficients()[1],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[2],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[3],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[4],  2.0) );
      assert( eq( dumSi.getObjCoefficients()[5],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[6],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[7], -1.0) );

      dumSi.writeMps("CoinMpsIoTest.mps");//,0,0,1);
    }

    // Read just written file
    {    
      CoinMpsIO dumSi;
      dumSi.readMps("CoinMpsIoTest");
      int nc = dumSi.getNumCols();
      int nr = dumSi.getNumRows();
      const double * cl = dumSi.getColLower();
      const double * cu = dumSi.getColUpper();
      const double * rl = dumSi.getRowLower();
      const double * ru = dumSi.getRowUpper();
      assert( nc == 8 );
      assert( nr == 5 );
      assert( eq(cl[0],2.5) );
      assert( eq(cl[1],0.0) );
      assert( eq(cu[1],4.1) );
      assert( eq(cu[2],1.0) );
      assert( eq(rl[0],2.5) );
      assert( eq(rl[4],3.0) );
      assert( eq(ru[1],2.1) );
      assert( eq(ru[4],15.0) );
      
      assert( !eq(cl[3],1.2345) );
      
      assert( !eq(cu[4],10.2345) );
      
      assert( eq( dumSi.getObjCoefficients()[0],  1.0) );
      assert( eq( dumSi.getObjCoefficients()[1],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[2],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[3],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[4],  2.0) );
      assert( eq( dumSi.getObjCoefficients()[5],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[6],  0.0) );
      assert( eq( dumSi.getObjCoefficients()[7], -1.0) );
    }
    
    // Test matrixByRow method
    { 
      const CoinMpsIO si(m);
      const CoinPackedMatrix * smP = si.getMatrixByRow();
      // LL:      const CoinDumPackedMatrix * osmP = dynamic_cast<const CoinDumPackedMatrix*>(smP);
      // LL: assert( osmP!=NULL );
      
      CoinRelFltEq eq;
      const double * ev = smP->getElements();
      assert( eq(ev[0],   3.0) );
      assert( eq(ev[1],   1.0) );
      assert( eq(ev[2],  -2.0) );
      assert( eq(ev[3],  -1.0) );
      assert( eq(ev[4],  -1.0) );
      assert( eq(ev[5],   2.0) );
      assert( eq(ev[6],   1.1) );
      assert( eq(ev[7],   1.0) );
      assert( eq(ev[8],   1.0) );
      assert( eq(ev[9],   2.8) );
      assert( eq(ev[10], -1.2) );
      assert( eq(ev[11],  5.6) );
      assert( eq(ev[12],  1.0) );
      assert( eq(ev[13],  1.9) );
      
      const CoinBigIndex * mi = smP->getVectorStarts();
      assert( mi[0]==0 );
      assert( mi[1]==5 );
      assert( mi[2]==7 );
      assert( mi[3]==9 );
      assert( mi[4]==11 );
      assert( mi[5]==14 );
      
      const int * ei = smP->getIndices();
      assert( ei[0]  ==  0 );
      assert( ei[1]  ==  1 );
      assert( ei[2]  ==  3 );
      assert( ei[3]  ==  4 );
      assert( ei[4]  ==  7 );
      assert( ei[5]  ==  1 );
      assert( ei[6]  ==  2 );
      assert( ei[7]  ==  2 );
      assert( ei[8]  ==  5 );
      assert( ei[9]  ==  3 );
      assert( ei[10] ==  6 );
      assert( ei[11] ==  0 );
      assert( ei[12] ==  4 );
      assert( ei[13] ==  7 );    
      
      assert( smP->getMajorDim() == 5 ); 
      assert( smP->getNumElements() == 14 );
      
    }
        // Test matrixByCol method
    {
      
      const CoinMpsIO si(m);
      const CoinPackedMatrix * smP = si.getMatrixByCol();
      // LL:      const CoinDumPackedMatrix * osmP = dynamic_cast<const CoinDumPackedMatrix*>(smP);
      // LL: assert( osmP!=NULL );
      
      CoinRelFltEq eq;
      const double * ev = smP->getElements();
      assert( eq(ev[0],   3.0) );
      assert( eq(ev[1],   5.6) );
      assert( eq(ev[2],   1.0) );
      assert( eq(ev[3],   2.0) );
      assert( eq(ev[4],   1.1) );
      assert( eq(ev[5],   1.0) );
      assert( eq(ev[6],  -2.0) );
      assert( eq(ev[7],   2.8) );
      assert( eq(ev[8],  -1.0) );
      assert( eq(ev[9],   1.0) );
      assert( eq(ev[10],  1.0) );
      assert( eq(ev[11], -1.2) );
      assert( eq(ev[12], -1.0) );
      assert( eq(ev[13],  1.9) );
      
      const CoinBigIndex * mi = smP->getVectorStarts();
      assert( mi[0]==0 );
      assert( mi[1]==2 );
      assert( mi[2]==4 );
      assert( mi[3]==6 );
      assert( mi[4]==8 );
      assert( mi[5]==10 );
      assert( mi[6]==11 );
      assert( mi[7]==12 );
      assert( mi[8]==14 );
      
      const int * ei = smP->getIndices();
      assert( ei[0]  ==  0 );
      assert( ei[1]  ==  4 );
      assert( ei[2]  ==  0 );
      assert( ei[3]  ==  1 );
      assert( ei[4]  ==  1 );
      assert( ei[5]  ==  2 );
      assert( ei[6]  ==  0 );
      assert( ei[7]  ==  3 );
      assert( ei[8]  ==  0 );
      assert( ei[9]  ==  4 );
      assert( ei[10] ==  2 );
      assert( ei[11] ==  3 );
      assert( ei[12] ==  0 );
      assert( ei[13] ==  4 );    
      
      assert( smP->getMajorDim() == 8 ); 
      assert( smP->getNumElements() == 14 );

      assert( smP->getSizeVectorStarts()==9 );
      assert( smP->getMinorDim() == 5 );
      
    }
    //--------------
    // Test rowsense, rhs, rowrange, matrixByRow
    {
      CoinMpsIO lhs;
      {      
        assert( m.rowrange_==NULL );
        assert( m.rowsense_==NULL );
        assert( m.rhs_==NULL );
        assert( m.matrixByRow_==NULL );
        
        CoinMpsIO siC1(m);     
        assert( siC1.rowrange_==NULL );
        assert( siC1.rowsense_==NULL );
        assert( siC1.rhs_==NULL );
        assert( siC1.matrixByRow_==NULL );

        const char   * siC1rs  = siC1.getRowSense();
        assert( siC1rs[0]=='G' );
        assert( siC1rs[1]=='L' );
        assert( siC1rs[2]=='E' );
        assert( siC1rs[3]=='R' );
        assert( siC1rs[4]=='R' );
        
        const double * siC1rhs = siC1.getRightHandSide();
        assert( eq(siC1rhs[0],2.5) );
        assert( eq(siC1rhs[1],2.1) );
        assert( eq(siC1rhs[2],4.0) );
        assert( eq(siC1rhs[3],5.0) );
        assert( eq(siC1rhs[4],15.) ); 
        
        const double * siC1rr  = siC1.getRowRange();
        assert( eq(siC1rr[0],0.0) );
        assert( eq(siC1rr[1],0.0) );
        assert( eq(siC1rr[2],0.0) );
        assert( eq(siC1rr[3],5.0-1.8) );
        assert( eq(siC1rr[4],15.0-3.0) );
        
        const CoinPackedMatrix * siC1mbr = siC1.getMatrixByRow();
        assert( siC1mbr != NULL );
        
        const double * ev = siC1mbr->getElements();
        assert( eq(ev[0],   3.0) );
        assert( eq(ev[1],   1.0) );
        assert( eq(ev[2],  -2.0) );
        assert( eq(ev[3],  -1.0) );
        assert( eq(ev[4],  -1.0) );
        assert( eq(ev[5],   2.0) );
        assert( eq(ev[6],   1.1) );
        assert( eq(ev[7],   1.0) );
        assert( eq(ev[8],   1.0) );
        assert( eq(ev[9],   2.8) );
        assert( eq(ev[10], -1.2) );
        assert( eq(ev[11],  5.6) );
        assert( eq(ev[12],  1.0) );
        assert( eq(ev[13],  1.9) );
        
        const CoinBigIndex * mi = siC1mbr->getVectorStarts();
        assert( mi[0]==0 );
        assert( mi[1]==5 );
        assert( mi[2]==7 );
        assert( mi[3]==9 );
        assert( mi[4]==11 );
        assert( mi[5]==14 );
        
        const int * ei = siC1mbr->getIndices();
        assert( ei[0]  ==  0 );
        assert( ei[1]  ==  1 );
        assert( ei[2]  ==  3 );
        assert( ei[3]  ==  4 );
        assert( ei[4]  ==  7 );
        assert( ei[5]  ==  1 );
        assert( ei[6]  ==  2 );
        assert( ei[7]  ==  2 );
        assert( ei[8]  ==  5 );
        assert( ei[9]  ==  3 );
        assert( ei[10] ==  6 );
        assert( ei[11] ==  0 );
        assert( ei[12] ==  4 );
        assert( ei[13] ==  7 );    
        
        assert( siC1mbr->getMajorDim() == 5 ); 
        assert( siC1mbr->getNumElements() == 14 );
        

        assert( siC1rs  == siC1.getRowSense() );
        assert( siC1rhs == siC1.getRightHandSide() );
        assert( siC1rr  == siC1.getRowRange() );
      }
    }
  }
  
}





