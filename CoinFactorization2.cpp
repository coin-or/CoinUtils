// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinFactorization.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#if DENSE_CODE==1
// using simple clapack interface
extern "C" int dgetrf_(int *m, int *n, double *a, int *	lda, 
		       int *ipiv, int *info);
#endif
//  factorSparse.  Does sparse phase of factorization
//return code is <0 error, 0= finished
int
CoinFactorization::factorSparse (  )
{
  int *indexRow = indexRowU_;
  int *indexColumn = indexColumnU_;
  double *element = elementU_;
  int count = 1;
  double *workArea = new double [ numberRows_ ];

  // when to go dense
  int denseThreshold=denseThreshold_;

  CoinFillN ( workArea, numberRows_ , 0.0);
  //get space for bit work area
  CoinBigIndex workSize = 1000;
  unsigned int *workArea2 = ( unsigned int * ) new int [ workSize ];
  int lastColumnInBlock;

  lastColumnInBlock = numberColumns_;
  int larger;

  if ( numberRows_ < numberColumns_ ) {
    larger = numberColumns_;
  } else {
    larger = numberRows_;
  }
  int status = 0;
  //do slacks first
  int pivotColumn;
  for ( pivotColumn = 0; pivotColumn < lastColumnInBlock;
	pivotColumn++ ) {
    if ( numberInColumn_[pivotColumn] == 1 ) {
      CoinBigIndex start = startColumnU_[pivotColumn];
      double value = element[start];

      if ( value == slackValue_ && numberInColumnPlus_[pivotColumn] == 0 ) {
	//treat as slack
	int iRow = indexRowU_[start];


	totalElements_ -= numberInRow_[iRow];
	if ( !pivotColumnSingleton ( iRow, pivotColumn ) ) {
	  status = -99;
	  count=biggerDimension_+1;
	  break;
	}
	pivotColumn_[numberGoodU_] = pivotColumn;
	numberGoodU_++;
      }
    }
  }
  numberSlacks_ = numberGoodU_;
  while ( count <= biggerDimension_ ) {
    CoinBigIndex minimumCount = INT_MAX;
    CoinBigIndex minimumCost = INT_MAX;

    count = 1;
    bool stopping = false;
    int pivotRow = -1;
    int pivotColumn = -1;
    int pivotRowPosition = -1;
    int pivotColumnPosition = -1;
    int look = firstCount_[count];
    int trials = 0;

    while ( !stopping ) {
      if ( count == 1 && firstCount_[1] >= 0 ) {
	//do column singletons first to put more in U
	while ( look >= 0 ) {
	  if ( look < numberRows_ ) {
	    look = nextCount_[look];
	  } else {
	    int iColumn = look - numberRows_;

#if COIN_DEBUG
	    if ( numberInColumn_[iColumn] != count ) {
	      abort (  );
	    }
#endif
	    CoinBigIndex start = startColumnU_[iColumn];
	    int iRow = indexRow[start];

	    pivotRow = iRow;
	    pivotRowPosition = start;
	    pivotColumn = iColumn;
	    pivotColumnPosition = -1;
	    stopping = true;
	    look = -1;
	    break;
	  }
	}			/* endwhile */
	if ( !stopping ) {
	  //back to singletons
	  look = firstCount_[1];
	}
      }
      int *nextCount = nextCount_;
      int *numberInRow = numberInRow_;
      int *numberInColumn = numberInColumn_;
      CoinBigIndex *startRow = startRowU_;
      CoinBigIndex *startColumn = startColumnU_;
      double pivotTolerance = pivotTolerance_;
      int numberTrials = numberTrials_;
      int numberRows = numberRows_;

      while ( look >= 0 ) {
	if ( look < numberRows_ ) {
	  int iRow = look;

#if COIN_DEBUG
	  if ( numberInRow[iRow] != count ) {
	    abort (  );
	  }
#endif
	  look = nextCount[look];
	  bool rejected = false;
	  CoinBigIndex start = startRow[iRow];
	  CoinBigIndex end = start + count;

	  CoinBigIndex i;
	  for ( i = start; i < end; i++ ) {
	    int iColumn = indexColumn[i];
	    CoinBigIndex cost = ( count - 1 ) * numberInColumn[iColumn];

	    if ( cost < minimumCost ) {
	      CoinBigIndex where = startColumn[iColumn];
	      double minimumValue = element[where];

	      minimumValue = fabs ( minimumValue ) * pivotTolerance;
	      while ( indexRow[where] != iRow ) {
		where++;
	      }			/* endwhile */
#if COIN_DEBUG
	      {
		CoinBigIndex end_debug = startColumn[iColumn] +
		  numberInColumn[iColumn];

		if ( where >= end_debug ) {
		  abort (  );
		}
	      }
#endif
	      double value = element[where];

	      value = fabs ( value );
	      if ( value >= minimumValue ) {
		minimumCost = cost;
		minimumCount = numberInColumn[iColumn];
		pivotRow = iRow;
		pivotRowPosition = -1;
		pivotColumn = iColumn;
		pivotColumnPosition = i;
		if ( minimumCount < count ) {
		  stopping = true;
		  look = -1;
		  break;
		}
	      } else if ( pivotRow == -1 ) {
		rejected = true;
	      }
	    }
	  }
	  trials++;
	  if ( trials >= numberTrials && pivotRow >= 0 ) {
	    stopping = true;
	    look = -1;
	    break;
	  }
	  if ( rejected ) {
	    //take out for moment
	    //eligible when row changes
	    deleteLink ( iRow );
	    addLink ( iRow, biggerDimension_ + 1 );
	  }
	} else {
	  int iColumn = look - numberRows;

#if COIN_DEBUG
	  if ( numberInColumn[iColumn] != count ) {
	    abort (  );
	  }
#endif
	  look = nextCount[look];
	  CoinBigIndex start = startColumn[iColumn];
	  CoinBigIndex end = start + numberInColumn[iColumn];
	  double minimumValue = element[start];

	  minimumValue = fabs ( minimumValue ) * pivotTolerance;
	  CoinBigIndex i;
	  for ( i = start; i < end; i++ ) {
	    double value = element[i];

	    value = fabs ( value );
	    if ( value >= minimumValue ) {
	      int iRow = indexRow[i];
	      CoinBigIndex cost = ( count - 1 ) * numberInRow[iRow];

	      if ( cost < minimumCost ) {
		minimumCost = cost;
		minimumCount = numberInRow[iRow];
		pivotRow = iRow;
		pivotRowPosition = i;
		pivotColumn = iColumn;
		pivotColumnPosition = -1;
		if ( minimumCount <= count + 1 ) {
		  stopping = true;
		  look = -1;
		  break;
		}
	      }
	    }
	  }
	  trials++;
	  if ( trials >= numberTrials && pivotRow >= 0 ) {
	    stopping = true;
	    look = -1;
	    break;
	  }
	}
      }				/* endwhile */
      //end of this - onto next
      if ( !stopping ) {
	count++;
	if ( count <= biggerDimension_ ) {
	  look = firstCount_[count];
	} else {
	  stopping = true;
	}
      } else {
	if ( pivotRow >= 0 ) {
	  int numberDoRow = numberInRow_[pivotRow] - 1;
	  int numberDoColumn = numberInColumn_[pivotColumn] - 1;

	  totalElements_ -= ( numberDoRow + numberDoColumn + 1 );
	  if ( numberDoColumn > 0 ) {
	    if ( numberDoRow > 0 ) {
	      if ( numberDoColumn > 1 ) {
		//  if (1) {
		//need to adjust more for cache and SMP
		//allow at least 4 extra
		int increment = numberDoColumn + 1 + 4;

		if ( increment & 15 ) {
		  increment = increment & ( ~15 );
		  increment += 16;
		}
		int increment2 =

		  ( increment + COINFACTORIZATION_BITS_PER_INT - 1 ) >> COINFACTORIZATION_SHIFT_PER_INT;
		CoinBigIndex size = increment2 * numberDoRow;

		if ( size > workSize ) {
		  workSize = size;
		  delete []  workArea2 ;
		  workArea2 = ( unsigned int * ) new int [ workSize ];
		}
		bool goodPivot;

		if ( larger < 32766 ) {
		  //branch out to best pivot routine 
		  goodPivot = pivot ( pivotRow, pivotColumn,
				      pivotRowPosition, pivotColumnPosition,
				      workArea, workArea2, increment,
				      increment2, ( short * ) markRow_ ,
				      32767);
		} else {
		  //might be able to do better by permuting
		  goodPivot = pivot ( pivotRow, pivotColumn,
				      pivotRowPosition, pivotColumnPosition,
				      workArea, workArea2, increment,
				      increment2, ( int * ) markRow_ ,
				      INT_MAX);
		}
		if ( !goodPivot ) {
		  status = -99;
		  count=biggerDimension_+1;
		  break;
		}
	      } else {
		if ( !pivotOneOtherRow ( pivotRow, pivotColumn ) ) {
		  status = -99;
		  count=biggerDimension_+1;
		  break;
		}
	      }
	    } else {
	      if ( !pivotRowSingleton ( pivotRow, pivotColumn ) ) {
		status = -99;
		count=biggerDimension_+1;
		break;
	      }
	    }
	  } else {
	    if ( !pivotColumnSingleton ( pivotRow, pivotColumn ) ) {
	      status = -99;
	      count=biggerDimension_+1;
	      break;
	    }
	  }
	  pivotColumn_[numberGoodU_] = pivotColumn;
	  numberGoodU_++;
	}
      }
    }				/* endwhile */
#if COIN_DEBUG==2
    checkConsistency (  );
#endif
    if (denseThreshold) {
      // see whether to go dense 
      int leftRows=numberRows_-numberGoodU_;
      double full = leftRows;
      full *= full;
      assert (full>=0.0);
      double leftElements = totalElements_;
      //if (leftRows==100)
      //printf("at 100 %d elements\n",totalElements_);
      if (2.0*leftElements>full&&leftRows>denseThreshold_) {
	//return to do dense
	if (status!=0)
	  break;
#ifdef DENSE_CODE
	int check=0;
	for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
	  if (numberInColumn_[iColumn]) 
	    check++;
	}
	if (check!=leftRows) {
	  printf("** mismatch %d columns left, %d rows\n",check,leftRows);
	  denseThreshold=0;
	} else {
	  status=2;
	  printf("** Went dense at %d rows %d %g %g\n",leftRows,
		 totalElements_,full,leftElements);
	  break;
	}
#endif
      }
    }
  }				/* endwhile */
  delete []  workArea ;
  delete [] workArea2 ;
  return status;
}
//:method factorDense.  Does dense phase of factorization
//return code is <0 error, 0= finished
int CoinFactorization::factorDense()
{
  int status=0;
#ifdef DENSE_CODE
  numberDense_=numberRows_-numberGoodU_;
  if (sizeof(CoinBigIndex)==4&&numberDense_>=2<<15) {
    abort();
  } 
  CoinBigIndex full = numberDense_*numberDense_;
  totalElements_=full;
  denseArea_= new double [full];
  memset(denseArea_,0,full*sizeof(double));
  densePermute_= new int [numberDense_];
  //mark row lookup using lastRow
  int i;
  for (i=0;i<numberRows_;i++)
    lastRow_[i]=0;
  int * indexRow = indexRowU_;
  double * element = elementU_;
  for (int i=0;i<numberGoodU_;i++) {
    int iRow=pivotRowL_[i];
    lastRow_[iRow]=-1;
  } 
  int which=0;
  for (i=0;i<numberRows_;i++) {
    if (!lastRow_[i]) {
      lastRow_[i]=which;
      nextRow_[i]=numberGoodU_+which;
      densePermute_[which]=i;
      which++;
    }
  } 
  //for L part
  CoinBigIndex endL=startColumnL_[numberGoodL_];
  //take out of U
  double * column = denseArea_;
  int rowsDone=0;
  //#define NEWSTYLE
  for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
    if (numberInColumn_[iColumn]) {
      //move
      CoinBigIndex start = startColumnU_[iColumn];
      int number = numberInColumn_[iColumn];
      CoinBigIndex end = start+number;
      for (CoinBigIndex i=start;i<end;i++) {
        int iRow=indexRow[i];
        iRow=lastRow_[iRow];
        column[iRow]=element[i];
      } /* endfor */
      column+=numberDense_;
      while (lastRow_[rowsDone]<0) {
        rowsDone++;
      } /* endwhile */
      pivotRowL_[numberGoodU_]=rowsDone;
      startColumnL_[numberGoodU_+1]=endL;
      numberInColumn_[iColumn]=0;
      pivotColumn_[numberGoodU_]=iColumn;
      pivotRegion_[numberGoodU_]=1.0;
      numberGoodU_++;
    } 
  } 
  assert(numberGoodU_==numberRows_);
