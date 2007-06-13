// Copyright (C) 2002, International Business Machines
// Corporation and others.  All Rights Reserved.

#if defined(_MSC_VER)
// Turn off compiler warning about long names
#  pragma warning(disable:4786)
#endif

#include "CoinUtilsConfig.h"

#include <cassert>
#include <cfloat>
#include <stdio.h>
#include "CoinFactorization.hpp"
#include "CoinIndexedVector.hpp"
#include "CoinHelperFunctions.hpp"
#if DENSE_CODE==1
// using simple clapack interface
extern "C" int dgetrf_(int *m, int *n, double *a, int *	lda, 
		       int *ipiv, int *info);
#endif
static int counter1=0;
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
  counter1++;
  // when to go dense
  int denseThreshold=denseThreshold_;

  CoinZeroN ( workArea, numberRows_ );
  //get space for bit work area
  CoinBigIndex workSize = 1000;
  unsigned int *workArea2 = ( unsigned int * ) new int [ workSize ];

  int larger;

  if ( numberRows_ < numberColumns_ ) {
    larger = numberColumns_;
  } else {
    larger = numberRows_;
  }
  int status = 0;
  //do slacks first
  if (biasLU_<3&&numberColumns_==numberRows_) {
    int pivotColumn;
    for ( pivotColumn = 0; pivotColumn < numberColumns_;
	  pivotColumn++ ) {
      if ( numberInColumn_[pivotColumn] == 1 ) {
	CoinBigIndex start = startColumnU_[pivotColumn];
	double value = element[start];
	if ( value == slackValue_ && numberInColumnPlus_[pivotColumn] == 0 ) {
	  // treat as slack
	  int iRow = indexRowU_[start];
	  // but only if row not marked
	  if (numberInRow_[iRow]>0) {
	    totalElements_ -= numberInRow_[iRow];
	    //take out this bit of indexColumnU
	    int next = nextRow_[iRow];
	    int last = lastRow_[iRow];
	    
	    nextRow_[last] = next;
	    lastRow_[next] = last;
	    nextRow_[iRow] = numberGoodU_;	//use for permute
	    //modify linked list for pivots
	    deleteLink ( iRow );
	    numberInRow_[iRow]=-1;
	    numberInColumn_[pivotColumn]=0;
	    pivotRowL_[numberGoodL_] = iRow;
	    numberGoodL_++;
	    startColumnL_[numberGoodL_] = 0;
	    pivotColumn_[numberGoodU_] = pivotColumn;
	    numberGoodU_++;
	  }
	}
      }
    }
    // redo
    preProcess(4);
  }
  numberSlacks_ = numberGoodU_;
  int *nextCount = nextCount_;
  int *numberInRow = numberInRow_;
  int *numberInColumn = numberInColumn_;
  CoinBigIndex *startRow = startRowU_;
  CoinBigIndex *startColumn = startColumnU_;
  double pivotTolerance = pivotTolerance_;
  int numberTrials = numberTrials_;
  int numberRows = numberRows_;
  // Put column singletons first - (if false)
  separateLinks(1,(biasLU_>1));
  int counter2=0;
  while ( count <= biggerDimension_ ) {
    counter2++;
    int badRow=-1;
    if (counter1==-1&&counter2>=0) {
      // check counts consistent
      for (int iCount=1;iCount<numberRows_;iCount++) {
        int look = firstCount_[iCount];
        while ( look >= 0 ) {
          if ( look < numberRows_ ) {
            int iRow = look;
            if (iRow==badRow)
              printf("row count for row %d is %d\n",iCount,iRow);
            if ( numberInRow[iRow] != iCount ) {
              printf("failed debug on %d entry to factorSparse and %d try\n",
                     counter1,counter2);
              printf("row %d - count %d number %d\n",iRow,iCount,numberInRow[iRow]);
              abort();
            }
            look = nextCount[look];
          } else {
            int iColumn = look - numberRows;
            if ( numberInColumn[iColumn] != iCount ) {
              printf("failed debug on %d entry to factorSparse and %d try\n",
                     counter1,counter2);
              printf("column %d - count %d number %d\n",iColumn,iCount,numberInColumn[iColumn]);
              abort();
            }
            look = nextCount[look];
          }
        }
      }
    }
    CoinBigIndex minimumCount = COIN_INT_MAX;
    double minimumCost = COIN_DBL_MAX;

    int pivotRow = -1;
    int pivotColumn = -1;
    int pivotRowPosition = -1;
    int pivotColumnPosition = -1;
    int look = firstCount_[count];
    int trials = 0;

    if ( count == 1 && firstCount_[1] >= 0 &&!biasLU_) {
      //do column singletons first to put more in U
      while ( look >= 0 ) {
        if ( look < numberRows_ ) {
          look = nextCount_[look];
        } else {
          int iColumn = look - numberRows_;
          
          assert ( numberInColumn_[iColumn] == count );
          CoinBigIndex start = startColumnU_[iColumn];
          int iRow = indexRow[start];
          
          pivotRow = iRow;
          pivotRowPosition = start;
          pivotColumn = iColumn;
          assert (pivotRow>=0&&pivotColumn>=0);
          pivotColumnPosition = -1;
          look = -1;
          break;
        }
      }			/* endwhile */
      if ( pivotRow < 0 ) {
        //back to singletons
        look = firstCount_[1];
      }
    }
    while ( look >= 0 ) {
      if ( look < numberRows_ ) {
        int iRow = look;
        
        if ( numberInRow[iRow] != count ) {
          printf("failed on %d entry to factorSparse and %d try\n",
                 counter1,counter2);
          printf("row %d - count %d number %d\n",iRow,count,numberInRow[iRow]);
          abort();
        }
        look = nextCount[look];
        bool rejected = false;
        CoinBigIndex start = startRow[iRow];
        CoinBigIndex end = start + count;
        
        CoinBigIndex i;
        for ( i = start; i < end; i++ ) {
          int iColumn = indexColumn[i];
          assert (numberInColumn[iColumn]>0);
          double cost = ( count - 1 ) * numberInColumn[iColumn];
          
          if ( cost < minimumCost ) {
            CoinBigIndex where = startColumn[iColumn];
            double minimumValue = element[where];
            
            minimumValue = fabs ( minimumValue ) * pivotTolerance;
            while ( indexRow[where] != iRow ) {
              where++;
            }			/* endwhile */
            assert ( where < startColumn[iColumn] +
                     numberInColumn[iColumn]);
            double value = element[where];
            
            value = fabs ( value );
            if ( value >= minimumValue ) {
              minimumCost = cost;
              minimumCount = numberInColumn[iColumn];
              pivotRow = iRow;
              pivotRowPosition = -1;
              pivotColumn = iColumn;
              assert (pivotRow>=0&&pivotColumn>=0);
              pivotColumnPosition = i;
              rejected=false;
              if ( minimumCount < count ) {
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
        
        assert ( numberInColumn[iColumn] == count );
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
            int nInRow = numberInRow[iRow];
            assert (nInRow>0);
            double cost = ( count - 1 ) * nInRow;
            
            if ( cost < minimumCost ) {
              minimumCost = cost;
              minimumCount = nInRow;
              pivotRow = iRow;
              pivotRowPosition = i;
              pivotColumn = iColumn;
              assert (pivotRow>=0&&pivotColumn>=0);
              pivotColumnPosition = -1;
              if ( minimumCount <= count + 1 ) {
                look = -1;
                break;
              }
            }
          }
        }
        trials++;
        if ( trials >= numberTrials && pivotRow >= 0 ) {
          look = -1;
          break;
        }
      }
    }				/* endwhile */
    if (pivotRow>=0) {
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
                                    COIN_INT_MAX);
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
            assert (!numberDoRow);
            if ( !pivotRowSingleton ( pivotRow, pivotColumn ) ) {
              status = -99;
              count=biggerDimension_+1;
              break;
            }
          }
        } else {
          assert (!numberDoColumn);
          if ( !pivotColumnSingleton ( pivotRow, pivotColumn ) ) {
            status = -99;
            count=biggerDimension_+1;
            break;
          }
        }
        pivotColumn_[numberGoodU_] = pivotColumn;
        numberGoodU_++;
        // This should not need to be trapped here - but be safe
        if (numberGoodU_==numberRows_) 
          count=biggerDimension_+1;
      }
