// Last edit: 2/10/07
//
// Name:     CoinLpIO.cpp; Support for Lp files
// Author:   Francois Margot
//           Tepper School of Business
//           Carnegie Mellon University, Pittsburgh, PA 15213
// Date:     12/28/03
//-----------------------------------------------------------------------------
// Copyright (C) 2003, Francois Margot, International Business Machines
// Corporation and others.  All Rights Reserved.

#include <cfloat>
#include <cctype>
#include <cassert>

#include "CoinHelperFunctions.hpp"
#include "CoinPackedMatrix.hpp"
#include "CoinLpIO.hpp"

using namespace std;

//#define LPIO_DEBUG

/************************************************************************/

CoinLpIO::CoinLpIO() :
  problemName_(strdup("")),
  numberRows_(0),
  numberColumns_(0),
  numberElements_(0),
  matrixByColumn_(NULL),
  matrixByRow_(NULL),
  rowlower_(NULL),
  rowupper_(NULL),
  collower_(NULL),
  colupper_(NULL),
  rhs_(NULL),
  rowrange_(NULL),
  rowsense_(NULL),
  objective_(NULL),
  objectiveOffset_(0),
  integerType_(NULL),
  fileName_(NULL),
  infinity_(DBL_MAX),
  epsilon_(1e-5),
  numberAcross_(10),
  decimals_(5),
  objName_(NULL)
{
  card_previous_names_[0] = 0;
  card_previous_names_[1] = 0;
  previous_names_[0] = NULL;
  previous_names_[1] = NULL;

  maxHash_[0]=0;
  numberHash_[0]=0;
  hash_[0] = NULL;
  names_[0] = NULL;
  maxHash_[1] = 0;
  numberHash_[1] = 0;
  hash_[1] = NULL;
  names_[1] = NULL;
}

/************************************************************************/
CoinLpIO::~CoinLpIO() {
  stopHash(0);
  stopHash(1);
  freeAll();
}

/************************************************************************/
void
CoinLpIO::freePreviousNames(const int section) {

  int j;

  if(previous_names_[section] != NULL) {
    for(j=0; j<card_previous_names_[section]; j++) {
      free(previous_names_[section][j]);
    }
    free(previous_names_[section]);
  }
  previous_names_[section] = NULL;
  card_previous_names_[section] = 0;
} /* freePreviousNames */

/************************************************************************/
void
CoinLpIO::freeAll() {

  delete matrixByColumn_;
  delete matrixByRow_;
  free(rowupper_);
  rowupper_ = NULL;
  free(rowlower_);
  rowlower_ = NULL;
  free(colupper_);
  colupper_ = NULL;
  free(collower_);
  collower_ = NULL;
  free(rhs_);
  rhs_ = NULL;
  free(rowrange_);
  rowrange_ = NULL;
  free(rowsense_);
  rowsense_ = NULL;
  free(objective_);
  objective_ = NULL;
  free(integerType_);
  integerType_ = NULL;
  free(problemName_);
  problemName_ = NULL;
  free(fileName_);
  fileName_ = NULL;

  freePreviousNames(0);
  freePreviousNames(1);
}

/*************************************************************************/
const char * CoinLpIO::getProblemName() const
{
  return problemName_;
}

void
CoinLpIO::setProblemName (const char *name)
{
  free(problemName_) ;
  problemName_ = strdup(name);
}

/*************************************************************************/
int CoinLpIO::getNumCols() const
{
  return numberColumns_;
}

/*************************************************************************/
int CoinLpIO::getNumRows() const
{
  return numberRows_;
}

/*************************************************************************/
int CoinLpIO::getNumElements() const
{
  return numberElements_;
}

/*************************************************************************/
const double * CoinLpIO::getColLower() const
{
  return collower_;
}

/*************************************************************************/
const double * CoinLpIO::getColUpper() const
{
  return colupper_;
}

/*************************************************************************/
const double * CoinLpIO::getRowLower() const
{
  return rowlower_;
}

/*************************************************************************/
const double * CoinLpIO::getRowUpper() const
{
  return rowupper_;
}

/*************************************************************************/
/** A quick inlined function to convert from lb/ub style constraint
    definition to sense/rhs/range style */
inline void
CoinLpIO::convertBoundToSense(const double lower, const double upper,
			      char& sense, double& right,
			      double& range) const
{
  range = 0.0;
  if (lower > -infinity_) {
    if (upper < infinity_) {
      right = upper;
      if (upper==lower) {
        sense = 'E';
      } else {
        sense = 'R';
        range = upper - lower;
      }
    } else {
      sense = 'G';
      right = lower;
    }
  } else {
    if (upper < infinity_) {
      sense = 'L';
      right = upper;
    } else {
      sense = 'N';
      right = 0.0;
    }
  }
}

/*************************************************************************/
 const char * CoinLpIO::getRowSense() const
{
  if(rowsense_  == NULL) {
    int nr=numberRows_;
    rowsense_ = (char *) malloc(nr*sizeof(char));
    
    double dum1,dum2;
    int i;
    for(i=0; i<nr; i++) {
      convertBoundToSense(rowlower_[i],rowupper_[i],rowsense_[i],dum1,dum2);
    }
  }
  return rowsense_;
}

/*************************************************************************/
const double * CoinLpIO::getRightHandSide() const
{
  if(rhs_==NULL) {
    int nr=numberRows_;
    rhs_ = (double *) malloc(nr*sizeof(double));

    char dum1;
    double dum2;
    int i;
    for (i=0; i<nr; i++) {
      convertBoundToSense(rowlower_[i],rowupper_[i],dum1,rhs_[i],dum2);
    }
  }
  return rhs_;
}

/*************************************************************************/
const double * CoinLpIO::getRowRange() const
{
  if (rowrange_ == NULL) {
    int nr=numberRows_;
    rowrange_ = (double *) malloc(nr*sizeof(double));
    std::fill(rowrange_,rowrange_+nr,0.0);

    char dum1;
    double dum2;
    int i;
    for (i=0; i<nr; i++) {
      convertBoundToSense(rowlower_[i],rowupper_[i],dum1,dum2,rowrange_[i]);
    }
  }
  return rowrange_;
}

/*************************************************************************/
const double * CoinLpIO::getObjCoefficients() const
{
  return objective_;
}
 
/*************************************************************************/
const CoinPackedMatrix * CoinLpIO::getMatrixByRow() const
{
  return matrixByRow_;
}

/*************************************************************************/
const CoinPackedMatrix * CoinLpIO::getMatrixByCol() const
{
  if (matrixByColumn_ == NULL && matrixByRow_) {
    matrixByColumn_ = new CoinPackedMatrix(*matrixByRow_);
    matrixByColumn_->reverseOrdering();
  }
  return matrixByColumn_;
}

/*************************************************************************/
const char * CoinLpIO::getObjName() const
{
  return objName_;
}
 
/*************************************************************************/
void CoinLpIO::checkRowNames() {

  int i, nrow = getNumRows();

  if(numberHash_[0] != nrow+1) {
    setDefaultRowNames();
    printf("### WARNING: CoinLpIO::checkRowNames(): non distinct or missing row names or objective function name.\nNow using default row names.\n");
  }

  char const * const * rowNames = getRowNames();
  const char *rSense = getRowSense();
  char rName[256];

  // Check that row names and objective function name are all distinct, 
  /// even after adding "_low" to ranged constraint names

  for(i=0; i<nrow; i++) {
    if(rSense[i] == 'R') {
      sprintf(rName, "%s_low", rowNames[i]);
      if(findHash(rName, 0) != -1) {
	setDefaultRowNames();
	printf("### WARNING: CoinLpIO::checkRowNames(): ranged constraint %d hasa name %s identical to another constraint name or objective function name.\nUse getPreviousNames() to get the old row names.\nNow using default row names.\n", i, rName);
	break;
      }
    }
  }
} /* checkRowNames */

