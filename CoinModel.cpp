// Copyright (C) 2005, International Business Machines
// Corporation and others.  All Rights Reserved.


#include "CoinHelperFunctions.hpp"
#include "CoinModel.hpp"
#include "CoinSort.hpp"
#include "CoinMpsIO.hpp"

//#############################################################################
// Constructors / Destructor / Assignment
//#############################################################################

//-------------------------------------------------------------------
// Default Constructor 
//-------------------------------------------------------------------
CoinModel::CoinModel () 
 :  numberRows_(0),
    maximumRows_(0),
    numberColumns_(0),
    maximumColumns_(0),
    numberElements_(0),
    maximumElements_(0),
    numberQuadraticElements_(0),
    maximumQuadraticElements_(0),
    optimizationDirection_(1.0),
    rowLower_(NULL),
    rowUpper_(NULL),
    rowType_(NULL),
    objective_(NULL),
    columnLower_(NULL),
    columnUpper_(NULL),
    integerType_(NULL),
    columnType_(NULL),
    start_(NULL),
    elements_(NULL),
    quadraticElements_(NULL),
    sortIndices_(NULL),
    sortElements_(NULL),
    sortSize_(0),
    type_(-1),
    links_(0)
{
}

//-------------------------------------------------------------------
// Copy constructor 
//-------------------------------------------------------------------
CoinModel::CoinModel (const CoinModel & rhs) 
  : numberRows_(rhs.numberRows_),
    maximumRows_(rhs.maximumRows_),
    numberColumns_(rhs.numberColumns_),
    maximumColumns_(rhs.maximumColumns_),
    numberElements_(rhs.numberElements_),
    maximumElements_(rhs.maximumElements_),
    numberQuadraticElements_(rhs.numberQuadraticElements_),
    maximumQuadraticElements_(rhs.maximumQuadraticElements_),
    optimizationDirection_(rhs.optimizationDirection_),
    rowName_(rhs.rowName_),
    columnName_(rhs.columnName_),
    string_(rhs.string_),
    hashElements_(rhs.hashElements_),
    rowList_(rhs.rowList_),
    columnList_(rhs.columnList_),
    hashQuadraticElements_(rhs.hashQuadraticElements_),
    sortSize_(rhs.sortSize_),
    quadraticRowList_(rhs.quadraticRowList_),
    quadraticColumnList_(rhs.quadraticColumnList_),
    type_(rhs.type_),
    links_(rhs.links_)
{
  rowLower_ = CoinCopyOfArray(rhs.rowLower_,maximumRows_);
  rowUpper_ = CoinCopyOfArray(rhs.rowUpper_,maximumRows_);
  rowType_ = CoinCopyOfArray(rhs.rowType_,maximumRows_);
  objective_ = CoinCopyOfArray(rhs.objective_,maximumColumns_);
  columnLower_ = CoinCopyOfArray(rhs.columnLower_,maximumColumns_);
  columnUpper_ = CoinCopyOfArray(rhs.columnUpper_,maximumColumns_);
  integerType_ = CoinCopyOfArray(rhs.integerType_,maximumColumns_);
  columnType_ = CoinCopyOfArray(rhs.columnType_,maximumColumns_);
  sortIndices_ = CoinCopyOfArray(rhs.sortIndices_,sortSize_);
  sortElements_ = CoinCopyOfArray(rhs.sortElements_,sortSize_);
  if (type_==0) {
    start_ = CoinCopyOfArray(rhs.start_,maximumRows_+1);
  } else if (type_==1) {
    start_ = CoinCopyOfArray(rhs.start_,maximumColumns_+1);
  } else {
    start_=NULL;
  }
  elements_ = CoinCopyOfArray(rhs.elements_,maximumElements_);
  quadraticElements_ = CoinCopyOfArray(rhs.quadraticElements_,maximumQuadraticElements_);
}

//-------------------------------------------------------------------
// Destructor 
//-------------------------------------------------------------------
CoinModel::~CoinModel ()
{
  delete [] rowLower_;
  delete [] rowUpper_;
  delete [] rowType_;
  delete [] objective_;
  delete [] columnLower_;
  delete [] columnUpper_;
  delete [] integerType_;
  delete [] columnType_;
  delete [] start_;
  delete [] elements_;
  delete [] quadraticElements_;
  delete [] sortIndices_;
  delete [] sortElements_;
}

