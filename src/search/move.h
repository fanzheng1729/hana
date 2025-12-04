#ifndef MOVE_H_INCLUDED
#define MOVE_H_INCLUDED

#include <iostream>
#include "../ass.h"
#include "../bank.h"
#include "goal.h"
#include "../util/for.h"

// Move in proof search tree
struct Move
{
    enum Type {NONE, THM, CONJ, DEFER};
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
    // # reserved variables
    Symbol2::ID nReserve;
    // Substitutions to be used, on our turn
    Substitutions substitutions;
    // Conjectures, last one = the abstracted goal
    typedef std::vector<Goal> Conjectures;
    // Abstract conjectures for conjectural moves on our turn
    Conjectures absconjs;
    // Essential hypotheses needed, on our turn
    mutable std::vector<pGoal> esshyps;
    Move(Type t = NONE) : type(t), pthm(NULL), nReserve(0) {}
    // A move applying a theorem, on our turn
    Move(pAss ptr, Substitutions const & subst) :
        type(THM), pthm(ptr), nReserve(0), substitutions(subst) {}
    Move(pAss ptr, Stepranges const & subst) :
        type(THM), pthm(ptr), nReserve(0)
    {
        substitutions.resize(subst.size());
        for (Hypsize i = 1; i < subst.size(); ++i)
            substitutions[i].assign(subst[i].first, subst[i].second);
    }
    // A move making conjectures, on our turn
    Move(Conjectures const & conjs, Bank const & bank) :
        type(conjs.empty() ? NONE : CONJ), pthm(NULL),
        nReserve(type == NONE ? 0 : bank.nReserve),
        absconjs(conjs)
    {
        if (type == NONE)
            return;
        // Max id of abstracted variable
        Symbol2::ID maxid = 0;
        FOR (Goal const & goal, absconjs)
            FOR (Proofstep step, goal.RPN)
                if (Symbol2::ID id = step.id())
                    if (!bank.substitution(id).empty())
                        if (id > maxid) maxid = id;
        // Preallocate for efficiency
        substitutions.resize(maxid + 1);
        // Fill in abstractions.
        FOR (Goal const & goal, absconjs)
            FOR (Proofstep step, goal.RPN)
                if (Symbol2::ID id = step.id())
                {
                    Proofsteps const & RPN = bank.substitution(id);
                    if (!RPN.empty())
                        substitutions[id] = RPN;
                }
    }
    // A move verifying a hypothesis, on their turn
    Move(Hypsize i) : index(i), pthm(NULL), nReserve(0) {}
    // Theorem the move uses
    strview label() const { return pthm ? pthm->first : ""; }
    Assertion const & theorem() const { return pthm->second; }
    // Type code of goal the move proves (must be of type THM or CONJ)
    strview goaltypecode() const
    {
        if (type == THM)
            return theorem().exptypecode();
        if (type == CONJ)
            return absconjs.empty() ? strview() : absconjs.back().typecode;
        return "";
    }
    // Goal the move proves (must be of type THM or CONJ)
    Goal goal() const
    {
        Goal result;
        Proofsteps const & expRPN = pthm ? pthm->second.expRPN : absconjs.back().RPN;
        makesubstitution
        (expRPN, result.RPN, substitutions, util::mem_fn(&Proofstep::id));
        result.typecode = goaltypecode();
        return result;
    }
    // Hypothesis (must be of type THM)
    strview hyplabel(Hypsize index) const { return theorem().hyplabel(index); }
    bool hypfloats(Hypsize index) const { return theorem().hypfloats(index); }
    Expression const & hypexp(Hypsize index) const
        { return theorem().hypexp(index); }
    strview hyptypecode(Hypsize index) const
        { return theorem().hyptypecode(index); }
    // Subgoal the attempt (must be of type THM) needs
    Goal subgoal(Hypsize index) const
    {
        if (!pthm) return Goal();
        Goal result;
        makesubstitution
        (theorem().hypRPN(index), result.RPN, substitutions,
            util::mem_fn(&Proofstep::id));
        result.typecode = theorem().hyptypecode(index);
        return result;
    }
    // Find the index of hypothesis by goal.
    Hypsize matchhyp(Goal const & goal) const
    {
        Hypsize i = 0;
        for ( ; i < hypcount(); ++i)
            if (goal == subgoal(i))
                return i;
        return i;
    }
    // # of hypotheses the attempt (must be of type THM) needs
    Hypsize hypcount() const { return theorem().hypcount(); }
    // # of variables the attempt (must be of type THM) needs
    Symbol3s::size_type varcount() const { return theorem().varcount(); }
    // # of essential hypotheses the attempt (must be of type THM) needs
    Hypsize esshypcount() const { return hypcount() - varcount(); }
    // Concrete conjecture
    Proofstep fullconj(Hypsize index) const;
    // Output the move (must be our move).
    friend std::ostream & operator<<(std::ostream & out, Move const & move)
    {
        static const char * const msg[]
        = {"NONE", "", "CONJ", "DEFER"};
        out << msg[move.type];
        if (move.type == THM)
            out << move.label().c_str;
        return out;
    }
};

// List of moves
typedef std::vector<Move> Moves;

#endif // MOVE_H_INCLUDED