/*************************************************************************/
void CoinLpIO::checkColNames() {

  int ncol = getNumCols();

  if(numberHash_[1] != ncol) {
    setDefaultColNames();
    printf("### WARNING: CoinLpIO::checkColNames(): non distinct or missing column names.\nNow using default column names.\n");
  }
} /* checkColNames */

/*************************************************************************/
void CoinLpIO::getPreviousRowNames(char const * const * prev, 
				   int *card_prev) const
{
  *card_prev = card_previous_names_[0];
  prev = previous_names_[0];
}
 
/*************************************************************************/
void CoinLpIO::getPreviousColNames(char const * const * prev, 
				   int *card_prev) const
{
  *card_prev = card_previous_names_[1];
  prev = previous_names_[1];
}
 
/*************************************************************************/
char const * const * CoinLpIO::getRowNames() const
{
  return names_[0];
}
 
/*************************************************************************/
char const * const * CoinLpIO::getColNames() const
{
  return names_[1];
}
 
/*************************************************************************/
const char * CoinLpIO::rowName(int index) const {

  if((names_[0] != NULL) && (index >= 0) && (index < numberRows_+1)) {
    return names_[0][index];
  } 
  else {
    return NULL;
  }
}

/*************************************************************************/
const char * CoinLpIO::columnName(int index) const {

  if((names_[1] != NULL) && (index >= 0) && (index < numberColumns_)) {
    return names_[1][index];
  } 
  else {
    return NULL;
  }
}

/*************************************************************************/
int CoinLpIO::rowIndex(const char * name) const {

  if (!hash_[0]) {
    return -1;
  }
  return findHash(name , 0);
}

/*************************************************************************/
int CoinLpIO::columnIndex(const char * name) const {

  if (!hash_[1]) {
    return -1;
  }
  return findHash(name , 1);
}

/************************************************************************/
double CoinLpIO::getInfinity() const
{
  return infinity_;
}

/************************************************************************/
void CoinLpIO::setInfinity(const double value) 
{
  if (value >= 1.020) {
    infinity_ = value;
  } 
  else {
    printf("### ERROR: CoinLpIO::setInfinity(): value: %f\n", value);
    exit(1);
  }
}

/************************************************************************/
double CoinLpIO::getEpsilon() const
{
  return epsilon_;
}

/************************************************************************/
void CoinLpIO::setEpsilon(const double value) 
{
  if (value < 0.1) {
    epsilon_ = value;
  } 
  else {
    printf("### ERROR: CoinLpIO::setEpsilon(): value: %f\n", value);
    exit(1);
  }
}

/************************************************************************/
int CoinLpIO::getNumberAcross() const
{
  return numberAcross_;
}

/************************************************************************/
void CoinLpIO::setNumberAcross(const int value) 
{
  if (value > 0) {
    numberAcross_ = value;
  } 
  else {
    printf("### ERROR: CoinLpIO::setNumberAcross(): value: %d\n", value);
    exit(1);
  }
}

/************************************************************************/
int CoinLpIO::getDecimals() const
{
  return decimals_;
}

/************************************************************************/
void CoinLpIO::setDecimals(const int value) 
{
  if (value > 0) {
    decimals_ = value;
  } 
  else {
    printf("### ERROR: CoinLpIO::setDecimals(): value: %d\n", value);
    exit(1);
  }
}

/************************************************************************/
double CoinLpIO::objectiveOffset() const
{
  return objectiveOffset_;
}

/************************************************************************/
bool CoinLpIO::isInteger(int columnNumber) const
{
  const char * intType = integerType_;
  if (intType == NULL) return false;
  assert (columnNumber >= 0 && columnNumber < numberColumns_);
  if (intType[columnNumber] != 0) return true;
  return false;
}

/************************************************************************/
const char * CoinLpIO::integerColumns() const
{
  return integerType_;
}

/************************************************************************/
void
CoinLpIO::setLpDataWithoutRowAndColNames(
			      const CoinPackedMatrix& m,
			      const double *collb, const double *colub,
			      const double *obj_coeff,
			      const char *is_integer,
			      const double *rowlb, const double *rowub) {

  freeAll();
  problemName_ = strdup("");

  if (m.isColOrdered()) {
    matrixByRow_ = new CoinPackedMatrix();
    matrixByRow_->reverseOrderedCopyOf(m);
  } 
  else {
    matrixByRow_ = new CoinPackedMatrix(m);
  }
  numberColumns_ = matrixByRow_->getNumCols();
  numberRows_ = matrixByRow_->getNumRows();
  
  rowlower_ = (double *) malloc (numberRows_ * sizeof(double));
  rowupper_ = (double *) malloc (numberRows_ * sizeof(double));
  collower_ = (double *) malloc (numberColumns_ * sizeof(double));
  colupper_ = (double *) malloc (numberColumns_ * sizeof(double));
  objective_ = (double *) malloc (numberColumns_ * sizeof(double));
  std::copy(rowlb, rowlb + numberRows_, rowlower_);
  std::copy(rowub, rowub + numberRows_, rowupper_);
  std::copy(collb, collb + numberColumns_, collower_);
  std::copy(colub, colub + numberColumns_, colupper_);
  std::copy(obj_coeff, obj_coeff + numberColumns_, objective_);

  if (is_integer) {
    integerType_ = (char *) malloc (numberColumns_ * sizeof(char));
    std::copy(is_integer, is_integer + numberColumns_, integerType_);
  } 
  else {
    integerType_ = 0;
  }

  if((numberHash_[0] > 0) && (numberHash_[0] != numberRows_+1)) {
    stopHash(0);
  }
  if((numberHash_[1] > 0) && (numberHash_[1] != numberColumns_)) {
    stopHash(1);
  }
} /* SetLpDataWithoutRowAndColNames */

/*************************************************************************/
void CoinLpIO::setDefaultRowNames() {

  int i, nrow = getNumRows();
  char **defaultRowNames = (char **) malloc ((nrow+1) * sizeof(char *));
  char buff[1024];

  for(i=0; i<nrow; i++) {
    sprintf(buff, "cons%d", i);
    defaultRowNames[i] = strdup(buff);
  }
  sprintf(buff, "obj");
  defaultRowNames[nrow] = strdup(buff);

  stopHash(0);
  startHash(defaultRowNames, nrow+1, 0);
  objName_ = strdup("obj");

  for(i=0; i<nrow+1; i++) {
    free(defaultRowNames[i]);
  }
  free(defaultRowNames);

} /* setDefaultRowNames */

/*************************************************************************/
void CoinLpIO::setDefaultColNames() {

  int j, ncol = getNumCols();
  char **defaultColNames = (char **) malloc (ncol * sizeof(char *));
  char buff[256];

  for(j=0; j<ncol; j++) {
    sprintf(buff, "x%d", j);
    defaultColNames[j] = strdup(buff);
  }
  stopHash(1);
  startHash(defaultColNames, ncol, 1);

  for(j=0; j<ncol; j++) {
    free(defaultColNames[j]);
  }
  free(defaultColNames);

} /* setDefaultColNames */

/*************************************************************************/
void CoinLpIO::setLpDataRowAndColNames(char const * const * const rownames,
				       char const * const * const colnames) {

  int nrow = getNumRows();
  int ncol = getNumCols();

  if(rownames != NULL) {
    if(are_invalid_names(rownames, nrow+1, true)) {
      setDefaultRowNames();
      printf("### WARNING: CoinLpIO::setLpDataRowAndColNames(): Invalid row names\nUse getPreviousNames() to get the old row names.\nNow using default row names.\n");
    } 
    else {
      stopHash(0);
      startHash(rownames, nrow+1, 0);
      objName_ = strdup(rownames[nrow]);
      checkRowNames();
    }
  }
  else {
    if(objName_ == NULL) {
      objName_ = strdup("obj");      
    }
  }

  if(colnames != NULL) {
    if(are_invalid_names(colnames, ncol, false)) {
      setDefaultColNames();
      printf("### WARNING: CoinLpIO::setLpDataRowAndColNames(): Invalid column names\nNow using default row names.\n");
    } 
    else {
      stopHash(1);
      startHash(colnames, ncol, 1);
      checkColNames();
    }
  }
} /* setLpDataColAndRowNames */