//----------------------------------------------------------------
// Assignment operator 
//-------------------------------------------------------------------
CoinModel &
CoinModel::operator=(const CoinModel& rhs)
{
  if (this != &rhs) {
    delete [] rowLower_;
    delete [] rowUpper_;
    delete [] rowType_;
    delete [] objective_;
    delete [] columnLower_;
    delete [] columnUpper_;
    delete [] integerType_;
    delete [] columnType_;
    delete [] start_;
    delete [] elements_;
    delete [] quadraticElements_;
    delete [] sortIndices_;
    delete [] sortElements_;
    numberRows_ = rhs.numberRows_;
    maximumRows_ = rhs.maximumRows_;
    numberColumns_ = rhs.numberColumns_;
    maximumColumns_ = rhs.maximumColumns_;
    numberElements_ = rhs.numberElements_;
    maximumElements_ = rhs.maximumElements_;
    numberQuadraticElements_ = rhs.numberQuadraticElements_;
    maximumQuadraticElements_ = rhs.maximumQuadraticElements_;
    optimizationDirection_ = rhs.optimizationDirection_;
    sortSize_ = rhs.sortSize_;
    rowName_ = rhs.rowName_;
    columnName_ = rhs.columnName_;
    string_ = rhs.string_;
    hashElements_=rhs.hashElements_;
    hashQuadraticElements_=rhs.hashQuadraticElements_;
    rowList_ = rhs.rowList_;
    quadraticColumnList_ = rhs.quadraticColumnList_;
    quadraticRowList_ = rhs.quadraticRowList_;
    columnList_ = rhs.columnList_;
    type_ = rhs.type_;
    links_ = rhs.links_;
    rowLower_ = CoinCopyOfArray(rhs.rowLower_,maximumRows_);
    rowUpper_ = CoinCopyOfArray(rhs.rowUpper_,maximumRows_);
    rowType_ = CoinCopyOfArray(rhs.rowType_,maximumRows_);
    objective_ = CoinCopyOfArray(rhs.objective_,maximumColumns_);
    columnLower_ = CoinCopyOfArray(rhs.columnLower_,maximumColumns_);
    columnUpper_ = CoinCopyOfArray(rhs.columnUpper_,maximumColumns_);
    integerType_ = CoinCopyOfArray(rhs.integerType_,maximumColumns_);
    columnType_ = CoinCopyOfArray(rhs.columnType_,maximumColumns_);
    if (type_==0) {
      start_ = CoinCopyOfArray(rhs.start_,maximumRows_+1);
    } else if (type_==1) {
      start_ = CoinCopyOfArray(rhs.start_,maximumColumns_+1);
    } else {
      start_=NULL;
    }
    elements_ = CoinCopyOfArray(rhs.elements_,maximumElements_);
    quadraticElements_ = CoinCopyOfArray(rhs.quadraticElements_,maximumQuadraticElements_);
    sortIndices_ = CoinCopyOfArray(rhs.sortIndices_,sortSize_);
    sortElements_ = CoinCopyOfArray(rhs.sortElements_,sortSize_);
  }
  return *this;
}
/* add a row -  numberInRow may be zero */
void 
CoinModel::addRow(int numberInRow, const int * columns,
                  const double * elements, double rowLower, 
                  double rowUpper, const char * name)
{
  if (type_==-1) {
    // initial
    type_=0;
    resize(100,0,1000);
  } else if (type_==1) {
    // mixed - do linked lists for rows
    rowList_.create(maximumRows_,maximumElements_,
                    numberRows_,numberColumns_,0,
                    numberElements_,elements_);
    type_=2;
    links_ |= 1;
  }
  int newColumn=0;
  if (numberInRow>0) {
    // Move and sort
    if (numberInRow>sortSize_) {
      delete [] sortIndices_;
      delete [] sortElements_;
      sortSize_ = numberInRow+100; 
      sortIndices_ = new int [sortSize_];
      sortElements_ = new double [sortSize_];
    }
    bool sorted = true;
    int last=-1;
    int i;
    for (i=0;i<numberInRow;i++) {
      int k=columns[i];
      if (k<=last)
        sorted=false;
      last=k;
      sortIndices_[i]=k;
      sortElements_[i]=elements[i];
    }
    if (!sorted) {
      CoinSort_2(sortIndices_,sortIndices_+numberInRow,sortElements_);
    }
    // check for duplicates etc
    if (sortIndices_[0]<0) {
      printf("bad index %d\n",sortIndices_[0]);
      // clean up
      abort();
    }
    last=-1;
    bool duplicate=false;
    for (i=0;i<numberInRow;i++) {
      int k=sortIndices_[i];
      if (k==last)
        duplicate=true;
      last=k;
    }
    if (duplicate) {
      printf("duplicates - what do we want\n");
      abort();
    }
    if (last>=numberColumns_) {
      newColumn = last+1;
    }
  }
  int newRow=0;
  int newElement=0;
  if (numberElements_+numberInRow>maximumElements_) {
    newElement = (3*(numberElements_+numberInRow)/2) + 1000;
    if (numberRows_*10>maximumRows_*9)
      newRow = (maximumRows_*3)/2+1000;
  }
  if (numberRows_==maximumRows_)
    newRow = (maximumRows_*3)/2+1000;
  if (newRow||newColumn||newElement) {
    if (!newColumn) {
      // columns okay
      resize(newRow,0,newElement);
    } else {
      // newColumn will be new numberColumns_
      resize(newRow,(3*newColumn)/2+100,newElement);
    }
  }
  // Do name
  if (name)
    rowName_.addHash(numberRows_,name);
  rowLower_[numberRows_]=rowLower;
  rowUpper_[numberRows_]=rowUpper;
  // If columns extended - take care of that
  fillColumns(newColumn,false);
  if (type_==0) {
    // can do simply
    int put = start_[numberRows_];
    assert (put==numberElements_);
    bool doHash = hashElements_.numberItems()!=0;
    for (int i=0;i<numberInRow;i++) {
      elements_[put].row=numberRows_;
      elements_[put].string=0;
      elements_[put].column=sortIndices_[i];
      elements_[put].value=sortElements_[i];
      if (doHash)
        hashElements_.addHash(put,numberRows_,sortIndices_[i],elements_);
      put++;
    }
    start_[numberRows_+1]=put;
  } else {
    if (numberInRow) {
      // must update at least one link
      assert (links_);
      if (links_==1||links_==3) {
        int first = rowList_.addEasy(numberRows_,numberInRow,sortIndices_,sortElements_,elements_,
                                     hashElements_);
        if (links_==3)
          columnList_.addHard(first,elements_,rowList_.firstFree(),rowList_.lastFree(),
                              rowList_.next());
      } else if (links_==2) {
        columnList_.addHard(numberRows_,numberInRow,sortIndices_,sortElements_,elements_,
                            hashElements_);
      }
    }
  }
  numberRows_++;
  numberElements_+=numberInRow;
}
// add a column - numberInColumn may be zero */
void 
CoinModel::addColumn(int numberInColumn, const int * rows,
                     const double * elements, 
                     double columnLower, 
                     double columnUpper, double objectiveValue,
                     const char * name, bool isInteger)
{
  if (type_==-1) {
    // initial
    type_=1;
    resize(0,100,1000);
  } else if (type_==0) {
    // mixed - do linked lists for columns
    columnList_.create(maximumColumns_,maximumElements_,
                    numberColumns_,numberRows_,1,
                    numberElements_,elements_);
    type_=2;
    links_ |= 2;
  }
  int newRow=0;
  if (numberInColumn>0) {
    // Move and sort
    if (numberInColumn>sortSize_) {
      delete [] sortIndices_;
      delete [] sortElements_;
      sortSize_ = numberInColumn+100; 
      sortIndices_ = new int [sortSize_];
      sortElements_ = new double [sortSize_];
    }
    bool sorted = true;
    int last=-1;
    int i;
    for (i=0;i<numberInColumn;i++) {
      int k=rows[i];
      if (k<=last)
        sorted=false;
      last=k;
      sortIndices_[i]=k;
      sortElements_[i]=elements[i];
    }
    if (!sorted) {
      CoinSort_2(sortIndices_,sortIndices_+numberInColumn,sortElements_);
    }
    // check for duplicates etc
    if (sortIndices_[0]<0) {
      printf("bad index %d\n",sortIndices_[0]);
      // clean up
      abort();
    }
    last=-1;
    bool duplicate=false;
    for (i=0;i<numberInColumn;i++) {
      int k=sortIndices_[i];
      if (k==last)
        duplicate=true;
      last=k;
    }
    if (duplicate) {
      printf("duplicates - what do we want\n");
      abort();
    }
    if (last>=numberRows_) {
      newRow = last+1;
    }
  }
  int newColumn=0;
  int newElement=0;
  if (numberElements_+numberInColumn>maximumElements_) {
    newElement = (3*(numberElements_+numberInColumn)/2) + 1000;
    if (numberColumns_*10>maximumColumns_*9)
      newColumn = (maximumColumns_*3)/2+1000;
  }
  if (numberColumns_==maximumColumns_)
    newColumn = (maximumColumns_*3)/2+1000;
  if (newColumn||newRow||newElement) {
    if (!newRow) {
      // rows okay
      resize(0,newColumn,newElement);
    } else {
      // newRow will be new numberRows_
      resize((3*newRow)/2+100,newColumn,newElement);
    }
  }
  // Do name
  if (name)
    columnName_.addHash(numberColumns_,name);
  columnLower_[numberColumns_]=columnLower;
  columnUpper_[numberColumns_]=columnUpper;
  // If rows extended - take care of that
  fillRows(newRow,false);
  if (type_==1) {
    // can do simply
    int put = start_[numberColumns_];
    assert (put==numberElements_);
    bool doHash = hashElements_.numberItems()!=0;
    for (int i=0;i<numberInColumn;i++) {
      elements_[put].column=numberColumns_;
      elements_[put].string=0;
      elements_[put].row=sortIndices_[i];
      elements_[put].value=sortElements_[i];
      if (doHash)
        hashElements_.addHash(put,sortIndices_[i],numberColumns_,elements_);
      put++;
    }
    start_[numberColumns_+1]=put;
  } else {
    if (numberInColumn) {
      // must update at least one link
      assert (links_);
      if (links_==1||links_==3) {
        int first = columnList_.addEasy(numberColumns_,numberInColumn,sortIndices_,sortElements_,elements_,
                                        hashElements_);
        if (links_==3)
          rowList_.addHard(first,elements_,columnList_.firstFree(),columnList_.lastFree(),
                              columnList_.next());
      } else if (links_==2) {
        rowList_.addHard(numberColumns_,numberInColumn,sortIndices_,sortElements_,elements_,
                         hashElements_);
      }
    }
  }
  numberColumns_++;
  numberElements_+=numberInColumn;
}
// Sets value for row i and column j 
void 
CoinModel::setElement(int i,int j,double value) 
{
  if (type_==-1) {
    // initial
    type_=2;
    resize(100,100,1000);
    rowList_.create(maximumRows_,maximumElements_,
                    numberRows_,numberColumns_,0,
                    numberElements_,elements_);
    links_ |= 1;
  }
  if (!hashElements_.numberItems()) {
    hashElements_.resize(maximumElements_,elements_);
  }
  int position = hashElements_.hash(i,j,elements_);
  if (position>=0) {
    elements_[position].value=value;
  } else {
    int newColumn=0;
    if (j>=numberColumns_) {
      newColumn = j+1;
    }
    int newRow=0;
    if (i>=numberRows_) {
      newRow = i+1;
    }
    int newElement=0;
    if (numberElements_==maximumElements_) {
      newElement = (3*numberElements_/2) + 1000;
    }
    if (newRow||newColumn||newElement) {
      if (newColumn) 
        newColumn = (3*newColumn)/2+100;
      if (newRow) 
        newRow = (3*newRow)/2+100;
      resize(newRow,newColumn,newElement);
    }
    // If columns extended - take care of that
    fillColumns(j,false);
    // If rows extended - take care of that
    fillRows(j,false);
    // treat as addRow unless only columnList_ exists
    if ((links_&1)!=0) {
      int first = rowList_.addEasy(i,1,&j,&value,elements_,hashElements_);
      if (links_==3)
        columnList_.addHard(first,elements_,rowList_.firstFree(),rowList_.lastFree(),
                            rowList_.next());
    } else if (links_==2) {
      columnList_.addHard(i,1,&j,&value,elements_,hashElements_);
    }
    numberRows_=CoinMax(numberRows_,i+1);;
    numberColumns_=CoinMax(numberColumns_,i+1);;
    numberElements_++;
  }
}
// Sets quadratic value for column i and j 
void 
CoinModel::setQuadraticElement(int i,int j,double value) 
{
  printf("not written yet\n");
  abort();
  return;
}
// Sets value for row i and column j as string
void 
CoinModel::setElement(int i,int j,const char * value) 
{
  printf("not written yet\n");
  abort();
  return;
}
// Associates a string with a value.  Returns string id (or -1 if does not exist)
int 
CoinModel::associateElement(const char * stringValue, double value)
{
  printf("not written yet\n");
  abort();
  return 0;
}
/* Sets rowLower (if row does not exist then
   all rows up to this are defined with default values and no elements)
*/
void 
CoinModel::setRowLower(int whichRow,double rowLower)
{
  assert (whichRow>=0);
  // make sure enough room and fill
  fillRows(whichRow,true);
  rowLower_[whichRow]=rowLower;
}
/* Sets rowUpper (if row does not exist then
   all rows up to this are defined with default values and no elements)
*/
void 
CoinModel::setRowUpper(int whichRow,double rowUpper)
{
  assert (whichRow>=0);
  // make sure enough room and fill
  fillRows(whichRow,true);
  rowUpper_[whichRow]=rowUpper;
}
/* Sets rowLower and rowUpper (if row does not exist then
   all rows up to this are defined with default values and no elements)
*/
void 
CoinModel::setRowBounds(int whichRow,double rowLower,double rowUpper)
{
  assert (whichRow>=0);
  // make sure enough room and fill
  fillRows(whichRow,true);
  rowLower_[whichRow]=rowLower;
  rowUpper_[whichRow]=rowUpper;
}
/* Sets name (if row does not exist then
   all rows up to this are defined with default values and no elements)
*/
void 
CoinModel::setRowName(int whichRow,const char * rowName)
{
  assert (whichRow>=0);
  if (!rowName)
    return;
  // make sure enough room and fill
  fillRows(whichRow,true);
  const char * oldName = rowName_.name(whichRow);
  if (oldName)
    rowName_.deleteHash(whichRow);
  rowName_.addHash(whichRow,rowName);
}
/* Sets columnLower (if column does not exist then
   all columns up to this are defined with default values and no elements)
*/
void 
CoinModel::setColumnLower(int whichColumn,double columnLower)
{
  assert (whichColumn>=0);
  // make sure enough room and fill
  fillColumns(whichColumn,true);
  columnLower_[whichColumn]=columnLower;
}
/* Sets columnUpper (if column does not exist then
   all columns up to this are defined with default values and no elements)
*/
void 
CoinModel::setColumnUpper(int whichColumn,double columnUpper)
{
  assert (whichColumn>=0);
  // make sure enough room and fill
  fillColumns(whichColumn,true);
  columnUpper_[whichColumn]=columnUpper;
}
/* Sets columnLower and columnUpper (if column does not exist then
   all columns up to this are defined with default values and no elements)
*/
void 
CoinModel::setColumnBounds(int whichColumn,double columnLower,double columnUpper)
{
  assert (whichColumn>=0);
  // make sure enough room and fill
  fillColumns(whichColumn,true);
  columnLower_[whichColumn]=columnLower;
  columnUpper_[whichColumn]=columnUpper;
}
/* Sets columnObjective (if column does not exist then
   all columns up to this are defined with default values and no elements)
*/
void 
CoinModel::setColumnObjective(int whichColumn,double columnObjective)
{
  assert (whichColumn>=0);
  // make sure enough room and fill
  fillColumns(whichColumn,true);
  objective_[whichColumn]=columnObjective;
}
/* Sets name (if column does not exist then
   all columns up to this are defined with default values and no elements)
*/
void 
CoinModel::setColumnName(int whichColumn,const char * columnName)
{
  assert (whichColumn>=0);
  if (!columnName)
    return;
  // make sure enough room and fill
  fillColumns(whichColumn,true);
  const char * oldName = columnName_.name(whichColumn);
  if (oldName)
    columnName_.deleteHash(whichColumn);
  columnName_.addHash(whichColumn,columnName);
}
/* Sets integer (if column does not exist then
   all columns up to this are defined with default values and no elements)
*/
void 
CoinModel::setColumnIsInteger(int whichColumn,bool columnIsInteger)
{
  assert (whichColumn>=0);
  // make sure enough room and fill
  fillColumns(whichColumn,true);
  integerType_[whichColumn]=(columnIsInteger) ? 1 : 0;
}
/* Deletes all entries in row and bounds.  If last row the number of rows
   will be decremented and true returned.  */
