#ifndef CoinSearchTree_H
#define CoinSearchTree_H

#include <vector>
#include <algorithm>
#include <cmath>

#include "CoinFinite.hpp"
#include "CoinHelperFunctions.hpp"

//#############################################################################

/** A class from which the real tree nodes should be derived from. Some of the
    data that undoubtedly exist in the real tree node is replicated here for
    fast access. This class is used in the various comparison functions. */
class CoinTreeNode {
protected:
    CoinTreeNode() :
	depth_(-1),
	quality_(-COIN_DBL_MAX),
	true_lower_bound_(-COIN_DBL_MAX) {}
    CoinTreeNode(int d,
		 double q = -COIN_DBL_MAX,
		 double tlb = -COIN_DBL_MAX /*, double f*/) :
	depth_(d),
	quality_(q),
	true_lower_bound_(tlb)/*, fractionality_(f)*/ {}
    CoinTreeNode(const CoinTreeNode& x) :
	depth_(x.depth_),
	quality_(x.quality_),
	true_lower_bound_(x.true_lower_bound_) {}
    CoinTreeNode& operator=(const CoinTreeNode& x) {
	// Not worth to test (this != &x)
	depth_ = x.depth_;
	quality_ = x.quality_;
	true_lower_bound_ = x.true_lower_bound_;
	return *this;
    }
private:
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

    inline int    getDepth()   const { return depth_; }
    inline double getQuality() const { return quality_; }
    inline double getTrueLB()  const { return true_lower_bound_; }
    
    inline void setDepth(int d)       { depth_ = d; }
    inline void setQuality(double q)  { quality_ = q; }
    inline void setTrueLB(double tlb) { true_lower_bound_ = tlb; }
};

class CoinTreeSiblings {
private:
    CoinTreeSiblings();
    CoinTreeSiblings& operator=(const CoinTreeSiblings&);
private:
    int current_;
    int numSiblings_;
    CoinTreeNode** siblings_;
public:
    CoinTreeSiblings(const int n, CoinTreeNode** nodes) :
	current_(0), numSiblings_(n), siblings_(new CoinTreeNode*[n])
    {
	CoinDisjointCopyN(nodes, n, siblings_);
    }
    CoinTreeSiblings(const CoinTreeSiblings& s) :
	current_(s.current_),
	numSiblings_(s.numSiblings_),
	siblings_(new CoinTreeNode*[s.numSiblings_])
    {
	CoinDisjointCopyN(s.siblings_, s.numSiblings_, siblings_);
    }
    ~CoinTreeSiblings() { delete[] siblings_; }
    inline CoinTreeNode* currentNode() const { return siblings_[current_]; }
    /** returns false if cannot be advanced */
    inline bool advanceNode() { return ++current_ != numSiblings_; }
    inline int toProcess() const { return numSiblings_ - current_; }
    inline int size() const { return numSiblings_; }
};

//#############################################################################

/** Function objects to compare search tree nodes. The comparison function
    must return true if the first argument is "better" than the second one,
    i.e., it should be processed first. */
/*@{*/
/** Depth First Search. */
struct CoinSearchTreeCompareDepth {
    inline bool operator()(const CoinTreeSiblings* x,
			   const CoinTreeSiblings* y) const {
	return x->currentNode()->getDepth() > y->currentNode()->getDepth();
    }
};

//-----------------------------------------------------------------------------
/* Breadth First Search */
struct CoinSearchTreeCompareBreadth {
    inline bool operator()(const CoinTreeSiblings* x,
			   const CoinTreeSiblings* y) const {
	return x->currentNode()->getDepth() < y->currentNode()->getDepth();
    }
};

//-----------------------------------------------------------------------------
/** Best first search */
struct CoinSearchTreeCompareBest {
    inline bool operator()(const CoinTreeSiblings* x,
			   const CoinTreeSiblings* y) const {
	return x->currentNode()->getQuality() < y->currentNode()->getQuality();
    }
};

//#############################################################################

class CoinSearchTreeBase
{
private:
    CoinSearchTreeBase(const CoinSearchTreeBase&);
    CoinSearchTreeBase& operator=(const CoinSearchTreeBase&);

protected:
    std::vector<CoinTreeSiblings*> candidateList_;
    int numInserted_;
    int size_;

protected:
    CoinSearchTreeBase() : candidateList_(), numInserted_(0), size_(0) {}

    virtual void realpop() = 0;
    virtual void realpush(CoinTreeSiblings* s) = 0;
    virtual void fixTop() = 0;

public:
    virtual ~CoinSearchTreeBase() {}

