#ifndef GOALDATA_H_INCLUDED
#define GOALDATA_H_INCLUDED

#include <algorithm>    // for std::lower_bound
#include "game.h"
#include "goalstat.h"
#include "../MCTS/MCTS.h"
#include "../types.h"
#include "../util/for.h"

// Pointer to node in proof search tree
typedef MCTS<Game>::Nodeptr Nodeptr;
// Set of nodes in proof search tree
typedef std::set<Nodeptr> Nodeptrs;

// Polymorphic context, with move generation and goal evaluation
struct Environ;
// Pointers to contexts
typedef std::vector<Environ const *> pEnvs;

// Return true if the context is a sub-context of the problem context
bool issubProb(Environ const & env);
// Return sub-contexts of env.
pEnvs const & subEnvs(Environ const & env);
// Return super-contexts of env.
pEnvs const & supEnvs(Environ const & env);
// Return true if from implies to.
bool implies(Environ const & from, Environ const & to);

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
    Environ const * pnewEnv;
    Goaldata(Goalstatus s, BigGoalptr p = NULL) :
        status(s), pBigGoal(p), pnewEnv(NULL) {}
    Goal const & goal() const { return pBigGoal->first; }
    Goaldatas & goaldatas() const { return pBigGoal->second; }
    Proofsteps const & proofsrc() const
    { return goaldatas().proven() ? goaldatas().proof : proof; }
    Proofsteps const & proofsrc(Environ const & env)
    {
        if (!proofsrc().empty()) return proofsrc();
        if (issubProb(env)) return proof;
        return proof;
        // Loop through sub-contexts.
        FOR (Goaldatas::const_reference goaldata, goaldatas())
            if (!goaldata.second.proof.empty() &&
                !issubProb(*goaldata.first) &&
                implies(env, *goaldata.first))
                return proof = goaldata.second.proof;
        return proof;
    }
    bool proven() const { return !proofsrc().empty(); }
    bool proven(Environ const & env) { return !proofsrc(env).empty(); }
    Proofsteps & proofdst(Environ const & env)
    { return issubProb(env) ? goaldatas().proof : proof; }
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
        Environ const * const pnewEnv = pGoal->second.pnewEnv;
        if (!pnewEnv) return pGoal;
        BigGoalptr const pBigGoal = pGoal->second.pBigGoal;
        if (!pBigGoal) return pGoal;
        Goaldatas::value_type value(pnewEnv, Goaldata(GOALTRUE, pBigGoal));
        return &*pBigGoal->second.insert(value).first;
    }
    void setstatustrue() { status = GOALTRUE; }
    Goalstatus getstatus() const { return status; }
    Goalstatus & getstatus(Environ const & env)
    {
        if (proven(env))
            return status = GOALTRUE;
        if (status != GOALNEW)
            return status; // No need to evaluate

        // Sub and super contexts of env
        pEnvs const & psubEnvs = subEnvs(env), & psupEnvs = supEnvs(env);
        pEnvs::const_iterator subiter = psubEnvs.begin();
        pEnvs::const_iterator const subend = psubEnvs.end();
        pEnvs::const_iterator supiter = psupEnvs.begin();
        pEnvs::const_iterator const supend = psupEnvs.end();

        FOR (Goaldatas::const_reference goaldata, goaldatas())
        {
            Environ const & other = *goaldata.first;
            if (&other == &env)
                continue;

            if (goaldata.second.status == GOALFALSE)
            {
                supiter = std::lower_bound(supiter, supend, &other);
                if (supiter != supend && *supiter == &other)
                    return status = GOALFALSE;
                // if (implies(other, env))
                //     return status = GOALFALSE;
            }

            if (goaldata.second.status == GOALTRUE && implies(env, other))
            {
                pnewEnv = goaldata.second.pnewEnv;
                if (!pnewEnv) pnewEnv = &other;
                return status = GOALTRUE;
            }
        }

        return status;
    }
};

#endif // GOALDATA_H_INCLUDED