bool 
CoinModel::deleteRow(int whichRow)
{
  printf("not written yet\n");
  abort();
  return false;
}
/* Deletes all entries in column and bounds.  If last column the number of columns
   will be decremented and true returned.  */
bool 
CoinModel::deleteColumn(int whichColumn)
{
  printf("not written yet\n");
  abort();
  return false;
}
/* Packs down all rows i.e. removes empty rows permanently.  Empty rows
   have no elements and feasible bounds. returns number of rows deleted. */
int 
CoinModel::packRows()
{
  printf("not written yet\n");
  abort();
  return 0;
}
/* Packs down all columns i.e. removes empty columns permanently.  Empty columns
   have no elements and no objective. returns number of columns deleted. */
int 
CoinModel::packColumns()
{
  printf("not written yet\n");
  abort();
  return 0;
}
/* Packs down all rows and columns.  i.e. removes empty rows and columns permanently.
   Empty rows have no elements and feasible bounds.
   Empty columns have no elements and no objective.
   returns number of rows+columns deleted. */
int 
CoinModel::pack()
{
  printf("not written yet\n");
  abort();
  return 0;
}

/* Write the problem in MPS format to a file with the given filename.
 */
int 
CoinModel::writeMps(const char *filename, int compression,
                    int formatType , int numberAcross ) 
{
  // Set to say all parts
  type_=2;
  resize(numberRows_,numberColumns_,numberElements_);
  // Do counts for CoinPackedMatrix
  int * length = new int[numberColumns_];
  CoinZeroN(length,numberColumns_);
  int i;
  int numberElements=0;
  for (i=0;i<numberElements_;i++) {
    int column = elements_[i].column;
    if (column>=0) {
      length[column]++;
      numberElements++;
    }
  }
  CoinBigIndex * start = new int[numberColumns_+1];
  int * row = new int[numberElements];
  double * element = new double[numberElements];
  start[0]=0;
  for (i=0;i<numberColumns_;i++) {
    start[i+1]=start[i]+length[i];
    length[i]=0;
  }
  for (i=0;i<numberElements_;i++) {
    int column = elements_[i].column;
    if (column>=0) {
      int put=start[column]+length[column];
      row[put]=(int) elements_[i].row;
      element[put]=elements_[i].value;
      length[column]++;
    }
  }
  for (i=0;i<numberColumns_;i++) {
    int put = start[i];
    CoinSort_2(row+put,row+put+length[i],element+put);
  }
  CoinPackedMatrix matrix(true,numberRows_,numberColumns_,numberElements,
                          element,row,start,length);
  delete [] start;
  delete [] length;
  delete [] row;
  delete [] element;
  char* integrality = new char[numberColumns_];
  bool hasInteger = false;
  for (i = 0; i < numberColumns_; i++) {
    if (integerType_[i]) {
      integrality[i] = 1;
      hasInteger = true;
    } else {
      integrality[i] = 0;
    }
  }

  CoinMpsIO writer;
  writer.setInfinity(COIN_DBL_MAX);
  const char *const * rowNames=NULL;
  if (rowName_.numberItems())
    rowNames=rowName_.names();
  const char * const * columnNames=NULL;
  if (columnName_.numberItems())
    columnNames=columnName_.names();
  writer.setMpsData(matrix, COIN_DBL_MAX,
                    columnLower_, columnUpper_,
                    objective_, hasInteger ? integrality : 0,
		     rowLower_, rowUpper_,
		     columnNames,rowNames);
  delete[] integrality;
  return writer.writeMps(filename, compression, formatType, numberAcross);
}
// Returns value for row i and column j
double 
CoinModel::getElement(int i,int j) const
{
  printf("not written yet\n");
  abort();
  return 0.0;
}
// Returns quadratic value for columns i and j
double 
CoinModel::getQuadraticElement(int i,int j) const
{
  printf("not written yet\n");
  abort();
  return 0.0;
}
// Returns value for row i and column j as string
const char * 
CoinModel::getElementAsString(int i,int j) const
{
  printf("not written yet\n");
  abort();
  return NULL;
}
/* Returns pointer to element for row i column j.
   Only valid until next modification. 
   NULL if element does not exist */
