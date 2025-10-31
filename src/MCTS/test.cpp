#include <cstddef>  // for std::size_t and NULL
#include <iostream>
#include "MCTS.h"
#include "../util/timer.h"

// Play out until the value is sure or size limit is reached.
// Return the value.
template<class Tree>
static Value playgame(Tree & tree, std::size_t maxsize)
{
    Timer timer;
    tree.play(maxsize);
    // Collect statistics.
    Time const t = timer;
    if (tree.size() > maxsize)
        std::cerr << "Tree size limit exceeded. ";
    std::cout << tree.size() << " nodes / " << t << "s = ";
    std::cout << tree.size()/t << " nps" << std::endl;
    return tree.value();
}

#include "gomsearch.h"
template<std::size_t M, std::size_t N, std::size_t K>
static Value playgom(int const p[], std::size_t maxsize,
                     Value const exploration[2])
{
    GomSearchTree<MCTS, M,N,K> tree(Gom<M,N,K>(p), exploration);
    return playgame(tree, maxsize);
}

// Check Monte Carlo tree search.
bool testMCTS(std::size_t maxsize, double const exploration[2])
{
    std::cout << "Playing Gom." << std::endl;
    if (playgom<2,2,2>(NULL, maxsize, exploration) != WDL::WIN)
        return false;
    if (playgom<3,3,3>(NULL, maxsize, exploration) != WDL::DRAW)
        return false;
    int a[] = {0, 0, 0, -1};
    if (playgom<2,2,2>(a, maxsize, exploration) != WDL::LOSS)
        return false;

    return true;
}
