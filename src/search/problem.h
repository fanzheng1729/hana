#ifndef PROBLEM_H_INCLUDED
#define PROBLEM_H_INCLUDED

#include "../DAG.h"
#include "environ.h"
#include "goaldata.h"
#include "../util/for.h"

// Add node pointer to p's goal data.
inline void addnodeptr(Nodeptr p)
{
    if (!p->game().proven())
        p->game().goaldata().nodeptrs.insert(p);
}

// Problem statement + Proof search tree with loop detection
// + context management + UI
class Problem : public MCTS<Game>
{
    // Assertions corresponding to contexts
    Assertions assertions;
    // DAG of polymorphic contexts
    DAG<Environs> environs;
    // Map: goal -> context -> evaluation
    Goalenvs goals;
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
        Goalstatus const status = env.valid(assertion.expRPN);
        if (status == GOALFALSE) return;
        // Root context
        Environ * const penv = new Env(env);
        penv->pProb = this;
        penv->enviter = environs.insert
        (std::make_pair(assertion.hypslabel().c_str(), penv)).first;
        // Root goal
        strview type = assertion.expression[0];
        Goalptr pgoal = penv->addgoal(assertion.expRPN, type, status);
        Environ * const pnewenv = pgoal->second.pnewenv
                                = addenv(penv, penv->hypstotrim(pgoal));
        // Root node
        *root() = Game(pgoal, pnewenv ? pnewenv : penv);
        addnodeptr(root());
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
        FOR (Nodeptr const other, p->game().goaldata().nodeptrs)
            if (other != p && !other->won())
            {
                setwin(other);
                Nodeptr const parent = other.parent();
                if (parent && !parent->won())
                    backprop(parent);
            }
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
        FOR (Environs::const_reference subenv, environs)
            n += subenv.second->countgoal(status);
        return n;
    }
    // # proven goals
    Goals::size_type countproof() const
    {
        Goals::size_type n = 0;
        FOR (Environs::const_reference subenv, environs)
            n += subenv.second->countproof();
        return n;
    }
    // # contexts
    Environs::size_type countenvs() const { return environs.size(); }
    // Add a context for the game. Return pointer to the new context.
    // Return NULL if not okay.
    Environ * addenv(Environ const * penv, Bvector const & hypstotrim);
    // Add a goal. Return its pointer.
    Goalenvptr addgoal(Proofsteps const & RPN, strview typecode, Goalstatus s);
    // Printing routines. DO NOTHING if ptr is NULL.
    void printmainline(Nodeptr p, size_type detail = 0) const;
    void printmainline(size_type detail = 0) const
        { printmainline(root(), detail); }
    virtual void checkmainline(Nodeptr p) const;
    void printstats() const;
    void printenvs() const
    {
        FOR (Environs::const_reference env, environs)
            env.second->printstats();
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
