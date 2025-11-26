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
    enum { STAGED = 1 };
    // Is staged move generation turned on?
    bool const staged;
    template<class Env>
    Problem(Env const & env, Value const params[2]) :
        assertion(env.assertion), staged(env.staged & STAGED),
        MCTS(Game(), params)
    {
        if (assertion.expression.empty()) return;
        Goalview const goal(assertion.expRPN, assertion.expression[0]);
        Goalstatus const s = env.status(goal);
        if (s == GOALFALSE) return;
        // Root context
        Environ * const pEnv = new Env(env);
        pEnv->pProb = this;
        pEnv->enviter = environs.insert
        (std::make_pair(assertion.hypslabel().c_str(), pEnv)).first;
        // Root goal
        Goalptr pGoal = addGoal(goal, pEnv, s);
        Environ * & pnewEnv = pGoal->second.pnewEnv = NULL;
        if (s == GOALTRUE)
            pnewEnv = addsubEnv(pEnv, pEnv->hypstotrim(pGoal->second.goal()));
        // Root node
        *root() = Game(addGoaldata(pGoal, pnewEnv));
        addNodeptr(root());
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
        if (!p->game().proven())
            return;
        FOR (Nodeptr const other, p->game().goaldata().nodeptrs)
            if (other != p && !other->won())
            {
                setwin(other);
                Nodeptr const parent = other.parent();
                if (parent && !parent->won())
                    backprop(parent);
            }
        copyPrffromsubEnv(p->game());
    }
    // Copy proofs from sub contexts.
    void copyPrffromsubEnv(Game const & game);
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
    // Add a sub-context for the game. Return pointer to the new context.
    // Return NULL if not okay.
    Environ * addsubEnv(Environ const * pEnv, Bvector const & hypstotrim);
    // Add a goal. Return its pointer.
    Goalptr addGoal(Goalview const & goal, Environ * pEnv, Goalstatus s);
    Goalptr addGoal
        (Proofsteps const & RPN, strview type, Environ * pEnv, Goalstatus s)
    { return addGoal(Goalview(RPN, type), pEnv, s); }
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
