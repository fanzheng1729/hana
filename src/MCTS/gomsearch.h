#ifndef GOMSEARCH_H_INCLUDED
#define GOMSEARCH_H_INCLUDED

#include "gom.h"
#include "../util/for.h"

// Search tree for the game
template<template<class> class MCTS, std::size_t M, std::size_t N, std::size_t K>
struct GomSearchTree : MCTS<Gom<M,N,K> >
{
    typedef MCTS<Gom<M,N,K> > MCTSearch;
    using typename MCTSearch::Nodeptr;
    GomSearchTree
        (Gom<M,N,K> const & gom, Value const exploration[2]) :
            MCTSearch(gom, exploration) {}
    // Evaluate the leaf. Return {value, sure?}
    // p should != NULL.
    virtual Eval evalleaf(Nodeptr p) const
    {
        Gom<M,N,K> const & game = p->game();
        int const winner = game.winner();
        return Eval(winner, winner || game.full());
    }
    // Evaluate the parent. Return {value, sure?}
    // p should != NULL.
    virtual Eval evalparent(Nodeptr p) const
    {
        Value const value = this->minimax(p);
        // If a parent is a win or a loss then it is sure.
        if (::issure(value)) return Eval(value, true);
        // Otherwise it is sure only if all its children are sure.
        FOR (Nodeptr child, *p.children())
            if (!this->issure(child))
                return Eval(value, false);
        return Eval(value, true);
    }
};

#endif // GOMSEARCH_H_INCLUDED
