#ifndef COINCLIQUELIST_H
#define COINCLIQUELIST_H

#include <cstddef>

/**
 * CoinCliqueList : a sequential list of cliques
 *
 *    optionally computes occurrence of nodes in cliques
 */
class CoinCliqueList
{
public:
  /**
    * Default constructor
    */
  CoinCliqueList( size_t _iniClqCap, size_t _iniClqElCap );

  void addClique( size_t size, const size_t els[] );

  size_t nCliques() const;

  size_t cliqueSize( size_t idxClq ) const;

  const size_t *cliqueElements( size_t idxClq ) const;

  // total number of elements in all cliques
  size_t totalElements() const;

  // computes data structures indicating in which clique each node appears
  void computeNodeOccurrences( size_t nNodes );

  size_t nNodeOccurrences( size_t idxNode ) const;

  const size_t *nodeOccurrences( size_t idxNode) const;

  size_t nDifferentNodes() const;

  const size_t *differentNodes() const;

  /**
    * Destructor
    */
  virtual ~CoinCliqueList();
private:
  size_t nCliques_;
  size_t cliquesCap_;

  size_t nCliqueElements_;
  size_t nCliqueElCap_;

  size_t *clqStart_;
  size_t *clqSize_;
  size_t *clqEls_;

  // only filled if computeNodeOccurrences is called
  size_t *nodeOccur_;
  size_t *startNodeOccur_;

  size_t nDifferent_;
  size_t *diffNodes_;
};

#endif // COINCLIQUELIST_H
