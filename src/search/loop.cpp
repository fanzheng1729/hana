#include "goaldata.h"

// If goal appears as the goal of a node or its ancestors,
// return the pointer of the ancestor.
// This check is necessary to prevent self-assignment in writeproof().
static pNode cycles(pGoal pgoal, pNode pnode)
{
    while (true)
    {
        Game const & game = pnode->game();
        if (game.nDefer == 0 && pgoal == game.pgoal)
            return pnode;
        if (pNode const parent = pnode.parent())
            pnode = parent.parent();
        else break;
    }
    return pNode();
}

namespace
{
// Set of pointers to goals
struct pGoals : std::set<pGoal>
{
    // Return true if all open children of p are present.
    bool hasallchildren(pNode p) const
    {
        FOR (pNode const child, *p.children())
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
    pGoal saturate()
    {
        FOR (pGoal const pgoal, *this)
            FOR (pNode const pnode, pgoal->second.pnodes())
            {
                pNode const parent = pnode.parent();
                if (parent->game().proven())
                    continue;
                pGoal const pnewgoal = parent->game().pgoal;
                if (hasallchildren(parent) && insert(pnewgoal).second)
                    return pnewgoal;
            }
        return pGoal();
    }
};
}

// Return true if ptr duplicates upstream goals.
bool loops(pNode p)
{
    Move const & move = p->game().attempt;
    // All the goals necessary to prove p
    pGoals allgoals;
    // Check if any of the hypotheses appears in a parent node.
    for (Hypsize i = 0; i < move.hypcount(); ++i)
    {
        if (move.hypfloats(i))
            continue;
        if (move.hypvec[i]->second.proven())
            continue;
        if (cycles(move.hypvec[i], p.parent()))
            return true;
        allgoals.insert(move.hypvec[i]);
    }
    // Check if these hypotheses combined prove a parent node.
    while (pGoal const pgoal = allgoals.saturate())
        FOR (pNode const pnewnode, pgoal->second.pnodes())
            if (pnewnode.isancestorof(p))
                return true;
    return false;
}