double * 
CoinModel::pointer (int i,int j) const
{
  printf("not written yet\n");
  abort();
  return NULL;
}

  
/* Returns first element in given row - index is -1 if none.
   Index is given by .index and value by .value
*/
CoinModelLink 
CoinModel::firstInRow(int whichRow) const
{
  CoinModelLink link;
  if (whichRow>=0&&whichRow<numberRows_) {
    link.setRow(whichRow);
    link.setOnRow(true);
    if (type_==0) {
      assert (start_);
      int position = start_[whichRow];
      if (position<start_[whichRow+1]) {
        link.setPosition(position);
        link.setColumn(elements_[position].column);
        assert (whichRow==(int) elements_[position].row);
        link.setValue(elements_[position].value);
      }
    } else {
      if ((links_&1)==0) {
        // Create list
        assert (!rowList_.numberMajor());
        rowList_.create(maximumRows_,maximumElements_,numberRows_,numberColumns_,0,
                        numberElements_,elements_);
      }
      int position = rowList_.first(whichRow);
      if (position>=0) {
        link.setPosition(position);
        link.setColumn(elements_[position].column);
        assert (whichRow==(int) elements_[position].row);
        link.setValue(elements_[position].value);
      }
    }
  }
  return link;
}
/* Returns last element in given row - index is -1 if none.
   Index is given by .index and value by .value
  */
