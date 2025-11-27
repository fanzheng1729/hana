#ifndef PROBLEM_H_INCLUDED
#define PROBLEM_H_INCLUDED

#include "../DAG.h"
#include "environ.h"
#include "goaldata.h"
#include "../util/for.h"

// Problem statement + Proof search tree with loop detection
// + context management + UI
class Problem : public MCTS<Game>
{
    // Assertions corresponding to contexts
    Assertions assertions;
    // DAG of polymorphic contexts
    DAG<Environs> environs;
    // Map: goal -> context -> evaluation
    Goals goals;
public:
    // The assertion to be proved
    Assertion const & assertion;
    // Problem context
    Environ * const pProbEnv;
    Environ const & probEnv() const { return *pProbEnv; }
    Enviter probEnviter() const { return probEnv().enviter; }
    // Return true if the context is a sub-context of the problem context
    bool envsubProb(Enviter iter) const
    { return iter == probEnviter() || environs.reachable(probEnviter(), iter); }
    // Is staged move generation used?
    enum { STAGED = 1 };
    bool const staged;
    template<class Env>
    Problem(Env const & env, Value const params[2]) :
        assertion(env.assertion),
        pProbEnv(assertion.expression.empty() ? NULL :
            addEnv(env, assertion.hypslabel())),
        staged(env.staged & STAGED),
        MCTS(Game(), params)
    {
        if (!pProbEnv) return;
        // Check goal.
        Goalview const goal(assertion.expRPN, assertion.exptypecode());
        Goalstatus const s = env.status(goal);
        if (s == GOALFALSE) return;
        // Add proofs of hypotheses.
        addhypproofs();
        // Root goal
        Goalptr const pGoal = addGoal(goal, pProbEnv, s);
        if (s == GOALTRUE)
            pGoal->second.pnewEnv = addsubEnv
                (pProbEnv, pProbEnv->hypstotrim(pGoal->second.goal()));
        // Root node
        *root() = Game(addsimpGoal(pGoal));
        addNodeptr(root());
    }
    // Add 1-step proof of all the hypotheses of the problem context.
    void addhypproofs()
    {
        for (Hypsize i = 0; i < assertion.hypcount(); ++i)
        {
            if (assertion.hypfloats(i)) continue;
            Goalview const goal(assertion.hypRPN(i), assertion.hyptypecode(i));
            goals[goal].proof.assign(1, assertion.hypiters[i]);
        }
    }
    // Add 1-step proof of all the hypotheses to a context.
    void addhypproofs(Environ * p)
    {
        if (!p || envsubProb(p->enviter)) return;
        for (Hypsize i = 0; i < p->assertion.hypcount(); ++i)
        {
            if (p->assertion.hypfloats(i)) continue;
            Goalview goal(p->assertion.hypRPN(i), p->assertion.hyptypecode(i));
            Goalptr const pGoal = addGoal(goal, p, GOALTRUE);
            pGoal->second.proof.assign(1, p->assertion.hypiters[i]);
        }
    }
    // UCB threshold for generating a new batch of moves
    // Change this to turn on staged move generation.
    virtual Value UCBnewstage(Nodeptr p) const;
    // Evaluate the leaf. Return {value, sure?}.
    // p should != NULL.
    virtual Eval evalleaf(Nodeptr p) const;
    Eval evaltheirleaf(Nodeptr p) const;
    // Evaluate the parent. Return {value, sure?}.
    // p should != NULL.
    virtual Eval evalparent(Nodeptr p) const
    {
        Value const v = minimax(p);
        bool const stuck = (staged & STAGED) && isourturn(p) && v == WDL::LOSS;
        return stuck ? p->eval() : v;
    }
    // Close all the nodes with p's proven goal.
    void closenodes(Nodeptr p)
    {
        if (!p->game().proven()) return;
        closenodesexcept(p->game().goaldata().nodeptrs, p);
        copyproof(p->game());
    }
    // Copy proof of the game to other contexts.
    void copyproof(Game const & game)
    {
        if (!game.proven()) return;
        Enviter const enviter = game.env().enviter;
        if (envsubProb(enviter)) return;
        // Loop through contexts with the same big goal.
        FOR (Goaldatas::reference goaldata, game.goaldatas())
            if (environs.reachable(goaldata.first->enviter, enviter))
                if (goaldata.second.setproof(game.proof()))
                    closenodesexcept(goaldata.second.nodeptrs);
    }
    // Record the proof of proven goals on back propagation.
    virtual void backpropcallback(Nodeptr p)
    {
        if (p->game().proven())
            setwin(p); // Fix seteval in backprop.
        else if (p->won() && p->game().writeproof())
            closenodes(p);
    }
    // Proof of the assertion, if not empty
    Proofsteps const & proof() const { return root()->game().proof(); }
    // # goals of a given status
    Goals::size_type countgoal(int status) const
    {
        Goals::size_type n = 0;
        FOR (Goals::const_reference goaldatas, goals)
            FOR (Goaldatas::const_reference goaldata, goaldatas.second)
                n += (goaldata.second.status == status);
        return n;
    }
    // # proven goals
    Goals::size_type countproof() const
    {
        Goals::size_type n = 0;
        FOR (Goals::const_reference goaldatas, goals)
            FOR (Goaldatas::const_reference goaldata, goaldatas.second)
                n += goaldata.second.proven();
        return n;
    }
    // # contexts
    Environs::size_type countenvs() const { return environs.size(); }
private:
    // Add and set up a context. Return its pointer.
    template<class Env>
    Environ * addEnv(Env const & env, strview name)
    {
        Environs::iterator const enviter
        = environs.insert(Environs::value_type(name, NULL)).first;
        Environ * const pEnv = new Env(env);
        (enviter->second = pEnv)->pProb = this;
        pEnv->enviter = enviter;
        return pEnv;
    }
    friend Environ;
    // Add a goal. Return its pointer.
    Goalptr addGoal(Goalview const & goal, Environ * p, Goalstatus s)
    {
        BigGoalptr const pBigGoal
        = &*goals.insert(std::make_pair(goal, Goaldatas())).first;
        Goaldata const goaldata(s, pBigGoal);
        return &*pBigGoal->second.insert(std::make_pair(p, goaldata)).first;
    }
    Goalptr addGoal
        (Proofsteps const & RPN, strview type, Environ * p, Goalstatus s)
    { return addGoal(Goalview(RPN, type), p, s); }
    // Add a sub-context with hypotheses trimmed.
    // Return pointer to the new context. Return NULL if unsuccessful.
    Environ * addsubEnv(Environ const * pEnv, Bvector const & hypstotrim);
    // close all the nodes except p
    void closenodesexcept(Nodeptrs const & nodeptrs, Nodeptr p = Nodeptr())
    {
        FOR (Nodeptr const other, nodeptrs)
            if (other != p && !other->won())
            {
                setwin(other);
                Nodeptr const parent = other.parent();
                if (parent && !parent->won())
                    backprop(parent);
            }
    }
public:
    // Printing routines. DO NOTHING if ptr is NULL.
    void printmainline(Nodeptr p, size_type detail = 0) const;
    void printmainline(size_type detail = 0) const
        { printmainline(root(), detail); }
    virtual void checkmainline(Nodeptr p) const;
    void printstats() const;
    void printenvs() const
    {
        FOR (Environs::const_reference env, environs)
            std::cout << env.first << std::endl;
    }
    void navigate(bool detailed = true) const;
    void writeproof(const char * const filename) const;
    virtual ~Problem()
    {
        FOR (Environs::const_reference subenv, environs)
            delete subenv.second;
    }
};

#endif // PROBLEM_H_INCLUDED
