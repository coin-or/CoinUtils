#include "CoinSearchTree.hpp"

void
CoinSearchTreeManager::newSolution(double solValue) {
    ++numSolution;
    const double q = tree_->top()->quality_;
    const bool switchToDFS = fabs(q) < 1e-3 ?
	(fabs(solValue) < 0.005) : ((solValue-q)/fabs(q) < 0.005);
    if (switchToDFS &&
	dynamic_cast<CoinSearchTree<CoinSearchTreeCompareDepth>*>(tree_) == NULL) {
	CoinSearchTree<CoinSearchTreeCompareDepth>* newtree =
	    new CoinSearchTree<CoinSearchTreeCompareDepth>(*tree_);
	delete tree_;
	tree_ = newtree;
    }
}

void CoinSearchTreeManager::reevaluateSearchStrategy() {
    const int n = tree_->numInserted();
    if ((n % 1000) == 1) {
	/* we can reevaluate things... */
    }
}