CoinModelLink 
CoinModel::lastInRow(int whichRow) const
{
  CoinModelLink link;
  if (whichRow>=0&&whichRow<numberRows_) {
    link.setRow(whichRow);
    link.setOnRow(true);
    if (type_==0) {
      assert (start_);
      int position = start_[whichRow+1]-1;
      if (position>=start_[whichRow]) {
        link.setPosition(position);
        link.setColumn(elements_[position].column);
        assert (whichRow==(int) elements_[position].row);
        link.setValue(elements_[position].value);
      }
    } else {
      if ((links_&1)==0) {
        // Create list
        assert (!rowList_.numberMajor());
        rowList_.create(maximumRows_,maximumElements_,numberRows_,numberColumns_,0,
                        numberElements_,elements_);
      }
      int position = rowList_.last(whichRow);
      if (position>=0) {
        link.setPosition(position);
        link.setColumn(elements_[position].column);
        assert (whichRow==(int) elements_[position].row);
        link.setValue(elements_[position].value);
      }
    }
  }
  return link;
}
/* Returns first element in given column - index is -1 if none.
   Index is given by .index and value by .value
*/
CoinModelLink 
CoinModel::firstInColumn(int whichColumn) const
{
  CoinModelLink link;
  if (whichColumn>=0&&whichColumn<numberColumns_) {
    link.setColumn(whichColumn);
    link.setOnRow(false);
    if (type_==1) {
      assert (start_);
      int position = start_[whichColumn];
      if (position<start_[whichColumn+1]) {
        link.setPosition(position);
        link.setRow(elements_[position].row);
        assert (whichColumn==(int) elements_[position].column);
        link.setValue(elements_[position].value);
      }
    } else {
      if ((links_&2)==0) {
        // Create list
        assert (!columnList_.numberMajor());
        columnList_.create(maximumColumns_,maximumElements_,numberColumns_,numberRows_,0,
                        numberElements_,elements_);
      }
      int position = columnList_.first(whichColumn);
      if (position>=0) {
        link.setPosition(position);
        link.setRow(elements_[position].row);
        assert (whichColumn==(int) elements_[position].column);
        link.setValue(elements_[position].value);
      }
    }
  }
  return link;
}
/* Returns last element in given column - index is -1 if none.
   Index is given by .index and value by .value
*/
CoinModelLink 
CoinModel::lastInColumn(int whichColumn) const
{
  CoinModelLink link;
  if (whichColumn>=0&&whichColumn<numberColumns_) {
    link.setColumn(whichColumn);
    link.setOnRow(false);
    if (type_==1) {
      assert (start_);
      int position = start_[whichColumn+1]-1;
      if (position>=start_[whichColumn]) {
        link.setPosition(position);
        link.setRow(elements_[position].row);
        assert (whichColumn==(int) elements_[position].column);
        link.setValue(elements_[position].value);
      }
    } else {
      if ((links_&2)==0) {
        // Create list
        assert (!columnList_.numberMajor());
        columnList_.create(maximumColumns_,maximumElements_,numberColumns_,numberRows_,0,
                        numberElements_,elements_);
      }
      int position = columnList_.last(whichColumn);
      if (position>=0) {
        link.setPosition(position);
        link.setRow(elements_[position].row);
        assert (whichColumn==(int) elements_[position].column);
        link.setValue(elements_[position].value);
      }
    }
  }
  return link;
}
/* Returns next element in current row or column - index is -1 if none.
   Index is given by .index and value by .value.
   User could also tell because input.next would be NULL
*/
CoinModelLink 
CoinModel::next(CoinModelLink & current) const
{
  CoinModelLink link=current;
  int position = current.position();
  if (position>=0) {
    if (current.onRow()) {
      // Doing by row
      int whichRow = current.row();
      if (type_==0) {
        assert (start_);
        position++;
        if (position<start_[whichRow+1]) {
          link.setPosition(position);
          link.setColumn(elements_[position].column);
          assert (whichRow==(int) elements_[position].row);
          link.setValue(elements_[position].value);
        } else {
          // signal end
          link.setPosition(-1);
          link.setColumn(-1);
          link.setValue(0.0);
        }
      } else {
        assert ((links_&1)!=0);
        position = rowList_.next()[position];
        if (position>=0) {
          link.setPosition(position);
          link.setColumn(elements_[position].column);
          assert (whichRow==(int) elements_[position].row);
          link.setValue(elements_[position].value);
        } else {
          // signal end
          link.setPosition(-1);
          link.setColumn(-1);
          link.setValue(0.0);
        }
      }
    } else {
      // Doing by column
      int whichColumn = current.column();
      if (type_==1) {
        assert (start_);
        position++;
        if (position<start_[whichColumn+1]) {
          link.setPosition(position);
          link.setRow(elements_[position].row);
          assert (whichColumn==(int) elements_[position].column);
          link.setValue(elements_[position].value);
        } else {
          // signal end
          link.setPosition(-1);
          link.setColumn(-1);
          link.setValue(0.0);
        }
      } else {
        assert ((links_&2)!=0);
        position = columnList_.next()[position];
        if (position>=0) {
          link.setPosition(position);
          link.setRow(elements_[position].row);
          assert (whichColumn==(int) elements_[position].column);
          link.setValue(elements_[position].value);
        } else {
          // signal end
          link.setPosition(-1);
          link.setColumn(-1);
          link.setValue(0.0);
        }
      }
    }
  }
  return link;
}
/* Returns previous element in current row or column - index is -1 if none.
   Index is given by .index and value by .value.
   User could also tell because input.previous would be NULL
*/
CoinModelLink 
CoinModel::previous(CoinModelLink & current) const
{
  CoinModelLink link=current;
  int position = current.position();
  if (position>=0) {
    if (current.onRow()) {
      // Doing by row
      int whichRow = current.row();
      if (type_==0) {
        assert (start_);
        position--;
        if (position>=start_[whichRow]) {
          link.setPosition(position);
          link.setColumn(elements_[position].column);
          assert (whichRow==(int) elements_[position].row);
          link.setValue(elements_[position].value);
        } else {
          // signal end
          link.setPosition(-1);
          link.setColumn(-1);
          link.setValue(0.0);
        }
      } else {
        assert ((links_&1)!=0);
        position = rowList_.previous()[position];
        if (position>=0) {
          link.setPosition(position);
          link.setColumn(elements_[position].column);
          assert (whichRow==(int) elements_[position].row);
          link.setValue(elements_[position].value);
        } else {
          // signal end
          link.setPosition(-1);
          link.setColumn(-1);
          link.setValue(0.0);
        }
      }
    } else {
      // Doing by column
      int whichColumn = current.column();
      if (type_==1) {
        assert (start_);
        position--;
        if (position>=start_[whichColumn]) {
          link.setPosition(position);
          link.setRow(elements_[position].row);
          assert (whichColumn==(int) elements_[position].column);
          link.setValue(elements_[position].value);
        } else {
          // signal end
          link.setPosition(-1);
          link.setColumn(-1);
          link.setValue(0.0);
        }
      } else {
        assert ((links_&2)!=0);
        position = columnList_.previous()[position];
        if (position>=0) {
          link.setPosition(position);
          link.setRow(elements_[position].row);
          assert (whichColumn==(int) elements_[position].column);
          link.setValue(elements_[position].value);
        } else {
          // signal end
          link.setPosition(-1);
          link.setColumn(-1);
          link.setValue(0.0);
        }
      }
    }
  }
  return link;
}
/* Returns first element in given quadratic column - index is -1 if none.
   Index is given by .index and value by .value
*/
CoinModelLink 
CoinModel::firstInQuadraticColumn(int whichColumn) const
{
  printf("not written yet\n");
  abort();
  CoinModelLink x;
  return x;
}
/* Returns last element in given quadratic column - index is -1 if none.
   Index is given by .index and value by .value
*/
CoinModelLink 
CoinModel::lastInQuadraticColumn(int whichColumn) const
{
  printf("not written yet\n");
  abort();
  CoinModelLink x;
  return x;
}
/* Gets rowLower (if row does not exist then -COIN_DBL_MAX)
 */
