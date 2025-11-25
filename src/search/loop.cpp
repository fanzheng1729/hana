#include "goaldata.h"

// If goal appears as the goal of a node or its ancestors,
// return the pointer of the ancestor.
// This check is necessary to prevent self-assignment in writeproof().
static Nodeptr cycles(Goaldataptr pGoal, Nodeptr pNode)
{
    while (true)
    {
        Game const & game = pNode->game();
        if (game.nDefer == 0 && pGoal == game.pGoal)
            return pNode;
        if (Nodeptr const parent = pNode.parent())
            pNode = parent.parent();
        else break;
    }
    return Nodeptr();
}

namespace
{
// Set of goal pointers
struct Goaldataptrs : std::set<Goaldataptr>
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
    Goaldataptr saturate()
    {
        FOR (Goaldataptr const pGoal, *this)
            FOR (Nodeptr const pNode, pGoal->second.nodeptrs)
            {
                Nodeptr const parent = pNode.parent();
                if (parent->game().proven())
                    continue;
                Goaldataptr const newGoal = parent->game().pGoal;
                if (haschildren(parent) && insert(newGoal).second)
                    return newGoal;
            }
        return Goaldataptr();
    }
};
}

// Return true if ptr duplicates upstream goals.
bool loops(Nodeptr p)
{
    Move const & move = p->game().attempt;
    // All the goals necessary to prove p
    Goaldataptrs allgoals;
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
    while (Goaldataptr const pnewgoal = allgoals.saturate())
        FOR (Nodeptr const pnewnode, pnewgoal->second.nodeptrs)
            if (pnewnode.isancestorof(p))
                return true;
    return false;
}
