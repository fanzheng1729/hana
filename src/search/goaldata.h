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

// Polymorphic context
struct Environ;

// Data associated with the goal
class Goaldata
{
    Goalstatus status;
    Proofsteps proof;
    // Set of nodes trying to prove the open goal
    Nodeptrs m_nodeptrs;
    // Pointer to the different contexts where the goal is evaluated
    BigGoalptr pBigGoal;
public:
    // New context after trimming unnecessary hypotheses
    Environ * pnewEnv;
    Goaldata(Goalstatus s, BigGoalptr p = NULL) :
        status(s), pBigGoal(p), pnewEnv(NULL) {}
    Goal const & goal() const { return pBigGoal->first; }
    Goaldatas & goaldatas() const { return pBigGoal->second; }
    Proofsteps const & proofsrc() const
    { return goaldatas().proven() ? goaldatas().proof : proof; }
    Proofsteps const & proofsrc(Environ const & env)
    {
        if (!proofsrc().empty()) return proofsrc();
        bool issubProb(Environ const & env);
        if (issubProb(env)) return proofsrc();
        // Loop through sub-contexts.
        bool reachable(Environ const & from, Environ const & to);
        FOR (Goaldatas::const_reference goaldata, goaldatas())
            if (goaldata.first && reachable(env, *goaldata.first))
                return proof = goaldata.second.proof;
    }
    bool proven() const { return !proofsrc().empty(); }
    bool proven(Environ const & env) { return !proofsrc(env).empty(); }
    Proofsteps & proofdst(Environ const & env)
    {
        bool issubProb(Environ const & env);
        return issubProb(env) ? goaldatas().proof : proof;
    }
    Nodeptrs const & nodeptrs() const { return m_nodeptrs; }
    // Add node pointer to p's goal data.
    friend void addNodeptr(Nodeptr p)
    {
        if (p->game().proven()) return;
        p->game().goaldata().m_nodeptrs.insert(p);
    }
    // Add simplified goal. Return its pointer. Return pGoal if unsuccessful.
    friend Goalptr addsimpGoal(Goalptr pGoal)
    {
        if (!pGoal) return pGoal;
        Environ * const pnewEnv = pGoal->second.pnewEnv;
        if (!pnewEnv) return pGoal;
        BigGoalptr const pBigGoal = pGoal->second.pBigGoal;
        if (!pBigGoal) return pGoal;
        Goaldatas::value_type value(pnewEnv, Goaldata(GOALTRUE, pBigGoal));
        return &*pBigGoal->second.insert(value).first;
    }
    void setstatustrue() { status = GOALTRUE; }
    Goalstatus getstatus() const { return status; }
    Goalstatus & getstatus(Environ & env)
    {
        if (proven())
            return status = GOALTRUE;
        if (status != GOALNEW)
            return status; // No need to evaluate
        bool reachable(Environ const & from, Environ const & to);
        Environ * pnewEnv2 = &env;
        FOR (Goaldatas::const_reference goaldata, goaldatas())
        {
            if (!goaldata.first)
                continue;
            if (reachable(*goaldata.first, env) &&
                goaldata.second.status == GOALFALSE)
                return status = GOALFALSE;
            if (reachable(env, *goaldata.first) &&
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
};

#endif // GOALDATA_H_INCLUDED
