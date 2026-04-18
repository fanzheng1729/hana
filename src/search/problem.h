#ifndef PROBLEM_H_INCLUDED
#define PROBLEM_H_INCLUDED

#include <algorithm>    // for std::min
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
    Assertion const & probAss() const { return probEnv().assertion; }
    Assertions::size_type number() const
    { return pProbEnv ? probAss().number : 0; }
    // Is staged move generation used?
    enum { STAGED = true };
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
        staged(env.staged && STAGED)
    {
        if (!pProbEnv) return;
        // Check goal.
        Goalview const goal(probAss().expRPN, probAss().exptypecode());
        Goalstatus const s = env.status(goal);
        if (s == GOALFALSE) return;
        // Add proofs of hypotheses.
        addhypproofs(probEnv());
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
        Assertion const & ass = probAss();
        for (Hypsize i = 0; i < ass.nhyps(); ++i)
        {
            if (ass.hypfloats(i)) continue;
            goals[Goalview(ass.hypRPN(i), ass.hyptypecode(i))]
            .proof.assign(1, ass.hypptr(i));
        }
    }
    Environ const & addhypproofs(Environ const & env)
    {
        if (&env != &probEnv() && env.subsumedbyProb())
            return env; // Skip contexts properly subsumed by the problem context.
        // env is either problem context or not subsumed by it.
        Assertion const & ass = env.assertion;
        for (Hypsize i = 0; i < ass.nhyps(); ++i)
        {
            if (ass.hypfloats(i)) continue;
            addgoal(Goalview(ass.hypRPN(i), ass.hyptypecode(i)), env, GOALTRUE)
            ->second.proofdst().assign(1, ass.hypptr(i));
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
        Value   const v = minimax(p);
        bool    const stuck = staged && isourturn(p) && v == WDL::LOSS;
        return  stuck ? p->eval() : v;
    }
    // Return true if game's rank < Problem's rank.
    bool ranksimplerthanProb(Game const & game) const
    {
        return game.env().rankssimplerthanProb() &&
            database.syntaxDAG().simplerthan
            (database.syntaxDAG().RPNranks(game.goal().rpn), maxranks);
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
    void addranks(pNode const p)
    {
        if (value(p) < ALMOSTWIN)
            return;
        database.syntaxDAG().addranks(maxranks, p->game().env().maxranks);
        database.syntaxDAG().addexp(maxranks, p->game().goal().rpn);
    }
    // Prune the sub-tree at p and update maxranks, if almost won.
    void prune(pNode p);
    // Update implications after problem context is simplified.
    void updateimps();
    // Focus the sub-tree at p, with updated maxranks, if almost won.
    void focus(pNode p);
    // Proof of the assertion, if not empty
    RPN const & proof() const { return root()->game().proof(); }
    // Return true if a proof is legal.
    bool legal(RPN const & proof) const
    {
        Assertions::size_type const nProb = number();
        FOR (RPNstep const step, proof)
            if (step.isthm() && step.pass->second.number >= nProb)
                return false; // Assertion # >= limit
            else
            if (step.ishyp() && !step.phyp->second.floats &&
                bank.hashyp(step.phyp->first))
                return false; // Auxillary essential hypothesis
        return true;
    }
    // Return true if proof() solves the problem *iter.
    bool checkproof(Assiter iter) const
    {
        if (!legal(proof()))
            return false;
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
        Environ * const p  = new Env(env);
        environs[env.name] = p;
        return initEnv(p);
    }
    friend Environ;
    // Add a goal. Return its pointer.
    template<class GOAL>
    pGoal addgoal(GOAL const & goal, Environ const & env, Goalstatus s)
    {
        std::pair<GOAL const &, Goaldatas> const bigGoal(goal, Goaldatas());
        pBIGGOAL const pbiggoal = &*goals.insert(bigGoal).first;
        Goaldatas::value_type const envdata(&env, Goaldata(s, &env, pbiggoal));
        return &*pbiggoal->second.insert(envdata).first;
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
        if (!p) return p;
        p->pProb = this;
        p->m_subsumedbyProb = environs.size()<=1 || (probEnv().compare(*p)==1);
        p->updateimps(maxranks);
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

typedef Problem::size_type Treesize;

// Test proof search. Return tree.size if okay. Return 0 if not.
Treesize testsearch(Assiter iter, Problem & tree, Treesize maxsize);

#endif // PROBLEM_H_INCLUDED
