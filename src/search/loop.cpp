#include "goaldata.h"

// If goal appears as the goal of a node or its ancestors,
// return the pointer of the ancestor.
// This check is necessary to prevent self-assignment in writeproof().
static Nodeptr cycles(Goalptr pgoal, Nodeptr pnode)
{
    while (true)
    {
        Game const & game = pnode->game();
        if (game.nDefer == 0 && pgoal == game.pGoal)
            return pnode;
        if (Nodeptr const parent = pnode.parent())
            pnode = parent.parent();
        else break;
    }
    return Nodeptr();
}

namespace
{
// Set of goal pointers
struct Goalptrs : std::set<Goalptr>
{
    // Return true if all open children of p are present.
    bool haschildren(Nodeptr p) const
    {
        FOR (Nodeptr const child, *p.children())
        {
            if (child->game().proven())
                continue;
            if (!count(child->game().pGoal))
                return false;
        }
        return true;
    }
    // Return pointer to a new goal implied by the existing goals.
    // Return NULL if there is no such goal.
    Goalptr saturate()
    {
        FOR (Goalptr const pgoal, *this)
            FOR (Nodeptr const pnode, pgoal->second.pnodes())
            {
                Nodeptr const parent = pnode.parent();
                if (parent->game().proven())
                    continue;
                Goalptr const pnewgoal = parent->game().pGoal;
                if (haschildren(parent) && insert(pnewgoal).second)
                    return pnewgoal;
            }
        return Goalptr();
    }
};
}

// Return true if ptr duplicates upstream goals.
bool loops(Nodeptr p)
{
    Move const & move = p->game().attempt;
    // All the goals necessary to prove p
    Goalptrs allgoals;
    // Check if any of the hypotheses appears in a parent node.
    for (Hypsize i = 0; i < move.hypcount(); ++i)
    {
        if (move.hypfloats(i))
            continue;
        if (move.hypvec[i]->second.proven(*move.hypvec[i]->first))
            continue;
        if (cycles(move.hypvec[i], p.parent()))
            return true;
        allgoals.insert(move.hypvec[i]);
    }
    // Check if these hypotheses combined prove a parent node.
    while (Goalptr const pGoal = allgoals.saturate())
        FOR (Nodeptr const pnewNode, pGoal->second.pnodes())
            if (pnewNode.isancestorof(p))
                return true;
    return false;
}
