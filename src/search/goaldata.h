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
    Proofsteps const & getproof() const
    { return goaldatas().proven() ? goaldatas().proof : proof; }
    // Set of nodes trying to prove the open goal
    Nodeptrs nodeptrs;
    // Pointer to the different contexts where the goal is evaluated
    BigGoalptr pBigGoal;
    // New context after trimming unnecessary hypotheses
    Environ * pnewEnv;
    Goaldata(Goalstatus s, BigGoalptr p = NULL) :
        status(s), pBigGoal(p), pnewEnv(NULL) {}
    bool proven() const { return !getproof().empty(); }
    Goal const & goal() const { return pBigGoal->first; }
    Goaldatas & goaldatas() const { return pBigGoal->second; }
    Goalstatus getstatus() const { return status; }
    Goalstatus & getstatus(struct Environ * p)
    {
        if (proven())
            return status = GOALTRUE;
        if (!p || status != GOALNEW)
            return status; // No need to evaluate
        bool reachable(Environ const & from, Environ const & to);
        Environ * pnewEnv2 = p;
        FOR (Goaldatas::const_reference goaldata, goaldatas())
        {
            if (!goaldata.first)
                continue;
            if (reachable(*goaldata.first, *p) &&
                goaldata.second.status == GOALFALSE)
                return status = GOALFALSE;
            if (reachable(*p, *goaldata.first) &&
                goaldata.second.status == GOALTRUE)
            {
                if (reachable(*pnewEnv2, *goaldata.first))
                    pnewEnv2 = goaldata.first;
                status = GOALTRUE;
            }
        }
        if (status == GOALTRUE)
            pnewEnv = pnewEnv2;
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
        setstatustrue();
        return true;
    }
};

// Add simplified goal. Return its pointer. Return pGoal if unsuccessful.
inline Goalptr addsimpGoal(Goalptr pGoal)
{
    if (!pGoal) return pGoal;
    Environ * const pnewEnv = pGoal->second.pnewEnv;
    if (!pnewEnv) return pGoal;
    BigGoalptr const pBigGoal = pGoal->second.pBigGoal;
    if (!pBigGoal) return pGoal;
    Goaldatas::value_type value(pnewEnv, Goaldata(GOALTRUE, pBigGoal));
    return &*pBigGoal->second.insert(value).first;
}

// Add node pointer to p's goal data.
inline void addNodeptr(Nodeptr p)
{
    if (p->game().proven()) return;
    p->game().goaldata().nodeptrs.insert(p);
}

#endif // GOALDATA_H_INCLUDED
