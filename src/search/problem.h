#ifndef PROBLEM_H_INCLUDED
#define PROBLEM_H_INCLUDED

#include "environ.h"
#include "goaldata.h"
#include "../util/for.h"

// Add node pointer to p's goal data.
inline void addnodeptr(Nodeptr p)
{
    Goaldata & goaldata = p->game().goaldata();
    if (!goaldata.proven())
        goaldata.nodeptrs.insert(p);
}

// Problem statement + Proof search tree with loop detection
// + environment management + UI
class Problem : public MCTS<Game>
{
    // Map: name -> polymorphic sub environments
    typedef std::map<std::string, struct Environ *> Subenvs;
    // Assertions corresponding to sub environments
    Assertions subassertions;
    // Polymorphic sub environments
    Subenvs subenvs;
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
        // Add the root environment.
        Environ * const penv = new Env(env);
        (subenvs[assertion.hypslabel()] = penv)->pProb = this;
        // Add the goal.
        Goalptr const pgoal = penv
        ->addgoal(assertion.expRPN, assertion.expression[0], GOALOPEN);
        // Fix the root node.
        const_cast<Game &>(root()->game()) = Game(pgoal, penv);
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
        Value const value = minimax(p);
        bool const stuck = (staged & STAGED) && isourturn(p)
                            && value == WDL::LOSS;
        return stuck ? p->eval() : value;
    }
    // Close all the nodes with p's proven goal.
    void closenodes(Nodeptr p)
    {
        FOR (Nodeptr other, p->game().goaldata().nodeptrs)
            if (other != p && value(other) != WDL::WIN)
            {
                seteval(other, EvalWIN);
                backprop(p.parent());
            }
    }
    // Record the proof of proven goals on back propagation.
    virtual void backpropcallback(Nodeptr p)
    {
        if (value(p) == WDL::WIN && p->game().writeproof())
            closenodes(p);
}
    // Proof of the assertion, if any
    Proofsteps const & proof() const
        { return root()->game().goaldata().proofsteps; }
    // # goals of a given status
    Goals::size_type countgoal(int status) const
    {
        Goals::size_type n = 0;
        FOR (Subenvs::const_reference subenv, subenvs)
            n += subenv.second->countgoal(status);
        return n;
    }
    // # sub environments
    Subenvs::size_type countenvs() const { return subenvs.size(); }
    // Add a sub environment for the game. Return true iff it is added.
    bool addsubenv(Game const & game);
    // Printing routines. DO NOTHING if ptr is NULL.
    void printmainline(Nodeptr p, bool detailed = false) const;
    void printmainline(bool detailed = false) const
    { printmainline(root(), detailed); }
    void printstats() const;
    void printenvs() const;
    void navigate(bool detailed = true) const;
    void writeproof(const char * const filename) const;
    virtual ~Problem()
    {
        FOR (Subenvs::const_reference subenv, subenvs)
            delete subenv.second;
    }
};

#endif