/************************************************************************/
void
CoinLpIO::out_coeff(FILE *fp, const double v, const int print_1) const {

  double lp_eps = getEpsilon();

  if(!print_1) {
    if(fabs(v-1) < lp_eps) {
      return;
    }
    if(fabs(v+1) < lp_eps) {
      fprintf(fp, " -");
      return;
    }
  }

  double frac = v - floor(v);

  if(frac < lp_eps) {
    fprintf(fp, " %.0f", floor(v));
  }
  else {
    if(frac > 1 - lp_eps) {
      fprintf(fp, " %.0f", floor(v+0.5));
    }
    else {
      int decimals = getDecimals();
      char form[15];
      sprintf(form, " %%.%df", decimals);
      fprintf(fp, form, v);
    }
  }
} /* out_coeff */

/************************************************************************/
int
CoinLpIO::writeLp(const char *filename, const double epsilon,
		  const int numberAcross, const int decimals,
		  const bool useRowNames) {

   FILE *fp = NULL;
   fp = fopen(filename,"w");
   if (!fp) {
     printf("### ERROR: in CoinLpIO::writeLP(): unable to open file %s\n",
	    filename);
     exit(1);
   }
   int nerr = writeLp(fp, epsilon, numberAcross, decimals, useRowNames);
   fclose(fp);
   return(nerr);
}

/************************************************************************/
int
CoinLpIO::writeLp(FILE *fp, const double epsilon,
		  const int numberAcross, const int decimals,
		  const bool useRowNames) {

  setEpsilon(epsilon);
  setNumberAcross(numberAcross);
  setDecimals(decimals);
  return writeLp(fp, useRowNames);
}

/************************************************************************/
int
CoinLpIO::writeLp(const char *filename, const bool useRowNames)
{
   FILE *fp = NULL;
   fp = fopen(filename,"w");
   if (!fp) {
     printf("### ERROR: in CoinLpIO::writeLP(): unable to open file %s\n",
	    filename);
     exit(1);
   }
   int nerr = writeLp(fp, useRowNames);
   fclose(fp);
   return(nerr);
}

/************************************************************************/
int
CoinLpIO::writeLp(FILE *fp, const bool useRowNames)
{
   double lp_eps = getEpsilon();
   double lp_inf = getInfinity();
   int numberAcross = getNumberAcross();

   int i, j, cnt_print, loc_row_names = 0, loc_col_names = 0;
   char **prowNames = NULL, **pcolNames = NULL;

   const int *indices = matrixByRow_->getIndices();
   const double *elements  = matrixByRow_->getElements();
   int ncol = getNumCols();
   int nrow = getNumRows();
   const double *collow = getColLower();
   const double *colup = getColUpper();
   const double *rowlow = getRowLower();
   const double *rowup = getRowUpper();
   const double *obj = getObjCoefficients();
   const char *integerType = integerColumns();
   char const * const * rowNames = getRowNames();
   char const * const * colNames = getColNames();
   
   char buff[256];

   if(rowNames == NULL) {
     loc_row_names = 1;
     prowNames = (char **) malloc ((nrow+1) * sizeof(char *));

     for (j=0; j<nrow; j++) {
       sprintf(buff, "cons%d", j);
       prowNames[j] = strdup(buff);
     }
     prowNames[nrow] = strdup("obj");
     rowNames = prowNames;
   }

   if(colNames == NULL) {
     loc_col_names = 1;
     pcolNames = (char **) malloc (ncol * sizeof(char *));

     for (j=0; j<ncol; j++) {
       sprintf(buff, "x%d", j);
       pcolNames[j] = strdup(buff);
     }
     colNames = pcolNames;
   }

#ifdef LPIO_DEBUG
   printf("CoinLpIO::writeLp(): numberRows: %d numberColumns: %d\n", 
	  nrow, ncol);
#endif
 
   fprintf(fp, "\\Problem name: %s\n\n", getProblemName());
   fprintf(fp, "Minimize\n");

   if(useRowNames) {
     fprintf(fp, "%s:", objName_);
   }

   cnt_print = 0;
   for(j=0; j<ncol; j++) {
     if((cnt_print > 0) && (objective_[j] > lp_eps)) {
       fprintf(fp, " +");
     }
     if(fabs(obj[j]) > lp_eps) {
       out_coeff(fp, obj[j], 0);
       fprintf(fp, " %s", colNames[j]);
       cnt_print++;
       if(cnt_print % numberAcross == 0) {
	 fprintf(fp, "\n");
       }
     }
   }
 
   if((cnt_print > 0) && (objectiveOffset_ > lp_eps)) {
     fprintf(fp, " +");
   }
   if(fabs(objectiveOffset_) > lp_eps) {
     out_coeff(fp, objectiveOffset_, 1);
     cnt_print++;
   }

   if(cnt_print % numberAcross != 0) {
     fprintf(fp, "\n");
   }
   
   fprintf(fp, "Subject To\n");
   
   int cnt_out_rows = 0;

   for(i=0; i<nrow; i++) {
     cnt_print = 0;
     
     if(useRowNames) {
       fprintf(fp, "%s: ", rowNames[i]);
     }
     cnt_out_rows++;

     for(j=matrixByRow_->getVectorFirst(i); 
	 j<matrixByRow_->getVectorLast(i); j++) {
       if((cnt_print > 0) && (elements[j] > lp_eps)) {
	 fprintf(fp, " +");
       }
       if(fabs(elements[j]) > lp_eps) {
	 out_coeff(fp, elements[j], 0);
	 fprintf(fp, " %s", colNames[indices[j]]);
	 cnt_print++;
	 if(cnt_print % numberAcross == 0) {
	   fprintf(fp, "\n");
	 }
       }
     }

     if(rowup[i] - rowlow[i] < lp_eps) {
          fprintf(fp, " =");
	  out_coeff(fp, rowlow[i], 1);
	  fprintf(fp, "\n");
     }
     else {
       if(rowup[i] < lp_inf) {
	 fprintf(fp, " <=");
	 out_coeff(fp, rowup[i], 1);	 
	 fprintf(fp, "\n");

	 if(rowlower_[i] > -lp_inf) {

	   cnt_print = 0;

	   if(useRowNames) {
	     fprintf(fp, "%s_low:", rowNames[i]);
	   }
	   cnt_out_rows++;
	   
	   for(j=matrixByRow_->getVectorFirst(i); 
	       j<matrixByRow_->getVectorLast(i); j++) {
	     if((cnt_print>0) && (elements[j] > lp_eps)) {
	       fprintf(fp, " +");
	     }
	     if(fabs(elements[j]) > lp_eps) {
	       out_coeff(fp, elements[j], 0);
	       fprintf(fp, " %s", colNames[indices[j]]);
	       cnt_print++;
	       if(cnt_print % numberAcross == 0) {
		 fprintf(fp, "\n");
	       }
	     }
	   }
	   fprintf(fp, " >=");
	   out_coeff(fp, rowlow[i], 1);
	   fprintf(fp, "\n");
	 }

       }
       else {
	 fprintf(fp, " >=");
	 out_coeff(fp, rowlow[i], 1);	 
	 fprintf(fp, "\n");
       }
     }
   }

#ifdef LPIO_DEBUG
   printf("CoinLpIO::writeLp(): Done with constraints\n");
#endif

   fprintf(fp, "Bounds\n");
   
   for(j=0; j<ncol; j++) {
     if((collow[j] > -lp_inf) && (colup[j] < lp_inf)) {
       out_coeff(fp, collow[j], 1);
       fprintf(fp, " <= %s <=", colNames[j]); 
       out_coeff(fp, colup[j], 1);
       fprintf(fp, "\n");
     }
     if((collow[j] == -lp_inf) && (colup[j] < lp_inf)) {
       fprintf(fp, "%s <=", colNames[j]);
       out_coeff(fp, colup[j], 1);
       fprintf(fp, "\n");
     }
     if((collow[j] > -lp_inf) && (colup[j] == lp_inf)) {
       if(fabs(collow[j]) > lp_eps) { 
	 out_coeff(fp, collow[j], 1);
	 fprintf(fp, " <= %s\n", colNames[j]);
       }
     }
     if(collow[j] == -lp_inf) {
       fprintf(fp, " %s Free\n", colNames[j]); 
     }
   }

#ifdef LPIO_DEBUG
   printf("CoinLpIO::writeLp(): Done with bounds\n");
#endif

   if(integerType != NULL) {
     int first_int = 1;
     cnt_print = 0;
     for(j=0; j<ncol; j++) {
       if(integerType[j] == 1) {

	 if(first_int) {
	   fprintf(fp, "Integers\n");
	   first_int = 0;
	 }

	 fprintf(fp, "%s ", colNames[j]);
	 cnt_print++;
	 if(cnt_print % numberAcross == 0) {
	   fprintf(fp, "\n");
	 }
       }
     }

     if(cnt_print % numberAcross != 0) {
       fprintf(fp, "\n");
     }
   }

#ifdef LPIO_DEBUG
   printf("CoinLpIO::writeLp(): Done with integers\n");
#endif

   fprintf(fp, "End\n");

   if(loc_row_names) {
     for(j=0; j<nrow+1; j++) {
       free(prowNames[j]);
     }
     free(prowNames);
   }

   if(loc_col_names) {
     for(j=0; j<ncol; j++) {
       free(pcolNames[j]);
     }
     free(pcolNames);
   }
   return 0;
} /* writeLp */

