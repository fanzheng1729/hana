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
    // Substitutions to be used, on our turn
    Substitutions substitutions;
    // Conjectures, last one = the abstracted goal
    typedef std::vector<Goal> Conjectures;
    // Abstract conjectures for conjectural moves on our turn
    Conjectures absconjs;
    // Essential hypotheses needed, on our turn
    mutable std::vector<pGoal> esshyps;
    Move(Type t = NONE) : type(t), pthm(NULL) {}
    // A move applying a theorem, on our turn
    Move(pAss ptr, Substitutions const & subst) :
        type(THM), pthm(ptr), substitutions(subst) {}
    Move(pAss ptr, Stepranges const & subst) : type(THM), pthm(ptr)
    {
        substitutions.resize(subst.size());
        for (Hypsize i = 1; i < subst.size(); ++i)
            substitutions[i].assign(subst[i].first, subst[i].second);
    }
    // A move making conjectures, on our turn
    Move(Conjectures const & conjs, Bank const & bank) :
        type(conjs.empty() ? NONE : CONJ), pthm(NULL), absconjs(conjs)
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
                    if (!RPN.empty() && substitutions[id].empty())
                        substitutions[id] = RPN;
                }
    }
    // A move verifying a hypothesis, on their turn
    Move(Hypsize i) : index(i), pthm(NULL) {}
    strview thmlabel() const { return pthm ? pthm->first : strview(); }
    strview label() const
    {
        static const char * const msg[]
        = {"NONE", "", "CONJ", "DEFER"};
        return type == THM ? thmlabel() : msg[type];
    }
    Hypsize childcount() const
    {
        switch (type)
        {
        case THM:
            return hypcount();
        case CONJ:
            return absconjs.size();
        case DEFER:
            return 1;
        default:
            return 0;
        }
        return 0;
    }
    // Theorem the move uses (must be of type THM)
    Assertion const & theorem() const { return pthm->second; }
    // Type code of goal the move proves
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
        Proofsteps const & expRPN
        = pthm ? pthm->second.expRPN : absconjs.back().RPN;
        makesubstitution(expRPN, result.RPN);
        result.typecode = goaltypecode();
        return result;
    }
    // Hypothesis (must be of type THM)
    strview hyplabel(Hypsize index) const { return theorem().hyplabel(index); }
    bool hypfloats(Hypsize index) const { return theorem().hypfloats(index); }
    Symbol3 hypvar(Hypsize index) const
    {
        Expression const & hypexp = theorem().hypexp(index);
        return hypexp.size() == 2 ? hypexp[1] : Symbol3();
    }
    // Type code of subgoal the move needs (must be of type THM or CONJ)
    strview subgoaltypecode(Hypsize index) const
    {
        if (type == THM)
            return theorem().hyptypecode(index);
        if (type == CONJ)
            if (index >= absconjs.size()) return "";
            else return absconjs[index].typecode;
        return "";
    }
    // Subgoal the move needs (must be of type THM or CONJ)
    Goal subgoal(Hypsize index) const
    {
        Goal result;
        Proofsteps const & hypRPN
        = pthm ? theorem().hypRPN(index) : absconjs[index].RPN;
        makesubstitution(hypRPN, result.RPN);
        result.typecode = subgoaltypecode(index);
        return result;
    }
    // Index of subgoal (must be of type THM or CONJ)
    Hypsize matchsubgoal(Goal const & goal) const
    {
        Hypsize i = 0;
        for ( ; i < childcount(); ++i)
            if (goal == subgoal(i))
                return i;
        return i;
    }
    // # of hypotheses the move needs (must be of type THM)
    Hypsize hypcount() const { return theorem().hypcount(); }
    // # of variables the move needs (must be of type THM)
    Symbol3s::size_type varcount() const { return theorem().varcount(); }
    // # of essential hypotheses the move needs (must be of type THM)
    Hypsize esshypcount() const { return hypcount() - varcount(); }
    // Output the move (must be our move).
    friend std::ostream & operator<<(std::ostream & out, Move const & move)
    { return out << move.label().c_str; }
private:
    Proofsize substitutionsize(Proofsteps const & src) const
    {
        Proofsize size = 0;
        // Make the substitution
        FOR (Proofstep step, src)
        {
            Symbol2::ID id = step.id();
            if (id > 0 && !substitutions[id].empty())
                size += substitutions[id].size();
            else
                ++size;
        }
        return size;
    }
    void makesubstitution(Proofsteps const & src, Proofsteps & dest) const
    {
        if (substitutions.empty())
            return dest.assign(src.begin(), src.end());
        // Preallocate for efficiency
        dest.reserve(substitutionsize(src));
        // Make the substitution
        FOR (Proofstep step, src)
        {
            Symbol2::ID const id = step.id();
            if (id > 0 && !substitutions[id].empty())
                dest += substitutions[id];  // variable with an id
            else
                dest.push_back(step);     // constant with no id
        }
    }
};

// List of moves
typedef std::vector<Move> Moves;

#endif // MOVE_H_INCLUDED