double  
CoinModel::getRowLower(int whichRow) const
{
  assert (whichRow>=0);
  if (whichRow<numberRows_&&rowLower_)
    return rowLower_[whichRow];
  else
    return -COIN_DBL_MAX;
}
/* Gets rowUpper (if row does not exist then +COIN_DBL_MAX)
 */
double  
CoinModel::getRowUpper(int whichRow) const
{
  assert (whichRow>=0);
  if (whichRow<numberRows_&&rowUpper_)
    return rowUpper_[whichRow];
  else
    return COIN_DBL_MAX;
}
/* Gets name (if row does not exist then "")
 */
const char * 
CoinModel::getRowName(int whichRow) const
{
  assert (whichRow>=0);
  if (whichRow<rowName_.numberItems())
    return rowName_.name(whichRow);
  else
    return NULL;
}
/* Gets columnLower (if column does not exist then 0.0)
 */
double  
CoinModel::getColumnLower(int whichColumn) const
{
  assert (whichColumn>=0);
  if (whichColumn<numberColumns_&&columnLower_)
    return columnLower_[whichColumn];
  else
    return 0.0;
}
/* Gets columnUpper (if column does not exist then COIN_DBL_MAX)
 */
double  
CoinModel::getColumnUpper(int whichColumn) const
{
  assert (whichColumn>=0);
  if (whichColumn<numberColumns_&&columnUpper_)
    return columnUpper_[whichColumn];
  else
    return COIN_DBL_MAX;
}
/* Gets columnObjective (if column does not exist then 0.0)
 */