/*************************************************************************/
int 
CoinLpIO::find_obj(FILE *fp) const {

  char buff[1024];

  sprintf(buff, "aa");
  unsigned lbuff = strlen(buff);

  while(((lbuff != 8) || (CoinStrNCaseCmp(buff, "minimize", 8) != 0)) &&
	((lbuff != 3) || (CoinStrNCaseCmp(buff, "min", 3) != 0)) &&
	((lbuff != 8) || (CoinStrNCaseCmp(buff, "maximize", 8) != 0)) &&
	((lbuff != 3) || (CoinStrNCaseCmp(buff, "max", 3) != 0))) {

    scan_next(buff, fp);
    lbuff = strlen(buff);

    if(feof(fp)) {
      printf("### ERROR: CoinLpIO: find_obj(): Unable to locate objective function\n");
      exit(1);
    }
  }

  if(((lbuff == 8) && (CoinStrNCaseCmp(buff, "minimize", 8) == 0)) ||
     ((lbuff == 3) && (CoinStrNCaseCmp(buff, "min", 3) == 0))) {
    return(1);
  }
  return(-1);
} /* find_obj */

/*************************************************************************/
int
CoinLpIO::is_subject_to(const char *buff) const {

  unsigned lbuff = strlen(buff);

  if(((lbuff == 4) && (CoinStrNCaseCmp(buff, "s.t.", 4) == 0)) ||
     ((lbuff == 3) && (CoinStrNCaseCmp(buff, "st.", 3) == 0)) ||
     ((lbuff == 2) && (CoinStrNCaseCmp(buff, "st", 2) == 0))) {
    return(1);
  }
  if((lbuff == 7) && (CoinStrNCaseCmp(buff, "subject", 7) == 0)) {
    return(2);
  }
  return(0);
} /* is_subject_to */

/*************************************************************************/
int 
CoinLpIO::first_is_number(const char *buff) const {

  int pos;
  char str_num[] = "1234567890";

  pos = strcspn (buff, str_num);
  if(pos == 0) {
    return(1);
  }
  return(0);
} /* first_is_number */

/*************************************************************************/
int 
CoinLpIO::is_sense(const char *buff) const {

  int pos;
  char str_sense[] = "<>=";

  pos = strcspn(buff, str_sense);
  if(pos == 0) {
    if(strcmp(buff, "<=") == 0) {
      return(0);
    }
    if(strcmp(buff, "=") == 0) {
      return(1);
    }
    if(strcmp(buff, ">=") == 0) {
      return(2);
    }
    
    printf("### ERROR: CoinLpIO: is_sense(): string: %s \n", buff);
  }
  return(-1);
} /* is_sense */

/*************************************************************************/
int 
CoinLpIO::is_free(const char *buff) const {

  unsigned lbuff = strlen(buff);

  if((lbuff == 4) && (CoinStrNCaseCmp(buff, "free", 4) == 0)) {
    return(1);
  }
  return(0);
} /* is_free */

/*************************************************************************/
int 
CoinLpIO::is_comment(const char *buff) const {

  if((buff[0] == '/') || (buff[0] == '\\')) {
    return(1);
  }
  return(0);
} /* is_comment */

/*************************************************************************/
void
CoinLpIO::skip_comment(char *buff, FILE *fp) const {

  while(strcspn(buff, "\n") == strlen(buff)) { // end of line not read yet
    if(feof(fp)) {
      printf("### ERROR: CoinLpIO::skip_comment(): end of file reached while skipping comment\n");
      exit(1);
    }
    if(ferror(fp)) {
      printf("### ERROR: CoinLpIO::skip_comment(): error while skipping comment\n");
      exit(1);
    }
    fgets(buff, sizeof(buff), fp);    
  } 
} /* skip_comment */

/*************************************************************************/
void
CoinLpIO::scan_next(char *buff, FILE *fp) const {

  fscanf(fp, "%s", buff);
  while(is_comment(buff)) {
    skip_comment(buff, fp);
    fscanf(fp, "%s", buff);
  }

#ifdef LPIO_DEBUG
  printf("CoinLpIO::scan_next: (%s)\n", buff);
#endif

} /* scan_next */

/*************************************************************************/
int 
CoinLpIO::is_invalid_name(const char *name, 
			  const bool ranged) const {

  unsigned pos, lname, valid_lname = 100;
  char str_valid[] = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ\"!#$%&(),.;?@_'`{}~";

  if(ranged) {
    valid_lname -= 4; // will add "_low" when writing the Lp file
  }

  if(name == NULL) {
    lname = 0;
  }
  else {
    lname = strlen(name);
  }
  if(lname < 1) {
    printf("### WARNING: CoinLpIO::is_invalid_name(): Name is empty\n");
    return(5);
  }
  if(lname > valid_lname) {
    printf("### WARNING: CoinLpIO::is_invalid_name(): Name %s is too long\n", 
	   name);
    return(1);
  }
  if(first_is_number(name)) {
    printf("### WARNING: CoinLpIO::is_invalid_name(): Name %s should not start with a number\n", name);
    return(2);
  }
  pos = strspn(name, str_valid);
  if(pos != lname) {
    printf("### WARNING: CoinLpIO::is_invalid_name(): Name %s contains illegal character '%c'\n", name, name[pos]);
    return(3);
  }

  if((is_keyword(name)) || (is_free(name))) {
    return(4);
  }

  return(0);
} /* is_invalid_name */

