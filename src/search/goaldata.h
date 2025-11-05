#ifndef GOALDATA_H_INCLUDED
#define GOALDATA_H_INCLUDED

#include "game.h"
#include "../MCTS/MCTS.h"
#include "../types.h"

// Proof status of a goal
enum Goalstatus {GOALOPEN = 0, GOALFALSE = -1, GOALNEW = -2};

// Pointer to node in proof search tree
typedef MCTS<Game>::Nodeptr Nodeptr;
// Set of nodes in proof search tree
typedef std::set<Nodeptr> Nodeptrs;

struct Goalptrs : std::set<Goalptr>
{
    bool haschildren(Nodeptr p) const;
    Goalptr saturate();
};

// Data associated with the goal
struct Goaldata
{
    // OPEN, FALSE or NEW
    Goalstatus status;
    // Proof of the expression
    Proofsteps proofsteps;
    // Set of nodes trying to prove the open goal
    Nodeptrs nodeptrs;
    // Pointer to the different contexts where the goal is evaluated
    Goal2ptr goal2ptr;
    // Unnecessary hypothesis of the goal
    Bvector hypstotrim;
    Goaldata(Goalstatus s, Proofsteps const & steps = Proofsteps()) :
        status(s), proofsteps(steps) {}
    bool proven() const { return !proofsteps.empty(); }
};

#endif // GOALDATA_H_INCLUDED
