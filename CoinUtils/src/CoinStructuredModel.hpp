// Copyright (C) 2008, International Business Machines
// Corporation and others.  All Rights Reserved.
#ifndef CoinStructuredModel_H
#define CoinStructuredModel_H

#include "CoinModel.hpp"
#include <vector>

/** 
    This is a model which is made up of CoinModel blocks.
*/
  typedef struct {
    unsigned int matrix:1; // nonzero if matrix exists
    unsigned int rhs:1; // nonzero if non default rhs exists
    unsigned int rowName:1; // nonzero if row names exists
    unsigned int rowBlock:29; // Which row block
    unsigned int integer:1; // nonzero if integer information exists
    unsigned int bounds:1; // nonzero if non default bounds/objective exists
    unsigned int columnName:1; // nonzero if column names exists
    unsigned int columnBlock:29; // Which column block
} CoinModelBlockInfo;

class CoinStructuredModel {
  
public:
  /**@name Useful methods for building model */
  //@{
  /** add a block from a CoinModel using names given as parameters 
      returns number of errors (e.g. both have objectives but not same)
   */
  int addBlock(const std::string & rowBlock,
		const std::string & columnBlock,
		const CoinModel & block);
  /** add a block from a CoinModel with names in model
      returns number of errors (e.g. both have objectives but not same)
 */
  int addBlock(const CoinModel & block);
  /** add a block from a CoinModel using names given as parameters 
      returns number of errors (e.g. both have objectives but not same)
      This passes in block - structured model takes ownership
   */
  int addBlock(const std::string & rowBlock,
		const std::string & columnBlock,
		CoinModel * block);

  /** Write the problem in MPS format to a file with the given filename.
      
  \param compression can be set to three values to indicate what kind
  of file should be written
  <ul>
  <li> 0: plain text (default)
  <li> 1: gzip compressed (.gz is appended to \c filename)
  <li> 2: bzip2 compressed (.bz2 is appended to \c filename) (TODO)
  </ul>
  If the library was not compiled with the requested compression then
  writeMps falls back to writing a plain text file.
  
  \param formatType specifies the precision to used for values in the
  MPS file
  <ul>
  <li> 0: normal precision (default)
  <li> 1: extra accuracy
  <li> 2: IEEE hex
  </ul>
  
  \param numberAcross specifies whether 1 or 2 (default) values should be
  specified on every data line in the MPS file.
  
  not const as may change model e.g. fill in default bounds
  */
  int writeMps(const char *filename, int compression = 0,
               int formatType = 0, int numberAcross = 2, bool keepStrings=false) ;
  /** Decompose a CoinModel
      1 - try D-W
      2 - try Benders
      3 - try Staircase
      Returns number of blocks
  */
  int decompose(const CoinModel &model,int type);
  
   //@}


  /**@name For getting information */
   //@{
   /// Return number of rows
  inline int numberRows() const
  { return numberRows_;}
   /// Return number of columns
  inline int numberColumns() const
  { return numberColumns_;}
   /// Return number of row blocks
  inline int numberRowBlocks() const
  { return numberRowBlocks_;}
   /// Return number of column blocks
  inline int numberColumnBlocks() const
  { return numberColumnBlocks_;}
   /// Return number of elementBlocks
  inline CoinBigIndex numberElementBlocks() const
  { return numberElementBlocks_;}
  /** Returns the (constant) objective offset
      This is the RHS entry for the objective row
  */
  inline double objectiveOffset() const
  { return objectiveOffset_;}
  /// Set objective offset
  inline void setObjectiveOffset(double value)
  { objectiveOffset_=value;}
  /// Get print level 0 - off, 1 - errors, 2 - more
  inline int logLevel() const
  { return logLevel_;}
  /// Set print level 0 - off, 1 - errors, 2 - more
  void setLogLevel(int value);
  /// Return the problem name
  inline const char * getProblemName() const
  { return problemName_.c_str();}
  /// Set problem name
  void setProblemName(const char *name) ;
  /// Set problem name
  void setProblemName(const std::string &name) ;
  /// Return the i'th row block name
  inline const std::string & getRowBlock(int i) const
  { return rowBlockName_[i];}
  /// Set i'th row block name
  inline void setRowBlock(int i,const std::string &name) 
  { rowBlockName_[i] = name;}
  /// Add or check a row block name and number of rows
  int addRowBlock(int numberRows,const std::string &name) ;
  /// Return i'th the column block name
  inline const std::string & getColumnBlock(int i) const
  { return columnBlockName_[i];}
  /// Set i'th column block name
  inline void setColumnBlock(int i,const std::string &name) 
  { columnBlockName_[i] = name;}
  /// Add or check a column block name and number of columns
  int addColumnBlock(int numberColumns,const std::string &name) ;
  /// Return i'th block
  inline CoinModel * block(int i) const
  { return blocks_[i];}
  /// Return i'th block type
  inline const CoinModelBlockInfo &  blockType(int i) const
  { return blockType_[i];}
  /// Return block corresponding to row and column
  const CoinModel *  block(int row,int column) const;
  /// Return block number corresponding to row and column
  int  blockIndex(int row,int column) const;
  /** Fill pointers corresponding to row and column */

  CoinModelBlockInfo block(int row,int column,
	     const double * & rowLower, const double * & rowUpper,
	     const double * & columnLower, const double * & columnUpper,
	     const double * & objective) const;
  /// Direction of optimization (1 - minimize, -1 - maximize, 0 - ignore
  inline double optimizationDirection() const {
    return  optimizationDirection_;
  }
  /// Set direction of optimization (1 - minimize, -1 - maximize, 0 - ignore
  inline void setOptimizationDirection(double value)
  { optimizationDirection_=value;}
   //@}

  /**@name Constructors, destructor */
   //@{
   /** Default constructor. */
  CoinStructuredModel();
  /** Read a problem in MPS format from the given filename.
      May try and decompose
   */
  CoinStructuredModel(const char *fileName,int decompose=0);
   /** Destructor */
   ~CoinStructuredModel();
   //@}

   /**@name Copy method */
   //@{
   /** The copy constructor. */
   CoinStructuredModel(const CoinStructuredModel&);
  /// =
   CoinStructuredModel& operator=(const CoinStructuredModel&);
   //@}

private:

  /** Fill in info structure and update counts
      Returns number of inconsistencies on border
  */
  int fillInfo(CoinModelBlockInfo & info,const CoinModel * block);
  /**@name Data members */
   //@{
  /// Current number of rows
  int numberRows_;
  /// Current number of row blocks
  int numberRowBlocks_;
  /// Current number of columns
  int numberColumns_;
  /// Current number of column blocks
  int numberColumnBlocks_;
  /// Current number of element blocks
  int numberElementBlocks_;
  /// Maximum number of element blocks
  int maximumElementBlocks_;
  /// Direction of optimization (1 - minimize, -1 - maximize, 0 - ignore
  double optimizationDirection_;
  /// Objective offset to be passed on
  double objectiveOffset_;
  /// Problem name
  std::string problemName_;
  /// Rowblock name
  std::vector<std::string> rowBlockName_;
  /// Columnblock name
  std::vector<std::string> columnBlockName_;
  /// Blocks
  CoinModel ** blocks_;
  /// Which parts of model are set in block
  CoinModelBlockInfo * blockType_;
  /** Print level.
      0 - no output
      1 - on errors
      2 - more detailed
  */
  int logLevel_;
   //@}
};
#endif