/*************************************************************************/
int 
CoinLpIO::are_invalid_names(char const * const * const vnames, 
			    const int card_vnames, 
			    const bool check_ranged) const {

  int i, invalid = 0, flag, nrows = getNumRows();
  bool is_ranged = 0;
  const char * rSense = getRowSense();

  if((check_ranged) && (card_vnames != nrows+1)) {
    printf("### ERROR: CoinLpIO::are_invalid_names(): card_vnames: %d   number of rows: %d\n", card_vnames, getNumRows());
    exit(1);
  }

  for(i=0; i<card_vnames; i++) {

    if((check_ranged) && (i < nrows) && (rSense[i] == 'R')) {
      is_ranged = true;
    }
    else {
      is_ranged = false;
    }
    flag = is_invalid_name(vnames[i], is_ranged);
    if(flag) {
      printf("### WARNING: CoinLpIO::are_invalid_names(): invalid name: vnames[%d]: %s\n",
	     i, vnames[i]);
      invalid = flag;
    }
  }
  return(invalid);
} /* are_invalid_names */

/*************************************************************************/
int 
CoinLpIO::read_monom_obj(FILE *fp, double *coeff, char **name, int *cnt, 
						 char **obj_name) {

  double mult;
  char buff[1024] = "aa", loc_name[1024], *start;
  int read_st = 0;

  scan_next(buff, fp);

  if(feof(fp)) {
    printf("### ERROR: CoinLpIO:  read_monom_obj(): Unable to read objective function\n");
    exit(1);
  }

  if(buff[strlen(buff)-1] == ':') {
    buff[strlen(buff)-1] = '\0';

#ifdef LPIO_DEBUG
    printf("CoinLpIO: read_monom_obj(): obj_name: %s\n", buff);
#endif

    *obj_name = strdup(buff);
    return(0);
  }


  read_st = is_subject_to(buff);

#ifdef LPIO_DEBUG
  printf("read_monom_obj: first buff: (%s)\n", buff);
#endif

  if(read_st > 0) {
    return(read_st);
  }

  start = buff;
  mult = 1;
  if(buff[0] == '+') {
    mult = 1;
    if(strlen(buff) == 1) {
      scan_next(buff, fp);
      start = buff;
    }
    else {
      start = &(buff[1]);
    }
  }
  
  if(buff[0] == '-') {
    mult = -1;
    if(strlen(buff) == 1) {
      scan_next(buff, fp);
      start = buff;
    }
    else {
      start = &(buff[1]);
    }
  }
  
  if(first_is_number(start)) {
    coeff[*cnt] = atof(start);       
    sprintf(loc_name, "aa");
    scan_next(loc_name, fp);
  }
  else {
    coeff[*cnt] = 1;
    strcpy(loc_name, start);
  }

  read_st = is_subject_to(loc_name);

#ifdef LPIO_DEBUG
  printf("read_monom_obj: second buff: (%s)\n", buff);
#endif

  if(read_st > 0) {
    setObjectiveOffset(mult * coeff[*cnt]);

#ifdef LPIO_DEBUG
  printf("read_monom_obj: objectiveOffset: %f\n", objectiveOffset_);
#endif

    return(read_st);
  }

  coeff[*cnt] *= mult;
  name[*cnt] = strdup(loc_name);

#ifdef LPIO_DEBUG
  printf("read_monom_obj: (%f)  (%s)\n", coeff[*cnt], name[*cnt]);
#endif

  (*cnt)++;

  return(read_st);
} /* read_monom_obj */

/*************************************************************************/
int 
CoinLpIO::read_monom_row(FILE *fp, char *start_str, 
			 double *coeff, char **name, 
			 int cnt_coeff) const {

  double mult;
  char buff[1024], loc_name[1024], *start;
  int read_sense = -1;

  sprintf(buff, start_str);
  read_sense = is_sense(buff);
  if(read_sense > -1) {
    return(read_sense);
  }

  start = buff;
  mult = 1;
  if(buff[0] == '+') {
    mult = 1;
    if(strlen(buff) == 1) {
      scan_next(buff, fp);
      start = buff;
    }
    else {
      start = &(buff[1]);
    }
  }
  
  if(buff[0] == '-') {
    mult = -1;
    if(strlen(buff) == 1) {
      scan_next(buff, fp);
      start = buff;
    }
    else {
      start = &(buff[1]);
    }
  }
  
  if(first_is_number(start)) {
    coeff[cnt_coeff] = atof(start);       
    scan_next(loc_name, fp);
  }
  else {
    coeff[cnt_coeff] = 1;
    strcpy(loc_name, start);
  }

  coeff[cnt_coeff] *= mult;
  name[cnt_coeff] = strdup(loc_name);

#ifdef LPIO_DEBUG
  printf("CoinLpIO: read_monom_row: (%f)  (%s)\n", 
	 coeff[cnt_coeff], name[cnt_coeff]);
#endif  
  return(read_sense);
} /* read_monom_row */

/*************************************************************************/
void
CoinLpIO::realloc_coeff(double **coeff, char ***colNames, 
			int *maxcoeff) const {
  
  *maxcoeff *= 5;

  *colNames = (char **) realloc ((*colNames), (*maxcoeff+1) * sizeof(char *));
  *coeff = (double *) realloc ((*coeff), (*maxcoeff+1) * sizeof(double));

} /* realloc_coeff */

/*************************************************************************/
void
CoinLpIO::realloc_row(char ***rowNames, int **start, double **rhs, 
		      double **rowlow, double **rowup, int *maxrow) const {

  *maxrow *= 5;
  *rowNames = (char **) realloc ((*rowNames), (*maxrow+1) * sizeof(char *));
  *start = (int *) realloc ((*start), (*maxrow+1) * sizeof(int));
  *rhs = (double *) realloc ((*rhs), (*maxrow+1) * sizeof(double));
  *rowlow = (double *) realloc ((*rowlow), (*maxrow+1) * sizeof(double));
  *rowup = (double *) realloc ((*rowup), (*maxrow+1) * sizeof(double));

} /* realloc_row */

/*************************************************************************/
void
CoinLpIO::realloc_col(double **collow, double **colup, char **is_int,
		      int *maxcol) const {
  
  *maxcol += 100;
  *collow = (double *) realloc ((*collow), (*maxcol+1) * sizeof(double));
  *colup = (double *) realloc ((*colup), (*maxcol+1) * sizeof(double));
  *is_int = (char *) realloc ((*is_int), (*maxcol+1) * sizeof(char));

} /* realloc_col */

/*************************************************************************/
void 
CoinLpIO::read_row(FILE *fp, char *buff,
		   double **pcoeff, char ***pcolNames, 
		   int *cnt_coeff,
		   int *maxcoeff,
		   double *rhs, double *rowlow, double *rowup, 
		   int *cnt_row, double inf) const {

  int read_sense = -1;
  char start_str[1024];
  
  sprintf(start_str, buff);

  while(read_sense < 0) {

    if((*cnt_coeff) == (*maxcoeff)) {
      realloc_coeff(pcoeff, pcolNames, maxcoeff);
    }
    read_sense = read_monom_row(fp, start_str, 
				*pcoeff, *pcolNames, *cnt_coeff);

    (*cnt_coeff)++;

    scan_next(start_str, fp);

    if(feof(fp)) {
      printf("### ERROR: CoinLpIO:  read_monom_row(): Unable to read row monomial\n");
      exit(1);
    }
  }
  (*cnt_coeff)--;

  rhs[*cnt_row] = atof(start_str);

  switch(read_sense) {
  case 0: rowlow[*cnt_row] = -inf; rowup[*cnt_row] = rhs[*cnt_row];
    break;
  case 1: rowlow[*cnt_row] = rhs[*cnt_row]; rowup[*cnt_row] = rhs[*cnt_row];
    break;
  case 2: rowlow[*cnt_row] = rhs[*cnt_row]; rowup[*cnt_row] = inf; 
    break;
  default: break;
  }
  (*cnt_row)++;

} /* read_row */

