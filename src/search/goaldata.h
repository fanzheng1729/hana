#ifndef GOALDATA_H_INCLUDED
#define GOALDATA_H_INCLUDED

#include "game.h"
#include "../MCTS/MCTS.h"
#include "../types.h"
#include "../util/for.h"

// Pointer to node in proof search tree
typedef MCTS<Game>::Nodeptr Nodeptr;
// Set of nodes in proof search tree
typedef std::set<Nodeptr> Nodeptrs;

// Proof status of a goal
enum Goalstatus {GOALNEW = -2, GOALFALSE, GOALOPEN, GOALTRUE};
// Data associated with the goal
struct Goaldata
{
    Goalstatus status;
    // Proof of the expression
    Proofsteps proof;
    // Set of nodes trying to prove the open goal
    Nodeptrs nodeptrs;
    // Pointer to the different contexts where the goal is evaluated
    Goal2ptr goal2ptr;
    // New context after trimming unnecessary hypotheses
    Environ * pnewenv;
    Goaldata(Goalstatus s) : status(s), goal2ptr(NULL), pnewenv(NULL) {}
    bool proven() const { return !proof.empty(); }
};

// Set of goal pointers
struct Goalptrs : std::set<Goalptr>
{
    // Return true if all open children of p are present.
    bool haschildren(Nodeptr p) const
    {
        FOR (Nodeptr const child, *p.children())
        {
            if (child->game().proven()) continue;
            if (!count(child->game().pgoal)) return false;
        }
        return true;
    }
    // Return pointer to a new goal implied by the existing goals.
    // Return NULL if there is no such goal.
    Goalptr saturate()
    {
        FOR (Goalptr const pgoal, *this)
            FOR (Nodeptr const pnode, pgoal->second.nodeptrs)
            {
                Nodeptr const parent = pnode.parent();
                if (parent->game().proven())
                    continue;
                Goalptr const newgoal = parent->game().pgoal;
                if (haschildren(parent) && insert(newgoal).second)
                    return newgoal;
            }
        return Goalptr();
    }
};

#endif // GOALDATA_H_INCLUDED