    inline const std::vector<CoinTreeSiblings*>& getCandidates() const {
	return candidateList_;
    }
    inline bool empty() const { return candidateList_.empty(); }
    inline int size() const { return size_; }
    inline int numInserted() const { return numInserted_; }
    inline CoinTreeNode* top() const {
	return (size_ == 0) ? NULL : candidateList_.front()->currentNode();
    }
    /** pop will advance the \c next pointer among the siblings on the top and
	then moves the top to its correct position. #realpop is the method
	that actually removes the element from the heap */
    inline void pop() {
	CoinTreeSiblings* s = candidateList_.front();
	if (!s->advanceNode()) {
	    realpop();
	    delete s;
	} else {
	    fixTop();
	}
	--size_;
    }
    inline void push(int numNodes, CoinTreeNode** nodes,
		     const bool incrInserted = true) {
	CoinTreeSiblings* s = new CoinTreeSiblings(numNodes, nodes);
	realpush(s);
	if (incrInserted) {
	    numInserted_ += numNodes;
	}
	size_ += numNodes;
    }
    inline void push(const CoinTreeSiblings& sib,
		     const bool incrInserted = true) {
	CoinTreeSiblings* s = new CoinTreeSiblings(sib);
	realpush(s);
	if (incrInserted) {
	    numInserted_ += sib.toProcess();
	}
	size_ += sib.size();
    }
};

//#############################################################################

#ifdef CAN_TRUST_STL_HEAP

template <class Comp>
class CoinSearchTree : public CoinSearchTreeBase
{
private:
    Comp comp_;
protected:
    virtual void realpop() {
	candidateList_.pop();
    }
    virtual void fixTop() {
	CoinTreeSiblings* s = top();
	realpop();
	push(s, false);
    }
    virtual void realpush(CoinTreeSiblings* s) {
	nodes_.push_back(s);
	std::push_heap(candidateList_.begin(), candidateList_.end(), comp_);
    }
public:
    CoinSearchTree() : CoinSearchTreeBase(), comp_() {}
    CoinSearchTree(const CoinSearchTreeBase& t) :
	CoinSearchTreeBase(), comp_() {
	candidateList_ = t.getCandidates();
	std::make_heap(candidateList_.begin(), candidateList_.end(), comp_);
	numInserted_ = t.numInserted_;
	size_ = t.size_;
    }
    ~CoinSearchTree() {}
};

#else

template <class Comp>
class CoinSearchTree : public CoinSearchTreeBase
{
private:
    Comp comp_;

protected:
    virtual void realpop() {
	candidateList_[0] = candidateList_.back();
	candidateList_.pop_back();
	fixTop();
    }
    /** After changing data in the top node, fix the heap */
    virtual void fixTop() {
	const int size = candidateList_.size();
	if (size > 1) {
	    CoinTreeSiblings** candidates = &candidateList_[0];
	    CoinTreeSiblings* s = candidates[0];
	    --candidates;
	    int pos = 1;
	    int ch;
	    for (ch = 2; ch < size; pos = ch, ch *= 2) {
		if (comp_(candidates[ch+1], candidates[ch]))
		    ++ch;
		if (comp_(s, candidates[ch]))
		    break;
		candidates[pos] = candidates[ch];
	    }
	    if (ch == size) {
		if (comp_(candidates[ch], s)) {
		    candidates[pos] = candidates[ch];
		    pos = ch;
		}
	    }
	    candidates[pos] = s;
	}
    }
    virtual void realpush(CoinTreeSiblings* s) {
	candidateList_.push_back(s);
	CoinTreeSiblings** candidates = &candidateList_[0];
	--candidates;
	int pos = candidateList_.size();
	int ch;
	for (ch = pos/2; ch != 0; pos = ch, ch /= 2) {
	    if (comp_(candidates[ch], s))
		break;
	    candidates[pos] = candidates[ch];
	}
	candidates[pos] = s;
    }
  
public:
    CoinSearchTree() : CoinSearchTreeBase(), comp_() {}
    CoinSearchTree(const CoinSearchTreeBase& t) :
	CoinSearchTreeBase(), comp_() {
	candidateList_ = t.getCandidates();
	std::sort(candidateList_.begin(), candidateList_.end(), comp_);
	numInserted_ = t.numInserted();
	size_ = t.size();
    }
    ~CoinSearchTree() {}
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
	candidates_->push(1, &node, incrInserted);
    }
    inline void push(const CoinTreeSiblings& s, const bool incrInserted=true) {
	candidates_->push(s, incrInserted);
    }
    inline void push(const int n, CoinTreeNode** nodes,
		     const bool incrInserted = true) {
	candidates_->push(n, nodes, incrInserted);
    }

    inline CoinTreeNode* bestQualityCandidate() const {
	return candidates_->top();
    }
    inline double bestQuality() const {
	return candidates_->top()->getQuality();
    }
    void newSolution(double solValue);
    void reevaluateSearchStrategy();
};

//#############################################################################

#endif