/*************************************************************************/
int 
CoinLpIO::is_keyword(const char *buff) const {

  unsigned lbuff = strlen(buff);

  if(((lbuff == 5) && (CoinStrNCaseCmp(buff, "bound", 5) == 0)) ||
     ((lbuff == 6) && (CoinStrNCaseCmp(buff, "bounds", 6) == 0))) {
    return(1);
  }

  if(((lbuff == 7) && (CoinStrNCaseCmp(buff, "integer", 7) == 0)) ||
     ((lbuff == 8) && (CoinStrNCaseCmp(buff, "integers", 8) == 0))) {
    return(2);
  }
  
  if(((lbuff == 7) && (CoinStrNCaseCmp(buff, "general", 7) == 0)) ||
     ((lbuff == 8) && (CoinStrNCaseCmp(buff, "generals", 8) == 0))) {
    return(2);
  }

  if(((lbuff == 6) && (CoinStrNCaseCmp(buff, "binary", 6) == 0)) ||
     ((lbuff == 8) && (CoinStrNCaseCmp(buff, "binaries", 8) == 0))) {
    return(3);
  }
  
  if((lbuff == 3) && (CoinStrNCaseCmp(buff, "end", 3) == 0)) {
    return(4);
  }

  return(0);

} /* is_keyword */

/*************************************************************************/
void
CoinLpIO::readLp(const char *filename, const double epsilon)
{
  setEpsilon(epsilon);
  readLp(filename);
}

/*************************************************************************/
void
CoinLpIO::readLp(const char *filename)
{
  FILE *fp = fopen(filename, "r");
  if(!fp) {
    printf("### ERROR: CoinLpIO:  Unable to open file %s for reading\n",
	   filename);
    exit(1);
  }
  readLp(fp);
  fclose(fp);
}

/*************************************************************************/
void
CoinLpIO::readLp(FILE* fp, const double epsilon)
{
  setEpsilon(epsilon);
  readLp(fp);
}

