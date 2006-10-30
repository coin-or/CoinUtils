#ifndef CoinSearchTree_H
#define CoinSearchTree_H

#include <queue>
#include <set>
#include <cmath>

//#############################################################################

/** An empty class, just so that solvers can derive their own search tree node
    class from this one. */
class CoinTreeNode {
public:
    virtual ~CoinTreeNode() {}
};
    
//#############################################################################

/** A class that contains a pointer to the real tree node and a few pieces of
    information lifted out for fast access. This class is used in the various
    comparison functions. */
class CoinTreeNodeInfo {
public:
    CoinTreeNode* node;
    /// The depth of the node in the tree
    int depth;
    /// Some quality for the node, e.g., the lower bound
    double quality;
    /** A measure of fractionality, e.g., the fraction of unsatisfied
	integrality requirements */
    /*double fractionality;*/
public:
    CoinTreeNodeInfo(CoinTreeNode* n, int d, double q/*, double f*/) :
	node(n), depth(d), quality(q)/*, fractionality(f)*/ {}
    // Default (bitwise) constr, copy constr, assignment op, and destr all OK.
    inline void set(CoinTreeNode* n, int d, double q/*, double f*/) {
	node = n;
	depth = d;
	quality = q;
	/*fractionality = f;*/
    }
};

//#############################################################################

/** Function objects to compare search tree nodes. The comparison function
    must return true if the first argument is "better" than the second one,
    i.e., it should be processed first. */
/*@{*/
/** Depth First Search. */
struct CoinSearchTreeCompareDepth {
    inline bool operator()(const CoinTreeNodeInfo* x,
			   const CoinTreeNodeInfo* y) const {
	return x->depth > y->depth;
    }
};

//-----------------------------------------------------------------------------
/* Breadth First Search */
struct CoinSearchTreeCompareBreadth {
    inline bool operator()(const CoinTreeNodeInfo* x,
			   const CoinTreeNodeInfo* y) const {
	return x->depth < y->depth;
    }
};

//-----------------------------------------------------------------------------
/** Best first search */
struct CoinSearchTreeCompareBest {
    inline bool operator()(const CoinTreeNodeInfo* x,
			   const CoinTreeNodeInfo* y) const {
	return x->quality < y->quality;
    }
};

//#############################################################################

class CoinSearchTreeBase
{
private:
    CoinSearchTreeBase(const CoinSearchTreeBase&);
    CoinSearchTreeBase& operator=(const CoinSearchTreeBase&);

protected:
    std::vector<CoinTreeNodeInfo*> nodes_;
    size_t numInserted_;

protected:
    CoinSearchTreeBase() : nodes_(), numInserted_(0) {}

public:
    virtual ~CoinSearchTreeBase() {}

    inline const std::vector<CoinTreeNodeInfo*> getNodes() const {
	return nodes_;
    }
    inline bool empty() const { return nodes_.empty(); }
    inline size_t size() const { return nodes_.size(); }
    inline size_t numInserted() const { return numInserted_; }
    inline CoinTreeNodeInfo* top() const { return nodes_.front(); }
    virtual void pop() = 0;
    virtual void push(CoinTreeNodeInfo* node) = 0;
};

//#############################################################################

template <class Comp>
class CoinSearchTree : public CoinSearchTreeBase
{
private:
    Comp comp_;
public:
    CoinSearchTree() : CoinSearchTreeBase(), comp_() {}
    CoinSearchTree(const CoinSearchTreeBase& t) :
	CoinSearchTreeBase(), comp_() {
	nodes_ = t.getNodes();
	std::make_heap(nodes_.begin(), nodes_.end(), comp_);
	numInserted_ = nodes_.size();
    }
    ~CoinSearchTree() {}

    void pop() {
	std::pop_heap(nodes_.begin(), nodes_.end(), comp_);
	nodes_.pop_back();
    }
    void push(CoinTreeNodeInfo* node) {
	nodes_.push_back(node);
	std::push_heap(nodes_.begin(), nodes_.end(), comp_);
	++numInserted_;
    }
};

//#############################################################################

class CoinSearchTreeManager
{
private:
    CoinSearchTreeManager(const CoinSearchTreeManager&);
    CoinSearchTreeManager& operator=(const CoinSearchTreeManager&);
private:
    CoinSearchTreeBase* tree_;
    int numSolution;
public:
    CoinSearchTreeManager() : tree_(NULL), numSolution(0) {}
    virtual ~CoinSearchTreeManager() {
	delete tree_;
    }
    inline void setTree(CoinSearchTreeBase* t) { delete tree_; tree_ = t; }
    inline CoinSearchTreeBase* getTree() const { return tree_; }
    void newSolution(double solValue);
    void reevaluateSearchStrategy();
};

//#############################################################################

#endif
