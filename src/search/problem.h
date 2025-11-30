#ifndef PROBLEM_H_INCLUDED
#define PROBLEM_H_INCLUDED

#include "environ.h"
#include "goaldata.h"
#include "../util/for.h"

// Problem statement + Proof search tree with loop detection
// + context management + goal management + UI
class Problem : public MCTS<Game>
{
    // Assertions corresponding to contexts
    Assertions assertions;
    // Polymorphic contexts
    Environs environs;
    // Map: goal -> context -> evaluation
    Goals goals;
public:
    // Database used
    Database const & database;
    // The assertion to be proved
    Assertion const & assertion;
private:
    // Maximal ranks of the assertion
    // Updated when problem is simplified
    SyntaxDAG::Ranks maxranks;
public:
    // Problem context
    Environ const * const pProbEnv;
    Environ const & probEnv() const { return *pProbEnv; }
    // Is staged move generation used?
    enum { STAGED = 1 };
    bool const staged;
    template<class Env>
    Problem(Env const & env, Value const params[2]) :
        MCTS(Game(), params), database(env.database),
        assertion(env.assertion),
        maxranks(database.assmaxranks(assertion)),
        pProbEnv(assertion.expression.empty() ? NULL : addProbEnv(env)),
        staged(env.staged & STAGED)
    {
        if (!pProbEnv) return;
        // Check goal.
        Goalview const goal(assertion.expRPN, assertion.exptypecode());
        Goalstatus const s = env.status(goal);
        if (s == GOALFALSE) return;
        // Add proofs of hypotheses.
        addhypproofs();
        // Root goal
        pGoal const pgoal = addgoal(goal, *pProbEnv, s);
        if (s == GOALTRUE)
            pgoal->second.psimpEnv = addsubEnv
                (*pProbEnv, pProbEnv->hypstotrim(pgoal->second.goal()));
        // Root node
        *root() = Game(addsimpgoal(pgoal));
        addpNode(root());
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
    void addhypproofs(Environ const & env)
    {
        if (env.subsumedbyProb()) return;
        for (Hypsize i = 0; i < env.assertion.hypcount(); ++i)
        {
            if (env.assertion.hypfloats(i)) continue;
            Goalview goal(env.assertion.hypRPN(i), env.assertion.hyptypecode(i));
            addgoal(goal, env, GOALTRUE)
            ->second.proofdst().assign(1, env.assertion.hypiters[i]);
        }
    }
    void updateimplication(Environ const & env)
    {
        FOR (Environs::const_reference renv, environs)
        {
            Environ const & oldEnv = *renv.second;
            if (&oldEnv == &env) continue;
            int const cmp = oldEnv.compare(env);
            oldEnv.addEnv(env, cmp);
            env.addEnv(oldEnv, -cmp);
        }
    }
    // UCB threshold for generating a new batch of moves
    // Change this to turn on staged move generation.
    virtual Value UCBnewstage(pNode p) const;
    // Do singular extension. Return the value.
    Value singularext(pNode p)
    {
        if (isourturn(p))
            return value(p);
        // Their turn
        Value value = WDL::WIN;
        if (expand<&Game::moves>(p))
        {
            evalnewleaves(p);
            value = minimax(p);
        }
        return value;
    }
    // Evaluate the leaf. Return {value, sure?}.
    // p should != NULL.
    virtual Eval evalleaf(pNode p) const;
    Eval evaltheirleaf(pNode p) const;
    // Evaluate the parent. Return {value, sure?}.
    // p should != NULL.
    virtual Eval evalparent(pNode p) const
    {
        Value const v = minimax(p);
        bool const stuck = (staged & STAGED) && isourturn(p) && v == WDL::LOSS;
        return stuck ? p->eval() : v;
    }
    // Close all the nodes with p's proven goal.
    void closenodes(pNode p)
    {
        if (!p->game().proven()) return;
        closenodesexcept(p->game().goaldata().pnodes(), p);
        copyproof(p->game());
    }
    // Copy proof of the game to other contexts.
    void copyproof(Game const & game);
    // Record the proof of proven goals on back propagation.
    virtual void backpropcallback(pNode p)
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
                n += (goaldata.second.getstatus() == status);
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
    // Add the problem context. Return its pointer.
    template<class Env>
    Environ const * addProbEnv(Env const & env)
    {
        Environ * const pEnv = new Env(env);
        environs[env.name] = pEnv;
        pEnv->pProb = this;
        pEnv->m_subsumedbyProb = true;
        pEnv->m_rankssimplerthanProb
        = database.syntaxDAG().simplerthan(pEnv->maxranks, maxranks);
        return pEnv;
    }
    friend Environ;
    // Add a goal. Return its pointer.
    pGoal addgoal(Goalview const & goal, Environ const & env, Goalstatus s)
    {
        pBIGGOAL const pbigGoal
        = &*goals.insert(std::make_pair(goal, Goaldatas())).first;
        Goaldatas::value_type const value(&env, Goaldata(s, &env, pbigGoal));
        return &*pbigGoal->second.insert(value).first;
    }
    pGoal addgoal
        (Proofsteps const & RPN, strview type, Environ const & env, Goalstatus s)
    { return addgoal(Goalview(RPN, type), env, s); }
    // Add a sub-context with hypotheses trimmed.
    // Return pointer to the new context. Return NULL if unsuccessful.
    Environ const * addsubEnv(Environ const & env, Bvector const & hypstotrim);
    // close all the nodes except p
    void closenodesexcept(pNodes const & pnodes, pNode p = pNode())
    {
        FOR (pNode const other, pnodes)
            if (other != p && !other->won())
            {
                setwin(other);
                pNode const parent = other.parent();
                if (parent && !parent->won())
                    backprop(parent);
            }
    }
public:
    // Printing routines. DO NOTHING if ptr is NULL.
    void printmainline(pNode p, size_type detail = 0) const;
    void printmainline(size_type detail = 0) const
        { printmainline(root(), detail); }
    virtual void checkmainline(pNode p) const;
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