/*************************************************************************/
void
CoinLpIO::readLp(FILE* fp)
{

  int maxrow = 1000;
  int maxcoeff = 40000;
  double lp_eps = getEpsilon();
  double lp_inf = getInfinity();

  char buff[1024];

  int objsense, cnt_coeff = 0, cnt_row = 0, cnt_obj = 0;
  char *objName = NULL;

  char **colNames = (char **) malloc ((maxcoeff+1) * sizeof(char *));
  double *coeff = (double *) malloc ((maxcoeff+1) * sizeof(double));

  char **rowNames = (char **) malloc ((maxrow+1) * sizeof(char *));
  int *start = (int *) malloc ((maxrow+1) * sizeof(int));
  double *rhs = (double *) malloc ((maxrow+1) * sizeof(double));
  double *rowlow = (double *) malloc ((maxrow+1) * sizeof(double));
  double *rowup = (double *) malloc ((maxrow+1) * sizeof(double));

  int i;

  objsense = find_obj(fp);

  int read_st = 0;
  while(!read_st) {
    read_st = read_monom_obj(fp, coeff, colNames, &cnt_obj, &objName);

    if(cnt_obj == maxcoeff) {
      realloc_coeff(&coeff, &colNames, &maxcoeff);
    }
  }
  
  start[0] = cnt_obj;
  cnt_coeff = cnt_obj;

  if(read_st == 2) {
    fscanf(fp, "%s", buff);
    unsigned lbuff = strlen(buff);

    if((lbuff != 2) || (CoinStrNCaseCmp(buff, "to", 2) != 0)) {
      printf("### ERROR: CoinLpIO::readLp(): Can not locate keyword 'Subject To'\n");
      exit(1);
    }
  }
  
  scan_next(buff, fp);

  while(!is_keyword(buff)) {
    if(buff[strlen(buff)-1] == ':') {
      buff[strlen(buff)-1] = '\0';

#ifdef LPIO_DEBUG
      printf("CoinLpIO::readLp(): rowName[%d]: %s\n", cnt_row, buff);
#endif

      rowNames[cnt_row] = strdup(buff);
      scan_next(buff, fp);
    }
    else {
      char rname[15];
      sprintf(rname, "cons%d", cnt_row); 
      rowNames[cnt_row] = strdup(rname);
    }
    read_row(fp, buff, 
	     &coeff, &colNames, &cnt_coeff, &maxcoeff, rhs, rowlow, rowup, 
	     &cnt_row, lp_inf);
    scan_next(buff, fp);
    start[cnt_row] = cnt_coeff;

    if(cnt_row == maxrow) {
      realloc_row(&rowNames, &start, &rhs, &rowlow, &rowup, &maxrow);
    }

  }

  numberRows_ = cnt_row;

  stopHash(1);
  startHash(colNames, cnt_coeff, 1);
  
  COINColumnIndex icol;
  int read_sense1,  read_sense2;
  double bnd1 = 0, bnd2 = 0;

  int maxcol = numberHash_[1] + 100;

  double *collow = (double *) malloc ((maxcol+1) * sizeof(double));
  double *colup = (double *) malloc ((maxcol+1) * sizeof(double));
  char *is_int = (char *) malloc ((maxcol+1) * sizeof(char));
  int has_int = 0;

  for (i=0; i<maxcol; i++) {
    collow[i] = 0;
    colup[i] = lp_inf;
    is_int[i] = 0;
  }

  int done = 0;

  while(!done) {
    switch(is_keyword(buff)) {

    case 1: /* Bounds section */ 
      scan_next(buff, fp);

      while(is_keyword(buff) == 0) {
	
	read_sense1 = -1;
	read_sense2 = -1;
	int mult = 1;
	char *start_str = buff;

	if(buff[0] == '-') {
	  mult = -1;
	  if(strlen(buff) == 1) {
	    scan_next(buff, fp);
	    start_str = buff;
	  }
	  else {
	    start_str = &(buff[1]);
	  }
	}

	if(first_is_number(start_str)) {
	  bnd1 = mult * atof(start_str);
	  scan_next(buff, fp);
	  read_sense1 = is_sense(buff);
	  if(read_sense1 < 0) {
	    printf("### ERROR: CoinLpIO::readLp(): lost in Bounds section; expect a sense, get: %s\n", buff);
	    exit(1);
	  }
	  scan_next(buff, fp);
	}
	
	icol = findHash(buff, 1);
	if(icol < 0) {
	  printf("### WARNING: CoinLpIO::readLp(): Variable %s does not appear in objective function or constraints\n", buff);
	  insertHash(buff, 1);
	  icol = findHash(buff, 1);
	  if(icol == maxcol) {
	    realloc_col(&collow, &colup, &is_int, &maxcol);
	  }
	}
	
	scan_next(buff, fp);
	if(is_free(buff)) {
	  collow[icol] = -lp_inf;
	  scan_next(buff, fp);
	}
       	else {
	  read_sense2 = is_sense(buff);
	  if(read_sense2 > -1) {
	    scan_next(buff, fp);
	    mult = 1;
	    start_str = buff;

	    if(buff[0] == '-'||buff[0] == '+') {
	      mult = (buff[0] == '-') ? -1 : +1;
	      if(strlen(buff) == 1) {
		scan_next(buff, fp);
		start_str = buff;
	      }
	      else {
		start_str = &(buff[1]);
	      }
	    }
	    if(first_is_number(start_str)) {
	      bnd2 = mult * atof(start_str);
	      scan_next(buff, fp);
	    }
	    else {
	      if (strncmp(start_str,"inf",3)&&strncmp(start_str,"Inf",3)) {
		printf("### ERROR: CoinLpIO::readLp(): lost in Bounds section; expect a number, get: %s\n", buff);
		exit(1);
	      } else {
		// infinity
		bnd2 = DBL_MAX;
		scan_next(buff, fp);
	      }
	    }
	  }

	  if((read_sense1 > -1) && (read_sense2 > -1)) {
	    if(read_sense1 != read_sense2) {
	      printf("### ERROR: CoinLpIO::readLp(): lost in Bounds section; \n\
variable: %s read_sense1: %d  read_sense2: %d\n", buff, read_sense1, read_sense2);
	      exit(1);
	    }
	    else {
	      if(read_sense1 == 1) {
		if(fabs(bnd1 - bnd2) > lp_eps) {
		  printf("### ERROR: CoinLpIO::readLp(): lost in Bounds section; \nvariable: %s read_sense1: %d  read_sense2: %d  bnd1: %f  bnd2: %f\n", 
			 buff, read_sense1, read_sense2, bnd1, bnd2);
		  exit(1);
		}
		collow[icol] = bnd1;
		colup[icol] = bnd1;
	      }
	      if(read_sense1 == 0) {
		collow[icol] = bnd1;
		colup[icol] = bnd2;	    
	      }
	      if(read_sense1 == 2) {
		colup[icol] = bnd1;
		collow[icol] = bnd2;	    
	      }
	    }
	  }
	  else {
	    if(read_sense1 > -1) {
	      switch(read_sense1) {
	      case 0: collow[icol] = bnd1; break;
	      case 1: collow[icol] = bnd1; colup[icol] = bnd1; break;
	      case 2: colup[icol] = bnd1; break;
	      }
	    }
	    if(read_sense2 > -1) {
	      switch(read_sense2) {
	      case 0: colup[icol] = bnd2; break;
	      case 1: collow[icol] = bnd2; colup[icol] = bnd2; break;
	      case 2: collow[icol] = bnd2; break;
	      }
	    }
	  }
	}
      }
      break;

    case 2: /* Integers/Generals section */

      scan_next(buff, fp);
    
      while(is_keyword(buff) == 0) {
      
	icol = findHash(buff, 1);

#ifdef LPIO_DEBUG
	printf("CoinLpIO::readLp(): integer: colname: (%s)  icol: %d\n", 
	       buff, icol);
#endif

	if(icol < 0) {
	  printf("### WARNING: CoinLpIO::readLp(): Integer variable %s does not appear in objective function or constraints\n", buff);
	  insertHash(buff, 1);
	  icol = findHash(buff, 1);
	  if(icol == maxcol) {
	    realloc_col(&collow, &colup, &is_int, &maxcol);
	  }
	  
#ifdef LPIO_DEBUG
	  printf("CoinLpIO::readLp(): integer: colname: (%s)  icol: %d\n", 
		 buff, icol);
#endif
	  
	}
	is_int[icol] = 1;
	has_int = 1;
	scan_next(buff, fp);
      };
      break;

    case 3: /* Binaries section */
  
      scan_next(buff, fp);
      
      while(is_keyword(buff) == 0) {
	
	icol = findHash(buff, 1);
	
#ifdef LPIO_DEBUG
	printf("CoinLpIO::readLp(): binary: colname: (%s)  icol: %d\n", 
	       buff, icol);
#endif
	
	if(icol < 0) {
	  printf("### WARNING: CoinLpIO::readLp(): Binary variable %s does not appear in objective function or constraints\n", buff);
	  insertHash(buff, 1);
	  icol = findHash(buff, 1);
	  if(icol == maxcol) {
	    realloc_col(&collow, &colup, &is_int, &maxcol);
	  }
#ifdef LPIO_DEBUG
	  printf("CoinLpIO::readLp(): binary: colname: (%s)  icol: %d\n", 
		 buff, icol);
#endif
	  
	}
	
	is_int[icol] = 1;
	has_int = 1;
	if(collow[icol] < 0) {
	  collow[icol] = 0;
	}
	if(colup[icol] > 1) {
	  colup[icol] = 1;
	}
	scan_next(buff, fp);
      }
      break;
      
    case 4: done = 1; break;
      
    default: 
      printf("### ERROR: CoinLpIO::readLp(): Lost while reading: (%s)\n",
	     buff);
      exit(1);
      break;
    }
  }

#ifdef LPIO_DEBUG
  printf("CoinLpIO::readLp(): Done with reading the Lp file\n");
#endif

  int *ind = (int *) malloc ((maxcoeff+1) * sizeof(int));

  for(i=0; i<cnt_coeff; i++) {
    ind[i] = findHash(colNames[i], 1);

#ifdef LPIO_DEBUG
    printf("CoinLpIO::readLp(): name[%d]: (%s)   ind: %d\n", 
	   i, colNames[i], ind[i]);
#endif

    if(ind[i] < 0) {
      printf("### ERROR: CoinLpIO::readLp(): Problem with hash tables\nCan not find name %s\n", colNames[i]);
      exit(1);
    }
  }
  
  numberColumns_ = numberHash_[1];
  numberElements_ = cnt_coeff - start[0];

  double *obj = (double *) malloc (numberColumns_ * sizeof(double));
  memset(obj, 0, numberColumns_ * sizeof(double));

  for(i=0; i<cnt_obj; i++) {
    icol = findHash(colNames[i], 1);
    if(icol < 0) {
      printf("### ERROR: CoinLpIO::readLp(): Problem with hash tables\nCan not find name %s (objective function)\n", colNames[i]);
      exit(1);
    }
    obj[icol] = objsense * coeff[i];
  }

  for(i=0; i<cnt_row+1; i++) {
    start[i] -= cnt_obj;
  }

  CoinPackedMatrix *matrix = 
    new CoinPackedMatrix(false,
			 numberColumns_, numberRows_, numberElements_,
			 &(coeff[cnt_obj]), &(ind[cnt_obj]), start, NULL);

#ifdef LPIO_DEBUG
  matrix->dumpMatrix();  
#endif

  setLpDataWithoutRowAndColNames(*matrix, collow, colup,
				 obj, has_int ? is_int : 0, rowlow, rowup);


  if(objName == NULL) {
    rowNames[cnt_row] = strdup("obj");
  }
  else {
    rowNames[cnt_row] = strdup(objName);
  }

  // Hash tables for column names are already set up
  setLpDataRowAndColNames(rowNames, NULL);

  if(are_invalid_names(names_[1], numberHash_[1], false)) {
    setDefaultColNames();
    printf("### WARNING: CoinLpIO::readLp(): Invalid column names\nNow using default column names.\n");
  } 
  
  for(i=0; i<cnt_coeff; i++) {
    free(colNames[i]);
  }
  free(colNames);

  for(i=0; i<cnt_row+1; i++) {
    free(rowNames[i]);
  }
  free(rowNames);

  free(objName);

#ifdef LPIO_DEBUG
  writeLp("readlp.xxx");
  printf("CoinLpIO::readLp(): read Lp file written in file readlp.xxx\n");
#endif

  free(coeff);
  free(start);
  free(ind);
  free(colup);
  free(collow);
  free(rhs);
  free(rowlow);
  free(rowup);
  free(is_int);
  free(obj);
  delete matrix;

} /* readLp */

/*************************************************************************/
void
CoinLpIO::print() const {

  printf("problemName_: %s\n", problemName_);
  printf("numberRows_: %d\n", numberRows_);
  printf("numberColumns_: %d\n", numberColumns_);

  printf("matrixByRows_:\n");
  matrixByRow_->dumpMatrix();  

  int i;
  printf("rowlower_:\n");
  for(i=0; i<numberRows_; i++) {
    printf("%.5f ", rowlower_[i]);
  }
  printf("\n");

  printf("rowupper_:\n");
  for(i=0; i<numberRows_; i++) {
    printf("%.5f ", rowupper_[i]);
  }
  printf("\n");
  
  printf("collower_:\n");
  for(i=0; i<numberColumns_; i++) {
    printf("%.5f ", collower_[i]);
  }
  printf("\n");

  printf("colupper_:\n");
  for(i=0; i<numberColumns_; i++) {
    printf("%.5f ", colupper_[i]);
  }
  printf("\n");
  
  printf("objective_:\n");
  for(i=0; i<numberColumns_; i++) {
    printf("%.5f ", objective_[i]);
  }
  printf("\n");
  
  if(integerType_ != NULL) {
    printf("integerType_:\n");
    for(i=0; i<numberColumns_; i++) {
      printf("%c ", integerType_[i]);
    }
  }
  else {
    printf("integerType_: NULL\n");
  }

  printf("\n");
  if(fileName_ != NULL) {
    printf("fileName_: %s\n", fileName_);
  }
  printf("infinity_: %.5f\n", infinity_);
} /* print */