#if COIN_DEBUG==2
      checkConsistency (  );
#endif
#if 0
      // Even if no dense code may be better to use naive dense
      if (!denseThreshold_&&totalElements_>1000) {
        int leftRows=numberRows_-numberGoodU_;
        double full = leftRows;
        full *= full;
        assert (full>=0.0);
        double leftElements = totalElements_;
        double ratio;
        if (leftRows>2000)
          ratio=4.0;
        else 
          ratio=3.0;
        if (ratio*leftElements>full) 
          denseThreshold=1;
      }
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
        double ratio;
        if (leftRows>2000)
          ratio=4.0;
        else if (leftRows>800)
          ratio=3.0;
        else if (leftRows>256)
          ratio=2.0;
        else
          ratio=1.5;
        if ((ratio*leftElements>full&&leftRows>denseThreshold_)) {
          //return to do dense
          if (status!=0)
            break;
          int check=0;
          for (int iColumn=0;iColumn<numberColumns_;iColumn++) {
            if (numberInColumn_[iColumn]) 
              check++;
          }
          if (check!=leftRows&&denseThreshold_) {
            //printf("** mismatch %d columns left, %d rows\n",check,leftRows);
            denseThreshold=0;
          } else {
            status=2;
            if ((messageLevel_&4)!=0) 
              std::cout<<"      Went dense at "<<leftRows<<" rows "<<
                totalElements_<<" "<<full<<" "<<leftElements<<std::endl;
            if (!denseThreshold_)
              denseThreshold_=-check; // say how many
            break;
          }
        }
      }
      // start at 1 again
      count = 1;
    } else {
      //end of this - onto next
      count++;
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
  numberDense_=numberRows_-numberGoodU_;
  if (sizeof(CoinBigIndex)==4&&numberDense_>=2<<15) {
    abort();
  } 
  CoinBigIndex full;
  if (denseThreshold_>0) 
    full = numberDense_*numberDense_;
  else
    full = - denseThreshold_*numberDense_;
  totalElements_=full;
  denseArea_= new double [full];
  CoinZeroN(denseArea_,full);
  densePermute_= new int [numberDense_];
  //mark row lookup using lastRow
  int i;
  for (i=0;i<numberRows_;i++)
    lastRow_[i]=0;
  int * indexRow = indexRowU_;
  double * element = elementU_;
  for ( i=0;i<numberGoodU_;i++) {
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
  int iColumn=0;
  for (iColumn=0;iColumn<numberColumns_;iColumn++) {
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
#ifdef DENSE_CODE
  if (denseThreshold_>0) {
    assert(numberGoodU_==numberRows_);
    numberGoodL_=numberRows_;
    //now factorize
    //dgef(denseArea_,&numberDense_,&numberDense_,densePermute_);
    int info;
    dgetrf_(&numberDense_,&numberDense_,denseArea_,&numberDense_,densePermute_,
            &info);
    // need to check size of pivots
    if(info)
      status = -1;
    return status;
  } 
#endif
  numberGoodU_ = numberRows_-numberDense_;
  int base = numberGoodU_;
  int iDense;
  int numberToDo=-denseThreshold_;
  denseThreshold_=0;
  double tolerance = zeroTolerance_;
  tolerance = 1.0e-30;
  // make sure we have enough space in L and U
  for (iDense=0;iDense<numberToDo;iDense++) {
    //how much space have we got
    iColumn = pivotColumn_[base+iDense];
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
  // Fill in ?
  for (iColumn=numberGoodU_+numberToDo;iColumn<numberRows_;iColumn++) {
    pivotRowL_[iColumn]=iColumn;
    startColumnL_[iColumn+1]=endL;
    pivotRegion_[iColumn]=1.0;
  } 
  if ( lengthL_ + full*0.5 > lengthAreaL_ ) {
    //need more memory
    if ((messageLevel_&4)!=0) 
      std::cout << "more memory needed in middle of invert" << std::endl;
    return -99;
  }
  for (iDense=0;iDense<numberToDo;iDense++) {
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
      iColumn = pivotColumn_[base+iDense];
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
      for (jDense=iDense+1;jDense<numberToDo;jDense++) {
	double value = element2[iDense];
	for (iRow=iDense+1;iRow<numberDense_;iRow++) {
	  //double oldValue=element2[iRow];
	  element2[iRow] -= value*element[iRow];
	  //if (oldValue&&!element2[iRow]) {
          //printf("Updated element for column %d, row %d old %g",
          //   pivotColumn_[base+jDense],densePermute_[iRow],oldValue);
          //printf(" new %g\n",element2[iRow]);
	  //}
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
  return status;
}
// Separate out links with same row/column count
void 
CoinFactorization::separateLinks(int count,bool rowsFirst)
{
  int next = firstCount_[count];
  int firstRow=-1;
  int firstColumn=-1;
  int lastRow=-1;
  int lastColumn=-1;
  while(next>=0) {
    int next2=nextCount_[next];
    if (next>=numberRows_) {
      nextCount_[next]=-1;
      // Column
      if (firstColumn>=0) {
	lastCount_[next]=lastColumn;
	nextCount_[lastColumn]=next;
      } else {
	lastCount_[next]= -2 - count;
	firstColumn=next;
      }
      lastColumn=next;
    } else {
      // Row
      if (firstRow>=0) {
	lastCount_[next]=lastRow;
	nextCount_[lastRow]=next;
      } else {
	lastCount_[next]= -2 - count;
	firstRow=next;
      }
      lastRow=next;
    }
    next=next2;
  }
  if (rowsFirst&&firstRow>=0) {
    firstCount_[count]=firstRow;
    nextCount_[lastRow]=firstColumn;
    if (firstColumn>=0)
      lastCount_[firstColumn]=lastRow;
  } else if (firstRow<0) {
    firstCount_[count]=firstColumn;
  } else if (firstColumn>=0) {
    firstCount_[count]=firstColumn;
    nextCount_[lastColumn]=firstRow;
    if (firstRow>=0)
      lastCount_[firstRow]=lastColumn;
  } 
}
// Debug - save on file
int
CoinFactorization::saveFactorization (const char * file  ) const
{
  FILE * fp = fopen(file,"wb");
  if (fp) {
    // Save so we can pick up scalars
    const char * first = (const char *) &pivotTolerance_;
    const char * last = (const char *) &biasLU_;
    // increment
    last += sizeof(int);
    if (fwrite(first,last-first,1,fp)!=1)
      return 1;
    int extraSpace = maximumColumnsExtra_ - numberColumns_;
    // Now arrays
    if (CoinToFile(elementU_,lengthAreaU_ , fp ))
      return 1;
    if (CoinToFile(indexRowU_,lengthAreaU_ , fp ))
      return 1;
    if (CoinToFile(indexColumnU_,lengthAreaU_ , fp ))
      return 1;
    if (CoinToFile(convertRowToColumnU_,lengthAreaU_ , fp ))
      return 1;
    if (CoinToFile(elementByRowL_,lengthAreaL_ , fp ))
      return 1;
    if (CoinToFile(indexColumnL_,lengthAreaL_ , fp ))
      return 1;
    if (CoinToFile(startRowL_ , numberRows_+1, fp ))
      return 1;
    if (CoinToFile(elementL_,lengthAreaL_ , fp ))
      return 1;
    if (CoinToFile(indexRowL_,lengthAreaL_ , fp ))
      return 1;
    if (CoinToFile(startColumnL_,numberRows_ + 1 , fp ))
      return 1;
    if (CoinToFile(markRow_,numberRows_  , fp))
      return 1;
    if (CoinToFile(saveColumn_,numberColumns_  , fp))
      return 1;
    if (CoinToFile(startColumnR_ , extraSpace + 1 , fp ))
      return 1;
    if (CoinToFile(startRowU_,maximumRowsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(numberInRow_,maximumRowsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(nextRow_,maximumRowsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(lastRow_,maximumRowsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(pivotRegion_,maximumRowsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(permuteBack_,maximumRowsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(permute_,maximumRowsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(pivotColumnBack_,maximumRowsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(startColumnU_,maximumColumnsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(numberInColumn_,maximumColumnsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(numberInColumnPlus_,maximumColumnsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(firstCount_,biggerDimension_ + 2 , fp ))
      return 1;
    if (CoinToFile(nextCount_,numberRows_ + numberColumns_ , fp ))
      return 1;
    if (CoinToFile(lastCount_,numberRows_ + numberColumns_ , fp ))
      return 1;
    if (CoinToFile(pivotRowL_,numberRows_ + 1 , fp ))
      return 1;
    if (CoinToFile(pivotColumn_,maximumColumnsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(nextColumn_,maximumColumnsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(lastColumn_,maximumColumnsExtra_ + 1 , fp ))
      return 1;
    if (CoinToFile(denseArea_ , numberDense_*numberDense_, fp ))
      return 1;
    if (CoinToFile(densePermute_ , numberDense_, fp ))
      return 1;
    fclose(fp);
  }
  return 0;
}
// Debug - restore from file
int 
CoinFactorization::restoreFactorization (const char * file , bool factorIt ) 
{
  FILE * fp = fopen(file,"rb");
  if (fp) {
    // Get rid of current
    gutsOfDestructor();
    int newSize=0; // for checking - should be same
    // Restore so we can pick up scalars
    char * first = (char *) &pivotTolerance_;
    char * last = (char *) &biasLU_;
    // increment
    last += sizeof(int);
    if (fread(first,last-first,1,fp)!=1)
      return 1;
    int extraSpace = maximumColumnsExtra_ - numberColumns_;
    CoinBigIndex space = lengthAreaL_ - lengthL_;
    // Now arrays
    if (CoinFromFile(elementU_,lengthAreaU_ , fp, newSize )==1)
      return 1;
    assert (newSize==lengthAreaU_);
    if (CoinFromFile(indexRowU_,lengthAreaU_ , fp, newSize )==1)
      return 1;
    assert (newSize==lengthAreaU_);
    if (CoinFromFile(indexColumnU_,lengthAreaU_ , fp, newSize )==1)
      return 1;
    assert (newSize==lengthAreaU_);
    if (CoinFromFile(convertRowToColumnU_,lengthAreaU_ , fp, newSize )==1)
      return 1;
    assert (newSize==lengthAreaU_||(newSize==0&&!convertRowToColumnU_));
    if (CoinFromFile(elementByRowL_,lengthAreaL_ , fp, newSize )==1)
      return 1;
    assert (newSize==lengthAreaL_||(newSize==0&&!elementByRowL_));
    if (CoinFromFile(indexColumnL_,lengthAreaL_ , fp, newSize )==1)
      return 1;
    assert (newSize==lengthAreaL_||(newSize==0&&!indexColumnL_));
    if (CoinFromFile(startRowL_ , numberRows_+1, fp, newSize )==1)
      return 1;
    assert (newSize==numberRows_+1||(newSize==0&&!startRowL_));
    if (CoinFromFile(elementL_,lengthAreaL_ , fp, newSize )==1)
      return 1;
    assert (newSize==lengthAreaL_);
    if (CoinFromFile(indexRowL_,lengthAreaL_ , fp, newSize )==1)
      return 1;
    assert (newSize==lengthAreaL_);
    if (CoinFromFile(startColumnL_,numberRows_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==numberRows_+1);
    if (CoinFromFile(markRow_,numberRows_  , fp, newSize )==1)
      return 1;
    assert (newSize==numberRows_);
    if (CoinFromFile(saveColumn_,numberColumns_  , fp, newSize )==1)
      return 1;
    assert (newSize==numberColumns_);
    if (CoinFromFile(startColumnR_ , extraSpace + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==extraSpace+1||(newSize==0&&!startColumnR_));
    if (CoinFromFile(startRowU_,maximumRowsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumRowsExtra_+1||(newSize==0&&!startRowU_));
    if (CoinFromFile(numberInRow_,maximumRowsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumRowsExtra_+1);
    if (CoinFromFile(nextRow_,maximumRowsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumRowsExtra_+1);
    if (CoinFromFile(lastRow_,maximumRowsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumRowsExtra_+1);
    if (CoinFromFile(pivotRegion_,maximumRowsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumRowsExtra_+1);
    if (CoinFromFile(permuteBack_,maximumRowsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumRowsExtra_+1||(newSize==0&&!permuteBack_));
    if (CoinFromFile(permute_,maximumRowsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumRowsExtra_+1||(newSize==0&&!permute_));
    if (CoinFromFile(pivotColumnBack_,maximumRowsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumRowsExtra_+1||(newSize==0&&!pivotColumnBack_));
    if (CoinFromFile(startColumnU_,maximumColumnsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumColumnsExtra_+1);
    if (CoinFromFile(numberInColumn_,maximumColumnsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumColumnsExtra_+1);
    if (CoinFromFile(numberInColumnPlus_,maximumColumnsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumColumnsExtra_+1);
    if (CoinFromFile(firstCount_,biggerDimension_ + 2 , fp, newSize )==1)
      return 1;
    assert (newSize==biggerDimension_+2);
    if (CoinFromFile(nextCount_,numberRows_ + numberColumns_ , fp, newSize )==1)
      return 1;
    assert (newSize==numberRows_+numberColumns_);
    if (CoinFromFile(lastCount_,numberRows_ + numberColumns_ , fp, newSize )==1)
      return 1;
    assert (newSize==numberRows_+numberColumns_);
    if (CoinFromFile(pivotRowL_,numberRows_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==numberRows_+1);
    if (CoinFromFile(pivotColumn_,maximumColumnsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumColumnsExtra_+1);
    if (CoinFromFile(nextColumn_,maximumColumnsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumColumnsExtra_+1);
    if (CoinFromFile(lastColumn_,maximumColumnsExtra_ + 1 , fp, newSize )==1)
      return 1;
    assert (newSize==maximumColumnsExtra_+1);
    if (CoinFromFile(denseArea_ , numberDense_*numberDense_, fp, newSize )==1)
      return 1;
    assert (newSize==numberDense_*numberDense_);
    if (CoinFromFile(densePermute_ , numberDense_, fp, newSize )==1)
      return 1;
    assert (newSize==numberDense_);
    lengthAreaR_ = space;
    elementR_ = elementL_ + lengthL_;
    indexRowR_ = indexRowL_ + lengthL_;
    fclose(fp);
    if (factorIt) {
      if (biasLU_>=3||numberRows_!=numberColumns_)
        preProcess ( 2 );
      else
        preProcess ( 3 ); // no row copy
      factor (  );
    }
  }
  return 0;
}
