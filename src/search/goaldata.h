#ifndef GOALDATA_H_INCLUDED
#define GOALDATA_H_INCLUDED

#include "game.h"
#include "goalstat.h"
#include "../MCTS/MCTS.h"
#include "../types.h"
#include "../util/for.h"

// Pointer to node in proof search tree
typedef MCTS<Game>::Nodeptr Nodeptr;
// Set of nodes in proof search tree
typedef std::set<Nodeptr> Nodeptrs;

// Data associated with the goal
struct Goaldata
{
    Goalstatus status;
    // Proof of the expression
    Proofsteps proof;
    // Set of nodes trying to prove the open goal
    Nodeptrs nodeptrs;
    // Pointer to the different contexts where the goal is evaluated
    BigGoalptr pBigGoal;
    // New context after trimming unnecessary hypotheses
    Environ * pnewEnv;
    Goaldata(Goalstatus s, BigGoalptr bigGoalptr = NULL) :
        status(s), pBigGoal(bigGoalptr), pnewEnv(NULL) {}
    bool proven() const { return !proof.empty(); }
};

// Add a (pEnv, Goaldata). Return its pointer.
// In case of failure, return pGoaldata.
inline Goaldataptr addGoaldata(Goaldataptr pGoaldata, Environ * pEnv)
{
    if (!pGoaldata || !pEnv) return pGoaldata;
    Goaldata const & goaldata = pGoaldata->second;
    BigGoalptr const pBigGoal = goaldata.pBigGoal;
    if (!pBigGoal) return pGoaldata;
    Goaldatas::value_type value(pEnv, Goaldata(goaldata.status, pBigGoal));
    return &*pBigGoal->second.insert(value).first;
}

// Add node pointer to p's goal data.
inline void addNodeptr(Nodeptr p)
{
    if (!p->game().proven())
        p->game().goaldata().nodeptrs.insert(p);
}

#endif // GOALDATA_H_INCLUDED