/*************************************************************************/
// Hash functions slightly modified from CoinMpsIO.cpp

static int
hash(const char *name, int maxsiz, int length)
{
  static int mmult[] = {
    262139, 259459, 256889, 254291, 251701, 249133, 246709, 244247,
    241667, 239179, 236609, 233983, 231289, 228859, 226357, 223829,
    221281, 218849, 216319, 213721, 211093, 208673, 206263, 203773,
    201233, 198637, 196159, 193603, 191161, 188701, 186149, 183761,
    181303, 178873, 176389, 173897, 171469, 169049, 166471, 163871,
    161387, 158941, 156437, 153949, 151531, 149159, 146749, 144299,
    141709, 139369, 136889, 134591, 132169, 129641, 127343, 124853,
    122477, 120163, 117757, 115361, 112979, 110567, 108179, 105727,
    103387, 101021, 98639, 96179, 93911, 91583, 89317, 86939, 84521,
    82183, 79939, 77587, 75307, 72959, 70793, 68447, 66103
  };
  int n = 0;
  int j;

  for ( j = 0; j < length; ++j ) {
    int iname = name[j];

    n += mmult[j] * iname;
  }
  return ( abs ( n ) % maxsiz );	/* integer abs */
}

/************************************************************************/
//  startHash.  Creates hash list for names
//  setup names_[section] with names in the same order as in the parameter, 
//  but removes duplicates

void
CoinLpIO::startHash(char const * const * const names, 
		    const COINColumnIndex number, int section)
{
  maxHash_[section] = 4 * number;
  int maxhash = maxHash_[section];
  COINColumnIndex i, ipos, iput;

  names_[section] = (char **) malloc(maxhash * sizeof(char *));
  hash_[section] = new CoinHashLink[maxhash];
  
  CoinHashLink * hashThis = hash_[section];
  char **hashNames = names_[section];
  
  for ( i = 0; i < maxhash; i++ ) {
    hashThis[i].index = -1;
    hashThis[i].next = -1;
  }
  
  /*
   * Initialize the hash table.  Only the index of the first name that
   * hashes to a value is entered in the table; subsequent names that
   * collide with it are not entered.
   */
  
  for (i=0; i<number; i++) {
    const char *thisName = names[i];
    int length = strlen(thisName);
    
    ipos = hash(thisName, maxhash, length);
    if (hashThis[ipos].index == -1) {
      hashThis[ipos].index = i; // will be changed below
    }
  }
  
  /*
   * Now take care of the names that collided in the preceding loop,
   * by finding some other entry in the table for them.
   * Since there are as many entries in the table as there are names,
   * there must be room for them.
   * Also setting up hashNames.
   */

  int cnt_distinct = 0;
  
  iput = -1;
  for (i=0; i<number; i++) {
    const char *thisName = names[i];
    int length = strlen(thisName);
    
    ipos = hash(thisName, maxhash, length);
    
    while (1) {
      COINColumnIndex j1 = hashThis[ipos].index;
      
      if(j1 == i) {

	// first occurence of thisName in the parameter "names"

	hashThis[ipos].index = cnt_distinct;
	hashNames[cnt_distinct] = strdup(thisName);
	cnt_distinct++;
	break;
      }
      else {

#ifdef LPIO_DEBUG
	if(j1 > i) {
	  printf("### ERROR: CoinLpIO::startHash(): j1: %d  i: %d\n",
		 j1, i);
	  exit(1);
	}
#endif

	if (strcmp(thisName, hashNames[j1]) == 0) {

	  // thisName already entered

	  break;
	}
	else {
	  // Collision; check if thisName already entered

	  COINColumnIndex k = hashThis[ipos].next;
	
	  if (k == -1) {

	    // thisName not found; enter it

	    while (1) {
	      ++iput;
	      if (iput > maxhash) {
		printf ( "### ERROR: CoinLpIO::startHash(): too many names\n");
		exit(1);
		break;
	      }
	      if (hashThis[iput].index == -1) {
		break;
	      }
	    }
	    hashThis[ipos].next = iput;
	    hashThis[iput].index = cnt_distinct;
	    hashNames[cnt_distinct] = strdup(thisName);
	    cnt_distinct++;
	    break;
	  } 
	  else {
	    ipos = k;

	    // continue the check with names in collision 

	  }
	}
      }
    }
  }

  numberHash_[section] = cnt_distinct;

} /* startHash */

/**************************************************************************/
//  stopHash.  Deletes hash storage
void
CoinLpIO::stopHash(int section)
{
  char **names = names_[section];

  freePreviousNames(section);
  previous_names_[section] = names_[section];
  card_previous_names_[section] = numberHash_[section];

  delete[] hash_[section];
  hash_[section] = NULL;

  names = NULL;

  maxHash_[section] = 0;
  numberHash_[section] = 0;

  if(section == 0) {
    free(objName_);
    objName_ = NULL;
  }
} /* stopHash */

/**********************************************************************/
//  findHash.  -1 not found
COINColumnIndex
CoinLpIO::findHash(const char *name, int section) const
{
  COINColumnIndex found = -1;

  char ** names = names_[section];
  CoinHashLink * hashThis = hash_[section];
  COINColumnIndex maxhash = maxHash_[section];
  COINColumnIndex ipos;

  /* default if we don't find anything */
  if (!maxhash)
    return -1;

  int length = strlen (name);

  ipos = hash (name, maxhash, length);
  while (1) {
    COINColumnIndex j1 = hashThis[ipos].index;

    if (j1 >= 0) {
      char *thisName2 = names[j1];

      if (strcmp (name, thisName2) != 0) {
	COINColumnIndex k = hashThis[ipos].next;

	if (k != -1)
	  ipos = k;
	else
	  break;
      } 
      else {
	found = j1;
	break;
      }
    } 
    else {
      found = -1;
      break;
    }
  }
  return found;
} /* findHash */

/*********************************************************************/
void
CoinLpIO::insertHash(const char *thisName, int section)
{

  int number = numberHash_[section];
  int maxhash = maxHash_[section];

  CoinHashLink * hashThis = hash_[section];
  char **hashNames = names_[section];

  int iput = -1;
  int length = strlen (thisName);

  int ipos = hash (thisName, maxhash, length);

  while (1) {
    COINColumnIndex j1 = hashThis[ipos].index;
    
    if (j1 == -1) {
      hashThis[ipos].index = number;
      break;
    }
    else {
      char *thisName2 = hashNames[j1];
      
      if ( strcmp (thisName, thisName2) != 0 ) {
	COINColumnIndex k = hashThis[ipos].next;
	
	if (k == -1) {
	  while (1) {
	    ++iput;
	    if (iput == maxhash) {
	      printf ( "### ERROR: CoinLpIO::insertHash(): too many names\n");
	      exit(1);
	      break;
	    }
	    if (hashThis[iput].index == -1) {
	      break;
	    }
	  }
	  hashThis[ipos].next = iput;
	  hashThis[iput].index = number;
	  break;
	} 
	else {
	  ipos = k;
	  /* nothing worked - try it again */
	}
      }
    }
  }

  hashNames[number] = strdup(thisName);
  (numberHash_[section])++;

}

