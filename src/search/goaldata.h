#ifndef GOALDATA_H_INCLUDED
#define GOALDATA_H_INCLUDED

#include "game.h"
#include "goalstat.h"
#include "../MCTS/MCTS.h"
#include "../types.h"

// Pointer to node in proof search tree
typedef MCTS<Game>::Nodeptr Nodeptr;

// Data associated with the goal
struct Goaldata
{
    Goalstatus status;
    // Proof of the expression
    Proofsteps proofsteps;
    Goaldata(Goalstatus s, Proofsteps const & steps = Proofsteps()) :
        status(s), proofsteps(steps) {}
    bool proven() const { return !proofsteps.empty(); }
    // Unnecessary hypothesis of the goal
    Bvector hypstotrim;
};

#endif // GOALDATA_H_INCLUDED
