// Copyright (C) 2008, International Business Machines
// Corporation and others.  All Rights Reserved.


#include "CoinUtilsConfig.h"
#include "CoinHelperFunctions.hpp"
#include "CoinStructuredModel.hpp"
#include "CoinSort.hpp"
#include "CoinMpsIO.hpp"
#include "CoinFloatEqual.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinStructuredModel::CoinStructuredModel () 
  :  
  numberRows_(0),
  numberRowBlocks_(0),
  numberColumns_(0),
  numberColumnBlocks_(0),
  numberElementBlocks_(0),
  maximumElementBlocks_(0),
  optimizationDirection_(1.0),
  objectiveOffset_(0.0),
  blocks_(NULL),
  blockType_(NULL),
  logLevel_(0)
{
  problemName_ = "";
}
/* Read a problem in MPS or GAMS format from the given filename.
 */
CoinStructuredModel::CoinStructuredModel(const char *fileName, 
					 int decomposeType)
 :
  numberRows_(0),
  numberRowBlocks_(0),
  numberColumns_(0),
  numberColumnBlocks_(0),
  numberElementBlocks_(0),
  maximumElementBlocks_(0),
  optimizationDirection_(1.0),
  objectiveOffset_(0.0),
  blocks_(NULL),
  blockType_(NULL),
  logLevel_(0)
{
  CoinModel coinModel(fileName,false);
  if (coinModel.numberRows()) {
    problemName_ = coinModel.getProblemName();
    optimizationDirection_ = coinModel.optimizationDirection();
    objectiveOffset_ = coinModel.objectiveOffset();
    if (!decomposeType) {
      addBlock("master_row","master_column",coinModel);
    } else {
      const CoinPackedMatrix * matrix = coinModel.packedMatrix();
      if (!matrix) 
	coinModel.convertMatrix();
      if (!decompose(coinModel,decomposeType)) {
	addBlock("master_row","master_column",coinModel);
      } 
    }
  }
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinStructuredModel::CoinStructuredModel (const CoinStructuredModel & rhs) 
  : numberRows_(rhs.numberRows_),
    numberRowBlocks_(rhs.numberRowBlocks_),
    numberColumns_(rhs.numberColumns_),
    numberColumnBlocks_(rhs.numberColumnBlocks_),
    numberElementBlocks_(rhs.numberElementBlocks_),
    maximumElementBlocks_(rhs.maximumElementBlocks_),
    optimizationDirection_(rhs.optimizationDirection_),
    objectiveOffset_(rhs.objectiveOffset_),
    logLevel_(rhs.logLevel_)
{
  problemName_ = rhs.problemName_;
  rowBlockName_ = rhs.rowBlockName_;
  columnBlockName_ = rhs.columnBlockName_;
  if (maximumElementBlocks_) {
    blocks_ = CoinCopyOfArray(rhs.blocks_,maximumElementBlocks_);
    for (int i=0;i<numberElementBlocks_;i++)
      blocks_[i]= new CoinModel(*rhs.blocks_[i]);
    blockType_ = CoinCopyOfArray(rhs.blockType_,maximumElementBlocks_);
  } else {
    blocks_ = NULL;
    blockType_ = NULL;
  }
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinStructuredModel::~CoinStructuredModel ()
{
  for (int i=0;i<numberElementBlocks_;i++)
    delete blocks_[i];
  delete [] blocks_;
  delete [] blockType_;
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinStructuredModel &
CoinStructuredModel::operator=(const CoinStructuredModel& rhs)
{
  if (this != &rhs) {
    for (int i=0;i<numberElementBlocks_;i++)
      delete blocks_[i];
    delete [] blocks_;
    delete [] blockType_;
    problemName_ = rhs.problemName_;
    rowBlockName_ = rhs.rowBlockName_;
    columnBlockName_ = rhs.columnBlockName_;
    numberRows_ = rhs.numberRows_;
    numberRowBlocks_ = rhs.numberRowBlocks_;
    numberColumns_ = rhs.numberColumns_;
    numberColumnBlocks_ = rhs.numberColumnBlocks_;
    numberElementBlocks_ = rhs.numberElementBlocks_;
    maximumElementBlocks_ = rhs.maximumElementBlocks_;
    optimizationDirection_ = rhs.optimizationDirection_;
    objectiveOffset_ = rhs.objectiveOffset_;
    logLevel_ = rhs.logLevel_;
    if (maximumElementBlocks_) {
      blocks_ = CoinCopyOfArray(rhs.blocks_,maximumElementBlocks_);
      for (int i=0;i<numberElementBlocks_;i++)
	blocks_[i]= new CoinModel(*rhs.blocks_[i]);
      blockType_ = CoinCopyOfArray(rhs.blockType_,maximumElementBlocks_);
    } else {
      blocks_ = NULL;
      blockType_ = NULL;
    }
  }
  return *this;
}
static bool sameValues(const double * a, const double *b, int n)
{
  int i;
  for ( i=0;i<n;i++) {
    if (a[i]!=b[i])
      break;
  }
  return (i==n);
}
static bool sameValues(const int * a, const int *b, int n)
{
  int i;
  for ( i=0;i<n;i++) {
    if (a[i]!=b[i])
      break;
  }
  return (i==n);
}
static bool sameValues(const CoinModel * a, const CoinModel *b, bool doRows)
{
  int i=0;
  int n=0;
  if (doRows) {
    n = a->numberRows();
    for ( i=0;i<n;i++) {
      const char * aName = a->getRowName(i);
      const char * bName = b->getRowName(i);
      bool good=true;
      if (aName) {
	if (!bName||strcmp(aName,bName))
	  good=false;
      } else if (bName) {
	good=false;
      }
      if (!good)
	break;
    }
  } else {
    n = a->numberColumns();
    for ( i=0;i<n;i++) {
      const char * aName = a->getColumnName(i);
      const char * bName = b->getColumnName(i);
      bool good=true;
      if (aName) {
	if (!bName||strcmp(aName,bName))
	  good=false;
      } else if (bName) {
	good=false;
      }
      if (!good)
	break;
    }
  }
  return (i==n);
}
// Add a row block name and number of rows
int
CoinStructuredModel::addRowBlock(int numberRows,const std::string &name) 
{
  int iRowBlock;
  for (iRowBlock=0;iRowBlock<numberRowBlocks_;iRowBlock++) {
    if (name==rowBlockName_[iRowBlock])
      break;
  }
  if (iRowBlock==numberRowBlocks_) {
    rowBlockName_.push_back(name);
    numberRowBlocks_++;
    numberRows_ += numberRows;
  }
  return iRowBlock;
}
// Add a column block name and number of columns
int
CoinStructuredModel::addColumnBlock(int numberColumns,const std::string &name) 
{
  int iColumnBlock;
  for (iColumnBlock=0;iColumnBlock<numberColumnBlocks_;iColumnBlock++) {
    if (name==columnBlockName_[iColumnBlock])
      break;
  }
  if (iColumnBlock==numberColumnBlocks_) {
    columnBlockName_.push_back(name);
    numberColumnBlocks_++;
    numberColumns_ += numberColumns;
  }
  return iColumnBlock;
}
/* Fill in info structure and update counts
   Returns number of inconsistencies on border
*/
int 
CoinStructuredModel::fillInfo(CoinModelBlockInfo & info,
			      const CoinModel * block)
{
  int whatsSet = block->whatIsSet();
  info.matrix = ((whatsSet&1)!=0) ? 1 : 0;
  info.rhs = ((whatsSet&2)!=0) ? 1 : 0;
  info.rowName = ((whatsSet&4)!=0) ? 1 : 0;
  info.integer = ((whatsSet&32)!=0) ? 1 : 0;
  info.bounds = ((whatsSet&8)!=0) ? 1 : 0;
  info.columnName = ((whatsSet&16)!=0) ? 1 : 0;
  int numberRows = block->numberRows();
  int numberColumns = block->numberColumns();
  // Which block
  int iRowBlock=addRowBlock(numberRows,block->getRowBlock());
  info.rowBlock=iRowBlock;
  int iColumnBlock=addColumnBlock(numberColumns,block->getColumnBlock());
  info.columnBlock=iColumnBlock;
  int numberErrors=0;
  CoinModelBlockInfo sumInfo=blockType_[numberElementBlocks_-1];;
  int iRhs=(sumInfo.rhs) ? numberElementBlocks_-1 : -1;
  int iRowName=(sumInfo.rowName) ? numberElementBlocks_-1 : -1;
  int iBounds=(sumInfo.bounds) ? numberElementBlocks_-1 : -1;
  int iColumnName=(sumInfo.columnName) ? numberElementBlocks_-1 : -1;
  int iInteger=(sumInfo.integer) ? numberElementBlocks_-1 : -1;
  for (int i=0;i<numberElementBlocks_-1;i++) {
    if (iRowBlock==blockType_[i].rowBlock) {
      if (numberRows!=blocks_[i]->numberRows())
	numberErrors+=1000;
      if (blockType_[i].rhs) {
	if (iRhs<0) {
	  iRhs=i;
	} else {
	  // check
	  const double * a = blocks_[iRhs]->rowLowerArray();
	  const double * b = blocks_[i]->rowLowerArray();
	  if (!sameValues(a,b,numberRows))
	    numberErrors++;
	  a = blocks_[iRhs]->rowUpperArray();
	  b = blocks_[i]->rowUpperArray();
	  if (!sameValues(a,b,numberRows))
	    numberErrors++;
	}
      }
      if (blockType_[i].rowName) {
	if (iRowName<0) {
	  iRowName=i;
	} else {
	  // check
	  if (!sameValues(blocks_[iColumnName],blocks_[i],true))
	    numberErrors++;
	}
      }
    }
    if (iColumnBlock==blockType_[i].columnBlock) {
      if (numberColumns!=blocks_[i]->numberColumns())
	numberErrors+=1000;
      if (blockType_[i].bounds) {
	if (iBounds<0) {
	  iBounds=i;
	} else {
	  // check
	  const double * a = blocks_[iBounds]->columnLowerArray();
	  const double * b = blocks_[i]->columnLowerArray();
	  if (!sameValues(a,b,numberColumns))
	    numberErrors++;
	  a = blocks_[iBounds]->columnUpperArray();
	  b = blocks_[i]->columnUpperArray();
	  if (!sameValues(a,b,numberColumns))
	    numberErrors++;
	  a = blocks_[iBounds]->objectiveArray();
	  b = blocks_[i]->objectiveArray();
	  if (!sameValues(a,b,numberColumns))
	    numberErrors++;
	}
      }
      if (blockType_[i].columnName) {
	if (iColumnName<0) {
	  iColumnName=i;
	} else {
	  // check
	  if (!sameValues(blocks_[iColumnName],blocks_[i],false))
	    numberErrors++;
	}
      }
      if (blockType_[i].integer) {
	if (iInteger<0) {
	  iInteger=i;
	} else {
	  // check
	  const int * a = blocks_[iInteger]->integerTypeArray();
	  const int * b = blocks_[i]->integerTypeArray();
	  if (!sameValues(a,b,numberColumns))
	    numberErrors++;
	}
      }
    }
  }
  return numberErrors;
}
/* add a block from a CoinModel without names*/
int 
CoinStructuredModel::addBlock(const std::string & rowBlock,
			      const std::string & columnBlock,
			      const CoinModel & block)
{
  CoinModel * block2 = new CoinModel(block);
  return addBlock(block2->getRowBlock(),block2->getColumnBlock(),
		  block2);
}
/* add a block from a CoinModel without names*/
int 
CoinStructuredModel::addBlock(const std::string & rowBlock,
			      const std::string & columnBlock,
			      CoinModel * block)
{
  if (numberElementBlocks_==maximumElementBlocks_) {
    maximumElementBlocks_ = 3*(maximumElementBlocks_+10)/2;
    CoinModel ** temp = new CoinModel * [maximumElementBlocks_];
    memcpy(temp,blocks_,numberElementBlocks_*sizeof(CoinModel *));
    delete [] blocks_;
    blocks_ = temp;
    CoinModelBlockInfo * temp2 = new CoinModelBlockInfo [maximumElementBlocks_];
    memcpy(temp2,blockType_,numberElementBlocks_*sizeof(CoinModelBlockInfo));
    delete [] blockType_;
    blockType_ = temp2;
  }
  blocks_[numberElementBlocks_++]=block;
  // Convert matrix
  if (block->type()!=3)
    block->convertMatrix();
  block->setRowBlock(rowBlock);
  block->setColumnBlock(columnBlock);
  int numberErrors=fillInfo(blockType_[numberElementBlocks_-1],block);
  return numberErrors;
}
/* add a block from a CoinModel with names*/
int
CoinStructuredModel::addBlock(const CoinModel & block)
{
  
  //inline const std::string & getRowBlock() const
  //abort();
  return addBlock(block.getRowBlock(),block.getColumnBlock(),
		  block);
}
/* Decompose a CoinModel
   1 - try D-W
   2 - try Benders
   3 - try Staircase
   Returns number of blocks
*/
int 
CoinStructuredModel::decompose(const CoinModel & coinModel, int type)
{
  int numberBlocks=0;
  if (type==1) {
    // Try master at top and bottom
    // but for now cheat
    int numberRows = coinModel.numberRows();
    int * rowBlock = new int[numberRows];
    int iRow;
    for (iRow=0;iRow<numberRows;iRow++)
      rowBlock[iRow]=-2;
    // these are master rows
    if (numberRows==105127) {
      // ken-18
      for (iRow=104976;iRow<numberRows;iRow++)
	rowBlock[iRow]=-1;
    } else if (numberRows==2426) {
      // ken-7
      for (iRow=2401;iRow<numberRows;iRow++)
	rowBlock[iRow]=-1;
    } else if (numberRows==810) {
      for (iRow=81;iRow<84;iRow++)
	rowBlock[iRow]=-1;
    } else if (numberRows==5418) {
      for (iRow=564;iRow<603;iRow++)
	rowBlock[iRow]=-1;
    } else if (numberRows==10280) {
      // osa-60
      for (iRow=10198;iRow<10280;iRow++)
	rowBlock[iRow]=-1;
    } else if (numberRows==1503) {
      // degen3
      for (iRow=0;iRow<561;iRow++)
	rowBlock[iRow]=-1;
    } else if (numberRows==929) {
      // czprob
      for (iRow=0;iRow<39;iRow++)
	rowBlock[iRow]=-1;
    }
    const CoinPackedMatrix * matrix = coinModel.packedMatrix();
    assert (matrix!=NULL);
    // get row copy
    CoinPackedMatrix rowCopy = *matrix;
    rowCopy.reverseOrdering();
    const int * row = matrix->getIndices();
    const int * columnLength = matrix->getVectorLengths();
    const CoinBigIndex * columnStart = matrix->getVectorStarts();
    //const double * elementByColumn = matrix->getElements();
    const int * column = rowCopy.getIndices();
    const int * rowLength = rowCopy.getVectorLengths();
    const CoinBigIndex * rowStart = rowCopy.getVectorStarts();
    //const double * elementByRow = rowCopy.getElements();
    int * stack = new int [numberRows];
    // to say if column looked at
    int numberColumns = coinModel.numberColumns();
    int * columnBlock = new int[numberColumns];
    int iColumn;
    for (iColumn=0;iColumn<numberColumns;iColumn++)
      columnBlock[iColumn]=-2;
    for (iColumn=0;iColumn<numberColumns;iColumn++) {
      int kstart = columnStart[iColumn];
      int kend = columnStart[iColumn]+columnLength[iColumn];
      if (columnBlock[iColumn]==-2) {
	// column not allocated
	int j;
	int nstack=0;
	for (j=kstart;j<kend;j++) {
	  int iRow= row[j];
	  if (rowBlock[iRow]!=-1) {
	    assert(rowBlock[iRow]==-2);
	    rowBlock[iRow]=numberBlocks; // mark
	    stack[nstack++] = iRow;
	  }
	}
	if (nstack) {
	  // new block - put all connected in
	  numberBlocks++;
	  columnBlock[iColumn]=numberBlocks-1;
	  while (nstack) {
	    int iRow = stack[--nstack];
	    int k;
	    for (k=rowStart[iRow];k<rowStart[iRow]+rowLength[iRow];k++) {
	      int iColumn = column[k];
	      int kkstart = columnStart[iColumn];
	      int kkend = kkstart + columnLength[iColumn];
	      if (columnBlock[iColumn]==-2) {
		columnBlock[iColumn]=numberBlocks-1; // mark
		// column not allocated
		int jj;
		for (jj=kkstart;jj<kkend;jj++) {
		  int jRow= row[jj];
		  if (rowBlock[jRow]==-2) {
		    rowBlock[jRow]=numberBlocks-1;
		    stack[nstack++]=jRow;
		  }
		}
	      } else {
		assert (columnBlock[iColumn]==numberBlocks-1);
	      }
	    }
	  }
	} else {
	  // Only in master
	  columnBlock[iColumn]=-1;
	}
      }
    }
    delete [] stack;
    int numberMasterRows=0;
    for (iRow=0;iRow<numberRows;iRow++) {
      int iBlock = rowBlock[iRow];
      if (iBlock==-1)
	numberMasterRows++;
    }
    int numberMasterColumns=0;
    for (iColumn=0;iColumn<numberColumns;iColumn++) {
      int iBlock = columnBlock[iColumn];
      if (iBlock==-1)
	numberMasterColumns++;
    }
    printf("%d blocks found - %d rows, %d columns in master\n",
	   numberBlocks,numberMasterRows,numberMasterColumns);
    if (numberBlocks) {
      if (numberBlocks>50) {
	int iBlock;
	for (iRow=0;iRow<numberRows;iRow++) {
	  iBlock = rowBlock[iRow];
	  if (iBlock>=0)
	    rowBlock[iRow] = iBlock%50;
	}
	for (iColumn=0;iColumn<numberColumns;iColumn++) {
	  iBlock = columnBlock[iColumn];
	  if (iBlock>=0)
	    columnBlock[iColumn] = iBlock%50;
	}
	numberBlocks=50;
      }
    }
    // Name for master so at top
    addRowBlock(numberMasterRows,"row_master");
    // make up problems
    // Create all sub problems
    // Could do faster
    int * whichRow = new int [numberRows];
    int * whichColumn = new int [numberColumns];
    // Space for creating
    double * obj = new double [numberColumns];
    double * columnLo = new double [numberColumns];
    double * columnUp = new double [numberColumns];
    double * rowLo = new double [numberRows];
    double * rowUp = new double [numberRows];
    // Arrays
    const double * objective = coinModel.objectiveArray();
    const double * columnLower = coinModel.columnLowerArray();
    const double * columnUpper = coinModel.columnUpperArray();
    const double * rowLower = coinModel.rowLowerArray();
    const double * rowUpper = coinModel.rowUpperArray();
    // get full matrix
    CoinPackedMatrix fullMatrix = *coinModel.packedMatrix();
    int numberRow2,numberColumn2;
    int iBlock;
    for (iBlock=0;iBlock<numberBlocks;iBlock++) {
      char rowName[20];
      sprintf(rowName,"row_%d",iBlock);
      char columnName[20];
      sprintf(columnName,"column_%d",iBlock);
      numberRow2=0;
      numberColumn2=0;
      for (iRow=0;iRow<numberRows;iRow++) {
	if (iBlock==rowBlock[iRow]) {
	  rowLo[numberRow2]=rowLower[iRow];
	  rowUp[numberRow2]=rowUpper[iRow];
	  whichRow[numberRow2++]=iRow;
	}
      }
      for (iColumn=0;iColumn<numberColumns;iColumn++) {
	if (iBlock==columnBlock[iColumn]) {
	  obj[numberColumn2]=objective[iColumn];
	  columnLo[numberColumn2]=columnLower[iColumn];
	  columnUp[numberColumn2]=columnUpper[iColumn];
	  whichColumn[numberColumn2++]=iColumn;
	}
      }
      // Diagonal block
      CoinPackedMatrix mat(fullMatrix,
			   numberRow2,whichRow,
			   numberColumn2,whichColumn);
      CoinModel * block = new CoinModel(numberRow2,numberColumn2,&mat,
					rowLo,rowUp,NULL,NULL,NULL);
      block->setOriginalIndices(whichRow,whichColumn);
      addBlock(rowName,columnName,block); // takes ownership
      // and top block
      numberRow2=0;
      // get top matrix
      for (iRow=0;iRow<numberRows;iRow++) {
	int iBlock = rowBlock[iRow];
	if (iBlock<0) {
	  whichRow[numberRow2++]=iRow;
	}
      }
      CoinPackedMatrix top(fullMatrix,
			   numberRow2,whichRow,
			   numberColumn2,whichColumn);
      block = new CoinModel(numberMasterRows,numberColumn2,&top,
			    NULL,NULL,columnLo,columnUp,obj);
      block->setOriginalIndices(whichRow,whichColumn);
      addBlock("row_master",columnName,block); // takes ownership
    }
    // and master
    numberRow2=0;
    numberColumn2=0;
    for (iRow=0;iRow<numberRows;iRow++) {
      int iBlock = rowBlock[iRow];
      if (iBlock<0) {
	rowLo[numberRow2]=rowLower[iRow];
	rowUp[numberRow2]=rowUpper[iRow];
	whichRow[numberRow2++]=iRow;
      }
    }
    for (iColumn=0;iColumn<numberColumns;iColumn++) {
      int iBlock = columnBlock[iColumn];
      if (iBlock<0) {
	obj[numberColumn2]=objective[iColumn];
	columnLo[numberColumn2]=columnLower[iColumn];
	columnUp[numberColumn2]=columnUpper[iColumn];
	whichColumn[numberColumn2++]=iColumn;
      }
    }
    delete [] rowBlock;
    delete [] columnBlock;
    CoinPackedMatrix top(fullMatrix,
			 numberRow2,whichRow,
			 numberColumn2,whichColumn);
    CoinModel * block = new CoinModel(numberRow2,numberColumn2,&top,
				      rowLo,rowUp,
				      columnLo,columnUp,obj);
    block->setOriginalIndices(whichRow,whichColumn);
    addBlock("row_master","column_master",block); // takes ownership
    delete [] whichRow;
    delete [] whichColumn;
    delete [] obj ; 
    delete [] columnLo ; 
    delete [] columnUp ; 
    delete [] rowLo ; 
    delete [] rowUp ; 
  } else {
    abort();
  }
  return numberBlocks;
}
// Return block corresponding to row and column
const CoinModel *  
CoinStructuredModel::block(int row,int column) const
{
  const CoinModel * block = NULL;
  if (blockType_) {
    for (int iBlock=0;iBlock<numberElementBlocks_;iBlock++) {
      if (blockType_[iBlock].rowBlock==row&&
	  blockType_[iBlock].columnBlock==column) {
	block = blocks_[iBlock];
	break;
      }
    }
  }
  return block;
}
/* Fill pointers corresponding to row and column.
   False if any missing */
CoinModelBlockInfo 
CoinStructuredModel::block(int row,int column,
			   const double * & rowLower, const double * & rowUpper,
			   const double * & columnLower, const double * & columnUpper,
			   const double * & objective) const
{
  CoinModelBlockInfo info;
  memset(&info,0,sizeof(info));
  rowLower=NULL;
  rowUpper=NULL;
  columnLower=NULL;
  columnUpper=NULL;
  objective=NULL;
  if (blockType_) {
    for (int iBlock=0;iBlock<numberElementBlocks_;iBlock++) {
      if (blockType_[iBlock].rowBlock==row) {
	if (blockType_[iBlock].rhs) {
	  info.rhs=1;
	  rowLower = blocks_[iBlock]->rowLowerArray();
	  rowUpper = blocks_[iBlock]->rowUpperArray();
	}
      }
      if (blockType_[iBlock].columnBlock==column) {
	if (blockType_[iBlock].bounds) {
	  info.bounds=1;
	  columnLower = blocks_[iBlock]->columnLowerArray();
	  columnUpper = blocks_[iBlock]->columnUpperArray();
	  objective = blocks_[iBlock]->objectiveArray();
	}
      }
    }
  }
  return info;
}
// Return block number corresponding to row and column
int  
CoinStructuredModel::blockIndex(int row,int column) const
{
  int block=-1;
  if (blockType_) {
    for (int iBlock=0;iBlock<numberElementBlocks_;iBlock++) {
      if (blockType_[iBlock].rowBlock==row&&
	  blockType_[iBlock].columnBlock==column) {
	block = iBlock;
	break;
      }
    }
  }
  return block;
}
