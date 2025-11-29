#ifndef GOALDATA_H_INCLUDED
#define GOALDATA_H_INCLUDED

#include <algorithm>    // for std::lower_bound
#include "game.h"
#include "goalstat.h"
#include "../MCTS/MCTS.h"
#include "../types.h"
#include "../util/for.h"

// Pointer to node in proof search tree
typedef MCTS<Game>::pNode pNode;
// Set of pointers of nodes in proof search tree
typedef std::set<pNode> pNodes;

// Polymorphic context, with move generation and goal evaluation
struct Environ;
// Pointers to contexts
typedef std::vector<Environ const *> pEnvs;

static const std::less<Environ const *> less;

// Return true if the context is a sub-context of the problem context
bool subsumedbyProb(Environ const & env);
// Return sub-contexts of env.
pEnvs const & subEnvs(Environ const & env);
// Return super-contexts of env.
pEnvs const & supEnvs(Environ const & env);

// Data associated with the goal
class Goaldata
{
    friend class Problem;
    Goalstatus status;
    Proofsteps proof;
    // Set of pointers to nodes trying to prove the open goal
    pNodes m_pnodes;
    // Pointer to the context
    Environ const * pEnv;
    // Pointer to the different contexts where the goal is evaluated
    pBIGGOAL pbigGoal;
public:
    // New context after trimming unnecessary hypotheses
    Environ const * pnewEnv;
    Goaldata(Goalstatus s, Environ const * envptr, pBIGGOAL bigpGoal) :
        status(s), pEnv(envptr), pbigGoal(bigpGoal), pnewEnv(NULL) {}
    Goal const & goal() const { return pbigGoal->first; }
    Goaldatas & goaldatas() const { return pbigGoal->second; }
    Proofsteps const & proofsrc() const
    { return goaldatas().proven() ? goaldatas().proof : proof; }
    Proofsteps const & proofsrc()
    {
        Proofsteps const & proof0
        = const_cast<Goaldata const *>(this)->proofsrc();
        if (!proof0.empty()) return proof0;
        if (subsumedbyProb(*pEnv)) return proof;

        // Sub-contexts of env
        pEnvs const & psubEnvs = subEnvs(*pEnv);
        pEnvs::const_iterator subiter = psubEnvs.begin();
        pEnvs::const_iterator const subend = psubEnvs.end();
        
        // Loop through sub-contexts.
        FOR (Goaldatas::const_reference goaldata, goaldatas())
            if (!goaldata.second.proof.empty() && !subsumedbyProb(*goaldata.first))
            {
                Environ const & otherEnv = *goaldata.first;
                subiter = std::lower_bound(subiter, subend, &otherEnv, less);
                // while (subiter != subend && less(*subiter, &otherEnv))
                //     ++subiter;
                if (subiter != subend && *subiter == &otherEnv)
                    return proof = goaldata.second.proof;
                if (subiter == subend) break;
            }
        
        return proof;
    }
    bool proven() const { return !proofsrc().empty(); }
    bool proven(Environ const & env) { return !proofsrc().empty(); }
    Proofsteps & proofdst()
    { return subsumedbyProb(*pEnv) ? goaldatas().proof : proof; }
    pNodes const & pnodes() const { return m_pnodes; }
    // Add node pointer to p's goal data.
    friend void addpNode(pNode p)
    {
        if (p->game().proven()) return;
        p->game().goaldata().m_pnodes.insert(p);
    }
    // Add simplified goal. Return its pointer. Return pgoal if unsuccessful.
    friend pGoal addsimpgoal(pGoal pgoal)
    {
        if (!pgoal) return pgoal;
        Environ const * const psimpEnv = pgoal->second.pnewEnv;
        if (!psimpEnv) return pgoal;
        pBIGGOAL const pbigGoal = pgoal->second.pbigGoal;
        if (!pbigGoal) return pgoal;
        Goaldatas::value_type const value
            (psimpEnv, Goaldata(GOALTRUE, psimpEnv, pbigGoal));
        return &*pbigGoal->second.insert(value).first;
    }
    void setstatustrue() { status = GOALTRUE; }
    Goalstatus getstatus() const { return status; }
    Goalstatus & getstatus()
    {
        if (proven(*pEnv))
            return status = GOALTRUE;
        if (status != GOALNEW)
            return status; // No need to evaluate

        // Sub and super contexts of env
        pEnvs const & psubEnvs = subEnvs(*pEnv);
        pEnvs const & psupEnvs = supEnvs(*pEnv);
        pEnvs::const_iterator subiter = psubEnvs.begin();
        pEnvs::const_iterator const subend = psubEnvs.end();
        pEnvs::const_iterator supiter = psupEnvs.begin();
        pEnvs::const_iterator const supend = psupEnvs.end();

        FOR (Goaldatas::const_reference goaldata, goaldatas())
        {
            Environ const & otherEnv = *goaldata.first;
            Goaldata const & otherdata = goaldata.second;

            if (&otherEnv == pEnv)
                continue;

            if (otherdata.status == GOALFALSE && supiter != supend)
            {
                supiter = std::lower_bound(supiter, supend, &otherEnv, less);
                // while (supiter != supend && less(*supiter, &otherEnv))
                //     ++supiter;
                if (supiter != supend && *supiter == &otherEnv)
                    return status = GOALFALSE;
            }

            if (otherdata.status == GOALTRUE && subiter != subend)
            {
                subiter = std::lower_bound(subiter, subend, &otherEnv, less);
                // while (subiter != subend && less(*subiter, &otherEnv))
                //     ++subiter;
                if (subiter != subend && *subiter == &otherEnv)
                {
                    pnewEnv = otherdata.pnewEnv;
                    if (!pnewEnv) pnewEnv = &otherEnv;
                    return status = GOALTRUE;
                }
            }
        }

        return status;
    }
};

#endif // GOALDATA_H_INCLUDED