#ifndef NEWSTYLE
  numberGoodL_=numberRows_;
  //now factorize
  //dgef(denseArea_,&numberDense_,&numberDense_,densePermute_);
  int info;
  dgetrf_(&numberDense_,&numberDense_,denseArea_,&numberDense_,densePermute_,
	 &info);
  // need to check size of pivots
  if(info)
    status = -1;
#else
  numberGoodU_ = numberRows_-numberDense_;
  int base = numberGoodU_;
  int iDense;
  double tolerance = zeroTolerance_;
  tolerance = 1.0e-30;
  // make sure we have enough space in L and U
  for (iDense=0;iDense<numberDense_;iDense++) {
    //how much space have we got
    int iColumn = pivotColumn_[base+iDense];
    int next = nextColumn_[iColumn];
    int numberInPivotColumn = iDense;
    CoinBigIndex space = startColumnU_[next] 
      - startColumnU_[iColumn]
      - numberInColumnPlus_[next];
    //assume no zero elements
    if ( numberInPivotColumn > space ) {
      //getColumnSpace also moves fixed part
      if ( !getColumnSpace ( iColumn, numberInPivotColumn ) ) {
	return -99;
      }
    }
    // set so further moves will work
    numberInColumn_[iColumn]=numberInPivotColumn;
  }
  if ( lengthL_ + full*0.5 > lengthAreaL_ ) {
    //need more memory
    std::cout << "more memory needed in middle of invert" << std::endl;
    return -99;
  }
  for (iDense=0;iDense<numberDense_;iDense++) {
    int iRow;
    int jDense;
    int pivotRow=-1;
    double * element = denseArea_+iDense*numberDense_;
    double largest = 1.0e-12;
    for (iRow=iDense;iRow<numberDense_;iRow++) {
      if (fabs(element[iRow])>largest) {
	largest = fabs(element[iRow]);
	pivotRow = iRow;
      }
    }
    if ( pivotRow >= 0 ) {
      int iColumn = pivotColumn_[base+iDense];
      double pivotElement=element[pivotRow];
      // get original row
      int originalRow = densePermute_[pivotRow];
      // do nextRow
      nextRow_[originalRow] = numberGoodU_;
      // swap
      densePermute_[pivotRow]=densePermute_[iDense];
      densePermute_[iDense] = originalRow;
      for (jDense=iDense;jDense<numberDense_;jDense++) {
	double value = element[iDense];
	element[iDense] = element[pivotRow];
	element[pivotRow] = value;
	element += numberDense_;
      }
      double pivotMultiplier = 1.0 / pivotElement;
      //printf("pivotMultiplier %g\n",pivotMultiplier);
      pivotRegion_[numberGoodU_] = pivotMultiplier;
      // Do L
      element = denseArea_+iDense*numberDense_;
      CoinBigIndex l = lengthL_;
      startColumnL_[numberGoodL_] = l;	//for luck and first time
      for (iRow=iDense+1;iRow<numberDense_;iRow++) {
	double value = element[iRow]*pivotMultiplier;
	element[iRow] = value;
	if (fabs(value)>tolerance) {
	  indexRowL_[l] = densePermute_[iRow];
	  elementL_[l++] = value;
	}
      }
      pivotRowL_[numberGoodL_++] = originalRow;
      lengthL_ = l;
      startColumnL_[numberGoodL_] = l;
      // update U column
      CoinBigIndex start = startColumnU_[iColumn];
      for (iRow=0;iRow<iDense;iRow++) {
	if (fabs(element[iRow])>tolerance) {
	  indexRowU_[start] = densePermute_[iRow];
	  elementU_[start++] = element[iRow];
	}
      }
      numberInColumn_[iColumn]=0;
      numberInColumnPlus_[iColumn] += start-startColumnU_[iColumn];
      startColumnU_[iColumn]=start;
      // update other columns
      double * element2 = element+numberDense_;
      for (jDense=iDense+1;jDense<numberDense_;jDense++) {
	double value = element2[iDense];
	for (iRow=iDense+1;iRow<numberDense_;iRow++) {
	  double oldValue=element2[iRow];
	  element2[iRow] -= value*element[iRow];
	  if (oldValue&&!element2[iRow]) {
	    printf("Updated element for column %d, row %d old %g",
		   pivotColumn_[base+jDense],densePermute_[iRow],oldValue);
	    printf(" new %g\n",element2[iRow]);
	  }
	}
	element2 += numberDense_;
      }
      numberGoodU_++;
    } else {
      return -1;
    }
  }
  // free area (could use L?)
  delete [] denseArea_;
  denseArea_ = NULL;
  // check if can use another array for densePermute_
  delete [] densePermute_;
  densePermute_ = NULL;
  numberDense_=0;
#endif
#endif
  return status;
}
