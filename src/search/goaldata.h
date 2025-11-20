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
    Proofsteps proof;
    // Set of nodes trying to prove the open goal
    Nodeptrs nodeptrs;
    // Pointer to the different contexts where the goal is evaluated
    Goal2ptr goal2ptr;
    // Unnecessary hypotheses of the goal
    Bvector hypstotrim;
    // New context after trimming unnecessary hypotheses
    Environ * pnewenv;
    Goaldata(Goalstatus s) : status(s), goal2ptr(NULL), pnewenv(NULL) {}
    bool proven() const { return !proof.empty(); }
};

#endif // GOALDATA_H_INCLUDED
