#ifndef MOVE_H_INCLUDED
#define MOVE_H_INCLUDED

#include <iostream>
#include "../ass.h"
#include "goal.h"
#include "../proof/verify.h"

// Move in proof search tree
struct Move
{
    enum Type { NONE, THM, CONJ, DEFER };
    typedef std::vector<Proofsteps> Substitutions;
    union
    {
        // Type of the attempt, on our turn
        Type type;
        // Index of the hypothesis, on their turn
        Hypsize index;
    };
    // Pointer to the theorem to be used, on our turn
    pAss pthm;
    // Substitutions to be used, on our turn
    Substitutions substitutions;
    // Essential hypotheses needed, on our turn
    mutable std::vector<pGoal> hypvec;
    Move(Type t = NONE) : type(t), pthm(NULL) {}
    // A move applying a theorem, on our turn
    Move(pAss ptr, Substitutions const & subst) :
        type(THM), pthm(ptr), substitutions(subst) {}
    Move(pAss ptr, Stepranges const & subst) :
        type(THM), pthm(ptr)
    {
        substitutions.resize(subst.size());
        for (Hypsize i = 1; i < subst.size(); ++i)
            substitutions[i].assign(subst[i].first, subst[i].second);
    }
    // A move verifying a hypothesis, on their turn
    Move(Hypsize i) : index(i), pthm(NULL) {}
    // Expression the attempt of using an assertion proves (must be of type THM)
    Proofsteps expRPN() const
    {
        Proofsteps result;
        makesubstitution
        (pthm->second.expRPN, result, substitutions,
            util::mem_fn(&Proofstep::id));
        return result;
    }
    strview label() const { return pthm->first; }
    Assertion const & theorem() const { return pthm->second; }
    strview exptypecode() const { return theorem().exptypecode(); }
    // Hypothesis (must be of type THM)
    strview hyplabel(Hypsize index) const { return theorem().hyplabel(index); }
    bool hypfloats(Hypsize index) const { return theorem().hypfloats(index); }
    Expression const & hypexp(Hypsize index) const
        { return theorem().hypexp(index); }
    strview hyptypecode(Hypsize index) const
        { return theorem().hyptypecode(index); }
    // Hypothesis the attempt (must be of type THM) needs
    Proofsteps hypRPN(Hypsize index) const
    {
        Proofsteps result;
        makesubstitution
        (theorem().hypRPN(index), result, substitutions,
            util::mem_fn(&Proofstep::id));
        return result;
    }
    // Find the index of hypothesis by goal.
    Hypsize matchhyp(Goal const & goal) const
    {
        Hypsize i = 0;
        for ( ; i < hypcount(); ++i)
            if (hypRPN(i) == goal.RPN && hyptypecode(i) == goal.typecode)
                return i;
        return i;
    }
    // # of hypotheses the attempt (must be of type THM) needs
    Hypsize hypcount() const { return theorem().hypcount(); }
    // # of variables the attempt (must be of type THM) needs
    Symbol3s::size_type varcount() const { return theorem().varcount(); }
    // # of essential hypotheses the attempt (must be of type THM) needs
    Hypsize esshypcount() const { return hypcount() - varcount(); }
    // Output the move (must be our move).
    friend std::ostream & operator<<(std::ostream & out, Move const & move)
    {
        static const char * const msg[] = {"NONE", "", "DEFER"};
        out << msg[move.type];
        if (move.type == THM)
            out << move.label().c_str;
        return out;
    }
};

// List of moves
typedef std::vector<Move> Moves;

#endif // MOVE_H_INCLUDED