double  
CoinModel::getColumnObjective(int whichColumn) const
{
  assert (whichColumn>=0);
  if (whichColumn<numberColumns_&&objective_)
    return objective_[whichColumn];
  else
    return 0.0;
}
/* Gets name (if column does not exist then "")
 */
const char * 
CoinModel::getColumnName(int whichColumn) const
{
  assert (whichColumn>=0);
  if (whichColumn<columnName_.numberItems())
    return columnName_.name(whichColumn);
  else
    return NULL;
}
/* Gets if integer (if column does not exist then false)
 */
bool 
CoinModel::getColumnIsInteger(int whichColumn) const
{
  assert (whichColumn>=0);
  if (whichColumn<numberColumns_&&integerType_)
    return integerType_[whichColumn]!=0;
  else
    return false;
}
// Row index from row name (-1 if no names or no match)
int 
CoinModel::row(const char * rowName) const
{
  printf("not written yet\n");
  abort();
  return 0;
}
// Column index from column name (-1 if no names or no match)
int 
CoinModel::column(const char * columnName) const
{
  printf("not written yet\n");
  abort();
  return 0;
}
// Resize
void 
CoinModel::resize(int maximumRows, int maximumColumns, int maximumElements)
{
  maximumElements = CoinMax(maximumElements,maximumElements_);
  if (type_==0||type_==2) {
    // need to redo row stuff
    maximumRows = CoinMax(maximumRows,numberRows_);
    if (maximumRows>maximumRows_) {
      double * tempArray;
      tempArray = new double[maximumRows];
      memcpy(tempArray,rowLower_,numberRows_*sizeof(double));
      delete [] rowLower_;
      rowLower_=tempArray;
      tempArray = new double[maximumRows];
      memcpy(tempArray,rowUpper_,numberRows_*sizeof(double));
      delete [] rowUpper_;
      rowUpper_=tempArray;
      int * tempArray2;
      tempArray2 = new int[maximumRows];
      memcpy(tempArray2,rowType_,numberRows_*sizeof(int));
      delete [] rowType_;
      rowType_=tempArray2;
      if (rowName_.numberItems()) {
        // resize hash
        rowName_.resize(maximumRows);
      }
      // If we have links we need to resize
      if ((links_&1)!=0) {
        rowList_.resize(maximumRows,maximumElements);
      }
      // If we have start then we need to resize that
      if (type_==0) {
        int * tempArray2;
        tempArray2 = new int[maximumRows+1];
        if (start_) {
          memcpy(tempArray2,start_,(numberRows_+1)*sizeof(int));
          delete [] start_;
        } else {
          tempArray2[0]=0;
        }
        start_=tempArray2;
      }
      maximumRows_=maximumRows;
      // Fill
      int save=numberRows_-1;
      numberRows_=0;
      fillRows(save,true);
    }
  }
  if (type_==1||type_==2) {
    // need to redo column stuff
    maximumColumns = CoinMax(maximumColumns,numberColumns_);
    if (maximumColumns>maximumColumns_) {
      double * tempArray;
      tempArray = new double[maximumColumns];
      memcpy(tempArray,columnLower_,numberColumns_*sizeof(double));
      delete [] columnLower_;
      columnLower_=tempArray;
      tempArray = new double[maximumColumns];
      memcpy(tempArray,columnUpper_,numberColumns_*sizeof(double));
      delete [] columnUpper_;
      columnUpper_=tempArray;
      tempArray = new double[maximumColumns];
      memcpy(tempArray,objective_,numberColumns_*sizeof(double));
      delete [] objective_;
      objective_=tempArray;
      int * tempArray2;
      tempArray2 = new int[maximumColumns];
      memcpy(tempArray2,columnType_,numberColumns_*sizeof(int));
      delete [] columnType_;
      columnType_=tempArray2;
      tempArray2 = new int[maximumColumns];
      memcpy(tempArray2,integerType_,numberColumns_*sizeof(int));
      delete [] integerType_;
      integerType_=tempArray2;
      if (columnName_.numberItems()) {
        // resize hash
        columnName_.resize(maximumColumns);
      }
      // If we have links we need to resize
      if ((links_&1)!=0) {
        columnList_.resize(maximumColumns,maximumElements);
      }
      // If we have start then we need to resize that
      if (type_==1) {
        int * tempArray2;
        tempArray2 = new int[maximumColumns+1];
        if (start_) {
          memcpy(tempArray2,start_,(numberColumns_+1)*sizeof(int));
          delete [] start_;
        } else {
          tempArray2[0]=0;
        }
        start_=tempArray2;
      }
      maximumColumns_=maximumColumns;
      // Fill
      int save=numberColumns_-1;
      numberColumns_=0;
      fillColumns(save,true);
    }
  }
  if (maximumElements>maximumElements_) {
    CoinModelTriple * tempArray = new CoinModelTriple[maximumElements];
    memcpy(tempArray,elements_,numberElements_*sizeof(CoinModelTriple));
    delete [] elements_;
    elements_=tempArray;
    if (hashElements_.numberItems())
      hashElements_.resize(maximumElements,elements_);
    maximumElements_=maximumElements;
  }
}
void
CoinModel::fillRows(int whichRow, bool forceCreation)
{
  if (forceCreation) {
    if (type_==-1) {
      // initial
      type_=0;
      resize(CoinMax(100,whichRow+1),0,1000);
    } else if (type_==1) {
      type_=2;
    }
    if (!rowLower_) {
      // need to set all
      whichRow = numberRows_-1;
      numberRows_=0;
      resize(CoinMax(100,whichRow+1),0,0);
    }
    if (whichRow>=maximumRows_) {
      resize(CoinMax((3*maximumRows_)/2,whichRow+1),0,0);
    }
  }
  if (whichRow>=numberRows_&&rowLower_) {
    // Need to fill
    int i;
    for ( i=numberRows_;i<=whichRow;i++) {
      rowLower_[i]=0.0;
      rowUpper_[i]=COIN_DBL_MAX;
      rowType_[i]=0;
    }
    if (rowName_.numberItems()) {
      // Do we need to do anything?
    }
  }
  numberRows_=whichRow+1;
}
void
CoinModel::fillColumns(int whichColumn,bool forceCreation)
{
  if (forceCreation) {
    if (type_==-1) {
      // initial
      type_=1;
      resize(0,CoinMax(100,whichColumn+1),1000);
    } else if (type_==0) {
      type_=2;
    }
    if (!objective_) {
      // need to set all
      whichColumn = numberColumns_-1;
      numberColumns_=0;
      resize(0,CoinMax(100,whichColumn+1),0);
    }
    if (whichColumn>=maximumColumns_) {
      resize(0,CoinMax((3*maximumColumns_)/2,whichColumn+1),0);
    }
  }
  if (whichColumn>=numberColumns_&&objective_) {
    // Need to fill
    int i;
    for ( i=numberColumns_;i<=whichColumn;i++) {
      columnLower_[i]=0.0;
      columnUpper_[i]=COIN_DBL_MAX;
      objective_[i]=0.0;
      integerType_[i]=0;
      columnType_[i]=0;
    }
    if (columnName_.numberItems()) {
      // Do we need to do anything?
    }
  }
  numberColumns_=whichColumn+1;
}
