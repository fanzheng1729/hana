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
    // Proof of the expression
    Proofsteps proof;
    Proofsteps const & proofsrc() const
    { return goaldatas().proven() ? goaldatas().proof : proof; }
    // Set of nodes trying to prove the open goal
    Nodeptrs nodeptrs;
    // Pointer to the different contexts where the goal is evaluated
    BigGoalptr pBigGoal;
    // New context after trimming unnecessary hypotheses
    Environ * pnewEnv;
    Goaldata(Goalstatus s, BigGoalptr p = NULL) :
        status(s), pBigGoal(p), pnewEnv(NULL) {}
    bool proven() const { return !proofsrc().empty(); }
    Goal const & goal() const { return pBigGoal->first; }
    Goaldatas & goaldatas() const { return pBigGoal->second; }
    Goalstatus & getstatus()
    {
        if (proven())
            return status = GOALTRUE;
        return status;
    }
    void setstatustrue() { status = GOALTRUE; }
private:
    Goalstatus status;
    friend class Problem;
    // Return true if a new proof is set.
    bool setproof(Proofsteps const & prf)
    {
        if (proven() || prf.empty()) return false;
        proof = prf;
        return status = GOALTRUE;
    }
};

// Add simplified goal. Return its pointer. Return pGoal if unsuccessful.
inline Goalptr addsimpGoal(Goalptr pGoal)
{
    if (!pGoal) return pGoal;
    Environ * const pnewEnv = pGoal->second.pnewEnv;
    if (!pnewEnv) return pGoal;
    Goaldata & goaldata = pGoal->second;
    BigGoalptr const pBigGoal = goaldata.pBigGoal;
    if (!pBigGoal) return pGoal;
    Goaldatas::value_type value(pnewEnv, Goaldata(goaldata.getstatus(), pBigGoal));
    return &*pBigGoal->second.insert(value).first;
}

// Add node pointer to p's goal data.
inline void addNodeptr(Nodeptr p)
{
    if (p->game().proven()) return;
    p->game().goaldata().nodeptrs.insert(p);
}

#endif // GOALDATA_H_INCLUDED
