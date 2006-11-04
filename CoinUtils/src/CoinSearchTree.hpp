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
    /** Some quality for the node. For normal branch-and-cut problems the LP
	relaxation value will do just fine. It is probably an OK approximation
	even if column generation is done. */
    double quality_;
    /** A true lower bound on the node. May be -infinity. For normal
	branch-and-cut problems the LP relaxation value is OK. It is different
	when column generation is done. */
    double true_lower_bound_;
    /** A measure of fractionality, e.g., the fraction of unsatisfied
	integrality requirements */
    /*double fractionality_;*/
public:
    virtual ~CoinTreeNode() {}
    // Default (bitwise) constr, copy constr, assignment op, and destr all OK.
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

    inline const std::vector<CoinTreeNode*>& getNodes() const {
	return nodes_;
    }
    inline bool empty() const { return nodes_.empty(); }
    inline size_t size() const { return nodes_.size(); }
    inline size_t numInserted() const { return numInserted_; }
    inline CoinTreeNode* top() const { return nodes_.front(); }
    virtual void pop() = 0;
    virtual void push(CoinTreeNode* node, const bool incrInserted = true) = 0;
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
    void push(CoinTreeNode* node, const bool incrInserted = true) {
	nodes_.push_back(node);
	std::push_heap(nodes_.begin(), nodes_.end(), comp_);
	if (incrInserted) {
	    ++numInserted_;
	}
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
    void push(CoinTreeNode* node, const bool incrInserted = true) {
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
	if (incrInserted) {
	    ++numInserted_;
	}
    }
};

#endif

//#############################################################################

enum CoinNodeAction {
    CoinAddNodeToCandidates,
    CoinTestNodeForDiving,
    CoinDiveIntoNode
};

class CoinSearchTreeManager
{
private:
    CoinSearchTreeManager(const CoinSearchTreeManager&);
    CoinSearchTreeManager& operator=(const CoinSearchTreeManager&);
private:
    CoinSearchTreeBase* candidates_;
    int numSolution;
    /** Whether there is an upper bound or not. The upper bound may have come
	as input, not necessarily from a solution */
    bool hasUB_;

    /** variable used to test whether we need to reevaluate search strategy */
    bool recentlyReevaluatedSearchStrategy_;
    
public:
    CoinSearchTreeManager() :
	candidates_(NULL),
	numSolution(0),
	recentlyReevaluatedSearchStrategy_(true)
    {}
    virtual ~CoinSearchTreeManager() {
	delete candidates_;
    }
    
    inline void setTree(CoinSearchTreeBase* t) {
	delete candidates_;
	candidates_ = t;
    }
    inline CoinSearchTreeBase* getTree() const {
	return candidates_;
    }

    inline bool empty() const { return candidates_->empty(); }
    inline size_t size() const { return candidates_->size(); }
    inline size_t numInserted() const { return candidates_->numInserted(); }
    inline CoinTreeNode* top() const { return candidates_->top(); }
    inline void pop() { candidates_->pop(); }
    inline void push(CoinTreeNode* node, const bool incrInserted = true) {
	candidates_->push(node, incrInserted);
    }

    inline CoinTreeNode* bestQualityCandidate() const {
	return candidates_->top();
    }
    inline double bestQuality() const {
	return candidates_->top()->quality_;
    }
    void newSolution(double solValue);
    void reevaluateSearchStrategy();
};

//#############################################################################

#endif
