#ifndef PROBLEM_H_INCLUDED
#define PROBLEM_H_INCLUDED

#include <algorithm>    // for std::min
#include "environ.h"
#include "goaldata.h"
#include "../proof/compspans.h"
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
private:
    // Bank of variables and hypotheses
    Bank bank;
    // Abstractions made
    typedef std::map<RPNspan, Absubstmoves, Compspans> Abstractions;
    Abstractions abstractions;
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
        abstractions(compspans),
        numberlimit(std::min(env.assnum(), database.assiters().size())),
        maxranks(database.assmaxranks(env.assertion)),
        maxranknumber(database.syntaxDAG().maxranknumber(maxranks)),
        pProbEnv(env.assertion.expression.empty() ? Environs::mapped_type() :
                 addProbEnv(env)),
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
            int const cmp = oldEnv.compEnv(env);
            if (cmp == 0) continue;
            oldEnv.addEnv(env, cmp);
            env.addEnv(oldEnv, -cmp);
        }
        return env;
    }
// Eval
    // UCB threshold for generating a new batch of moves
    // Change this to turn on staged move generation.
    virtual Value UCBnewstage(pNode p) const;
    // Do singular extension. Return the value.
    // p should != nullptr.
    Value singularext(pNode p);
    // Return true if game's rank < Problem's rank.
    bool ranksimplerthanProb(Game const & game) const;
    // Evaluate the leaf. Return {value, sure?}.
    // p should != nullptr.
    virtual Eval evalleaf(pNode p) const;
    Eval evaltheirleaf(pNode p) const;
    // Evaluate the parent. Return {value, sure?}.
    // p should != nullptr.
    virtual Eval evalparent(pNode p) const;
    // Copy proof of the game to other contexts.
// Node operations
    void copyproof(Game const & game);
    // Close all the nodes with p's proven goal.
    void closenodes(pNode p);
    // Record the proof of proven goals on back propagation.
    virtual void backpropcallback(pNode p);
    // Called after each playonce()
    virtual void playoncecallback();
// Reval
    // Add the ranks of a node to maxranks, if almost won.
    void addranks(pNode p);
    // Prune the sub-tree at p and update maxranks, if almost won.
    void prune(pNode p);
    // Update implications after problem context is simplified.
    void updateimps();
    // Focus the sub-tree at p, with updated maxranks, if almost won.
    void focus(pNode p);
    // Refocus the tree on simpler sub-tree, if almost won.
    void reval();
    // Proof of the assertion, if not empty
    RPN const & proof() const { return root()->game().proof(); }
// Stats
    // Return true if proof() solves the problem *iter.
    bool checkproof(Assiter iter) const;
    // # goals of a given status
    Goals::size_type countgoal(int status) const;
    // # proven goals
    Goals::size_type countproof() const;
    // # contexts
    Environs::size_type countenvs() const { return environs.size(); }
    // # sub-contexts
    Environs::size_type countsubenvs() const { return probEnv().nsubEnvs(); }
    // # sup-contexts
    Environs::size_type countsupenvs() const { return probEnv().nsupEnvs(); }
    // # abstractions
    Abstractions::size_type countabs() const { return abstractions.size(); }
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
    // Initialize a context if existent. Return its pointer.
    Environ const * initEnv(Environ * p);
    // Add a sub-context with hypotheses trimmed.
    // Return pointer to the new context. Return nullptr if unsuccessful.
    Environ const * addsubEnv(Environ const & env, Bvector const & hypstotrim);
    // Add a super-context with hypotheses trimmed.
    // Return pointer to the new context. Return nullptr if unsuccessful.
    Environ const * addsupEnv(Environ const & env, Move const & move);
    // Create an abstraction move.
    Move absmove
        (Goal const & goal, Absubstmove const & absubstmove,
         RPNspanAST goalsubexp);
    // Close all the nodes except p.
    void closenodesexcept(pNodes const & pnodes, pNode p = pNode());
public:
    // Printing routines. DO NOTHING if p is nullptr.
    void printmainline(pNode p, size_type detail = 0) const;
    void printmainline(size_type detail = 0) const
        { printmainline(root(), detail); }
    void checkmainline(pNode p) const;
    void printstats() const;
    void printenvs() const;
    void printranksinfo() const;
    void navigate(pNode p, bool detailed = true) const;
    void navigate(bool detailed = true) const { navigate(root(), detailed); }
    void printabs() const;
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
