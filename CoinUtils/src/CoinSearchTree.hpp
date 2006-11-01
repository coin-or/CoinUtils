#ifndef CoinSearchTree_H
#define CoinSearchTree_H

#include <vector>
#include <algorithm>
#include <cmath>

#include "CoinFinite.hpp"

//#############################################################################

/** A class from which the real tree nodes should be derived from. Some of the
    data that undoubtedly exist in the real tree node is replicated here for
    fast access. This class is used in the various comparison functions. */
class CoinTreeNode {
protected:
    CoinTreeNode() : depth_(-1), quality_(-COIN_DBL_MAX) {}
    CoinTreeNode(int d, double q/*, double f*/) :
	depth_(d), quality_(q)/*, fractionality_(f)*/ {}
    CoinTreeNode(const CoinTreeNode& x) :
	depth_(x.depth_), quality_(x.quality_) {}
    CoinTreeNode& operator=(const CoinTreeNode& x) {
	// Not worth to test (this != &x)
	depth_ = x.depth_;
	quality_ = x.quality_;
	return *this;
    }
public:
    /// The depth of the node in the tree
    int depth_;
    /// Some quality for the node, e.g., the lower bound
    double quality_;
    /** A measure of fractionality, e.g., the fraction of unsatisfied
	integrality requirements */
    /*double fractionality_;*/
public:
    virtual ~CoinTreeNode() {}
    // Default (bitwise) constr, copy constr, assignment op, and destr all OK.
    inline void set(int d, double q/*, double f*/) {
	depth_ = d;
	quality_ = q;
	/*fractionality_ = f;*/
    }
};

//#############################################################################

/** Function objects to compare search tree nodes. The comparison function
    must return true if the first argument is "better" than the second one,
    i.e., it should be processed first. */
/*@{*/
/** Depth First Search. */
struct CoinSearchTreeCompareDepth {
    inline bool operator()(const CoinTreeNode* x,
			   const CoinTreeNode* y) const {
	return x->depth_ > y->depth_;
    }
};

//-----------------------------------------------------------------------------
/* Breadth First Search */
struct CoinSearchTreeCompareBreadth {
    inline bool operator()(const CoinTreeNode* x,
			   const CoinTreeNode* y) const {
	return x->depth_ < y->depth_;
    }
};

//-----------------------------------------------------------------------------
/** Best first search */
struct CoinSearchTreeCompareBest {
    inline bool operator()(const CoinTreeNode* x,
			   const CoinTreeNode* y) const {
	return x->quality_ < y->quality_;
    }
};

//#############################################################################

class CoinSearchTreeBase
{
private:
    CoinSearchTreeBase(const CoinSearchTreeBase&);
    CoinSearchTreeBase& operator=(const CoinSearchTreeBase&);

protected:
    std::vector<CoinTreeNode*> nodes_;
    size_t numInserted_;

protected:
    CoinSearchTreeBase() : nodes_(), numInserted_(0) {}

public:
    virtual ~CoinSearchTreeBase() {}

    inline const std::vector<CoinTreeNode*> getNodes() const {
	return nodes_;
    }
    inline bool empty() const { return nodes_.empty(); }
    inline size_t size() const { return nodes_.size(); }
    inline size_t numInserted() const { return numInserted_; }
    inline CoinTreeNode* top() const { return nodes_.front(); }
    virtual void pop() = 0;
    virtual void push(CoinTreeNode* node) = 0;
};

//#############################################################################

#ifdef CAN_TRUST_STL_HEAP

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
    void push(CoinTreeNode* node) {
	nodes_.push_back(node);
	std::push_heap(nodes_.begin(), nodes_.end(), comp_);
	++numInserted_;
    }
};

#else

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
	std::sort(nodes_.begin(), nodes_.end(), comp_);
	numInserted_ = nodes_.size();
    }
    ~CoinSearchTree() {}

    void pop() {
	CoinTreeNode* node = nodes_.back();
	nodes_.pop_back();
	const int size = nodes_.size();
	if (size > 0) {
	    CoinTreeNode** nodes = &nodes_[0];
	    --nodes;
	    int pos = 1;
	    int ch;
	    for (ch = 2; ch < size; pos = ch, ch *= 2) {
		if (comp_(nodes[ch+1], nodes[ch]))
		    ++ch;
		if (comp_(node, nodes[ch]))
		    break;
		nodes[pos] = nodes[ch];
	    }
	    if (ch == size) {
		if (comp_(nodes[ch], node)) {
		    nodes[pos] = nodes[ch];
		    pos = ch;
		}
	    }
	    nodes[pos] = node;
	}
    }
    void push(CoinTreeNode* node) {
	nodes_.push_back(node);
	CoinTreeNode** nodes = &nodes_[0];
	--nodes;
	int pos = nodes_.size();
	int ch;
	for (ch = pos/2; ch != 0; pos = ch, ch /= 2) {
	    if (comp_(nodes[ch], node))
		break;
	    nodes[pos] = nodes[ch];
	}
	nodes[pos] = node;
	++numInserted_;
    }
};

#endif

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
