#ifndef PROBLEM_H_INCLUDED
#define PROBLEM_H_INCLUDED

#include <algorithm>    // for std::min
#include "environ.h"
#include "goaldata.h"
#include "../util/for.h"
#include "../io.h"

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
    // Bank of variables and hypotheses
    Bank bank;
private:
// Updated when problem is simplified
    // Must use assertion whose number is smaller than this.
    Assertions::size_type numberlimit;
    // Maximal ranks of the assertion
    SyntaxDAG::Ranks maxranks;
    // Max # of rank in maxranks
    Assertions::size_type maxranknumber;
public:
    // Problem context
    Environ const * const pProbEnv;
    Environ const & probEnv() const { return *pProbEnv; }
    // Assertion to be proven
    Assertion const & probAss() const {return probEnv().assertion; }
    // Is staged move generation used?
    enum { STAGED = 1 };
    bool const staged;
    template<class Env>
    Problem(Env const & env, MCTSParams const params) :
        MCTS(Game(), params),
        database(env.database),
        bank(database.nvar()),
        numberlimit(std::min(env.assertion.number,database.assiters().size())),
        maxranks(database.assmaxranks(env.assertion)),
        maxranknumber(database.syntaxDAG().maxranknumber(maxranks)),
        pProbEnv(env.assertion.expression.empty() ? NULL : addProbEnv(env)),
        staged(env.staged & STAGED)
    {
        if (!pProbEnv) return;
        // Check goal.
        Goalview const goal(probAss().expRPN, probAss().exptypecode());
        Goalstatus const s = env.status(goal);
        if (s == GOALFALSE) return;
        // Add proofs of hypotheses.
        addhypproofs();
        // Root goal
        pGoal const pgoal = addgoal(goal, probEnv(), s);
        if (s == GOALTRUE)
        {
            Goal const & rootgoal = pgoal->second.goal();
            rootgoal.fillast();
            pgoal->second.psimpEnv = addsubEnv
                (probEnv(), probEnv().hypstotrim(rootgoal));
        }
        // Root node
        *root() = Game(addsimpgoal(pgoal));
        addpNode(root());
    }
    // Add 1-step proof of all the hypotheses of the problem context.
    void addhypproofs()
    {
        for (Hypsize i = 0; i < probAss().nhyps(); ++i)
        {
            if (probAss().hypfloats(i)) continue;
            Goalview const goal(probAss().hypRPN(i), probAss().hyptypecode(i));
            goals[goal].proof.assign(1, probAss().hypptr(i));
        }
    }
    // Add 1-step proof of all the hypotheses to a context. Return env.
    Environ const & addhypproofs(Environ const & env)
    {
        if (env.subsumedbyProb()) return env;
        for (Hypsize i = 0; i < env.assertion.nhyps(); ++i)
        {
            if (env.assertion.hypfloats(i)) continue;
            Goalview goal(env.assertion.hypRPN(i), env.assertion.hyptypecode(i));
            addgoal(goal, env, GOALTRUE)
            ->second.proofdst().assign(1, env.assertion.hypptr(i));
        }
        return env;
    }
    // Add implication relation for newly added context. Return env.
    Environ const & addimps(Environ const & env)
    {
        FOR (Environs::const_reference renv, environs)
        {
            Environ const * poldEnv = renv.second;
            if (!poldEnv || poldEnv == &env) continue;
            Environ const & oldEnv = *renv.second;
            int const cmp = oldEnv.compare(env);
            if (cmp == 0) continue;
            oldEnv.addEnv(env, cmp);
            env.addEnv(oldEnv, -cmp);
        }
        return env;
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
    // Return true if game's rank < Problem's rank.
    bool ranksimplerthanProb(Game const & game) const
    {
        return game.env().rankssimplerthanProb() &&
            database.syntaxDAG().simplerthan
            (database.syntaxDAG().RPNranks(game.goal().RPN), maxranks);
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
    // Called after each playonce()
    virtual void playoncecallback()
    {
        re_eval();
        // checkmainline(root());
    }
    // Refocus the tree on simpler sub-tree, if almost won.
    void re_eval();
    // Add the ranks of a node to maxranks, if almost won.
    void addranks(pNode p);
    // Prune the sub-tree at p and update maxranks, if almost won.
    void prune(pNode p);
    // Update implications after problem context is simplified.
    void updateimps();
    // Focus the sub-tree at p, with updated maxranks, if almost won.
    void focus(pNode p);
    // Proof of the assertion, if not empty
    Proofsteps const & proof() const { return root()->game().proof(); }
    // Return true if proof() is okay.
    bool checkproof(Assiter iter) const
    {
        FOR (Proofstep const step, proof())
            if (step.isthm() && step.pass->second.number >= numberlimit)
                return false; // Assertion # >= limit
            else if (step.ishyp() && !step.phyp->second.floats &&
                     bank.hashyp(step.phyp->first))
                return false; // Essential Hypothesis not allowed
        Expression const & conclusion(verify(proof(), &*iter));
        return checkconclusion(iter->first,conclusion,iter->second.expression);
    }
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
        Environ * const p = new Env(env);
        environs[env.name] = p;
        p->pProb = this;
        p->m_subsumedbyProb = true;
        p->m_rankssimplerthanProb
        = database.syntaxDAG().simplerthan(p->maxranks, maxranks);
        return p;
    }
    friend Environ;
    // Add a goal. Return its pointer.
    template<class GOAL>
    pGoal addgoal(GOAL const & goal, Environ const & env, Goalstatus s)
    {
        std::pair<GOAL const &, Goaldatas> bigGoal(goal, Goaldatas());
        pBIGGOAL pbig=&*goals.insert(bigGoal).first;
        Goaldatas::value_type envdata(&env, Goaldata(s, &env, pbig));
        return &*pbig->second.insert(envdata).first;
    }
    // Add a sub-context with hypotheses trimmed.
    // Return pointer to the new context. Return NULL if unsuccessful.
    Environ const * addsubEnv(Environ const & env, Bvector const & hypstotrim);
    // Add a super-context with hypotheses trimmed.
    // Return pointer to the new context. Return NULL if unsuccessful.
    Environ const * addsupEnv(Environ const & env, Move const & move);
    // Initialize a context if existent. Return its pointer.
    Environ const * initEnv(Environ * p)
    {
        if (!p) return NULL;
        p->pProb = this;
        p->m_subsumedbyProb = (probEnv().compare(*p) == 1);
        p->m_rankssimplerthanProb
        = database.syntaxDAG().simplerthan(p->maxranks, maxranks);
        return &addimps(addhypproofs(*p));
    }
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
    void checkmainline(pNode p) const;
    void printstats() const;
    void printenvs() const;
    void printranksinfo() const;
    void navigate(pNode p, bool detailed = true) const;
    void navigate(bool detailed = true) const { navigate(root(), detailed); }
    void writeproof(const char * const filename) const;
    virtual ~Problem()
    {
        FOR (Environs::const_reference subenv, environs)
            delete subenv.second;
    }
};

// Test proof search. Return tree.size if okay. Return 0 if not.
Problem::size_type testsearch
    (Assiter iter, Problem & tree, Problem::size_type maxsize);

#endif // PROBLEM_H_INCLUDED
