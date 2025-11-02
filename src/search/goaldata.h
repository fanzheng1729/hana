#ifndef GOALDATA_H_INCLUDED
#define GOALDATA_H_INCLUDED

#include "game.h"
#include "goalstat.h"
#include "../MCTS/MCTS.h"
#include "../types.h"

// Pointer to node in proof search tree
typedef MCTS<Game>::Nodeptr Nodeptr;
// Set of nodes in proof search tree
struct Nodeptrs : std::set<Nodeptr>
{
    // Return the ancestor of a node.
    // Return NULL if there is not any.
    Nodeptr ancestor(Nodeptr p) const
    {
        FOR (Nodeptr other, *this)
            if (other.isancestorof(p))
                return other;
        return Nodeptr();
    }
    // Return a node that can be inferred.
    // Return NULL if there is not any.
    Nodeptr pop();
};

// Data associated with the goal
struct Goaldata
{
    // OPEN, FALSE or NEW
    Goalstatus status;
    // Proof of the expression
    Proofsteps proofsteps;
    // Set of nodes trying to prove the open goal.
    Nodeptrs nodeptrs;
    Goaldata(Goalstatus s, Proofsteps const & steps = Proofsteps()) :
        status(s), proofsteps(steps) {}
    bool proven() const { return !proofsteps.empty(); }
    // Unnecessary hypothesis of the goal
    Bvector hypstotrim;
};

#endif // GOALDATA_H_INCLUDED
