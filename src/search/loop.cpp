#include "goaldata.h"

// If goal appears as the goal of a node or its ancestors,
// return the pointer of the ancestor.
// This check is necessary to prevent self-assignment in writeproof().
static Nodeptr cycles(Goalptr pgoal, Nodeptr pnode)
{
    while (true)
    {
        Game const & game = pnode->game();
        if (game.ndefer == 0 && pgoal == game.pgoal)
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
            if (!count(child->game().pgoal))
                return false;
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
        Goalptr const pgoal = move.hypvec[i];
        if (pgoal->second.proven())
            continue;
        if (cycles(pgoal, p.parent()))
            return true;
        allgoals.insert(pgoal);
    }
    // Check if these hypotheses combined prove a parent node.
    while (Goalptr const pnewgoal = allgoals.saturate())
        FOR (Nodeptr const pnewnode, pnewgoal->second.nodeptrs)
            if (pnewnode.isancestorof(p))
                return true;
    return false;
}
