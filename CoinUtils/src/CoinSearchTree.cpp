#include "CoinSearchTree.hpp"

void
CoinSearchTreeManager::newSolution(double solValue)
{
    ++numSolution;
    hasUB_ = true;
    CoinTreeNode* top = candidates_->top();
    const double q = top ? top->quality_ : solValue;
    const bool switchToDFS = fabs(q) < 1e-3 ?
	(fabs(solValue) < 0.005) : ((solValue-q)/fabs(q) < 0.005);
    if (switchToDFS &&
	dynamic_cast<CoinSearchTree<CoinSearchTreeCompareDepth>*>(candidates_) == NULL) {
	CoinSearchTree<CoinSearchTreeCompareDepth>* cands =
	    new CoinSearchTree<CoinSearchTreeCompareDepth>(*candidates_);
	delete candidates_;
	candidates_ = cands;
    }
}

void
CoinSearchTreeManager::reevaluateSearchStrategy()
{
    const int n = candidates_->numInserted() % 1000;
    /* the tests below ensure that even if this method is not invoked after
       every push(), the search strategy will be reevaluated when n is ~500 */
    if (recentlyReevaluatedSearchStrategy_) {
	if (n > 250 && n <= 500) {
	    recentlyReevaluatedSearchStrategy_ = false;
	}
    } else {
	if (n > 500) {
	    recentlyReevaluatedSearchStrategy_ = true;
	    /* we can reevaluate things... */
	}
    }
}
