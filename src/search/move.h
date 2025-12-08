#ifndef MOVE_H_INCLUDED
#define MOVE_H_INCLUDED

#include "../ass.h"
#include "../bank.h"
#include "goal.h"
#include "../util/for.h"
#include "../util/hex.h"

static const std::string strconjecture = "CONJECTURE";
static const std::string strcombination = "COMBINATION";

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
    // Sub-goals needed, on our turn
    mutable std::vector<pGoal> subgoals;
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
        if (!isconj())
            return;
        // Max id of abstracted variable
        Symbol2::ID maxid = 0;
        FOR (Goal const & goal, absconjs)
            FOR (Proofstep const step, goal.RPN)
                if (Symbol2::ID const id = step.id())
                    if (!bank.substitution(id).empty())
                        if (id > maxid) maxid = id;
        // Preallocate for efficiency
        substitutions.resize(maxid + 1);
        // Fill in abstractions.
        FOR (Goal const & goal, absconjs)
            FOR (Proofstep const step, goal.RPN)
                if (Symbol2::ID const id = step.id())
                {
                    Proofsteps const & src = bank.substitution(id);
                    Proofsteps & dest = substitutions[id];
                    if (dest.empty())
                        dest = src;
                }
    }
    // A move verifying a hypothesis, on their turn
    Move(Hypsize i) : index(i), pthm(NULL) {}
    bool isthm() const  { return type == THM; }
    bool isconj() const { return type == CONJ; }
    bool isdefer() const{ return type == DEFER; }
    strview thmlabel() const { return pthm ? pthm->first : strview(); }
    strview label() const
    {
        static const char * const msg[]
        = {"NONE", "", "CONJ", "DEFER"};
        return isthm() ? thmlabel() : msg[type];
    }
    // Theorem the move uses (must be of type THM)
    Assertion const & theorem() const { return pthm->second; }
    // Type code of goal the move proves
    strview goaltypecode() const
    {
        if (isthm())
            return theorem().exptypecode();
        if (isconj())
            return absconjs.empty() ? strview() : absconjs.back().typecode;
        return "";
    }
    // Goal the move proves (must be of type THM or CONJ)
    Goal goal() const
    {
        if (!isthm() && !isconj())
            return Goal();
        Goal result;
        Proofsteps const & expRPN
        = pthm ? pthm->second.expRPN : absconjs.back().RPN;
        makesubstitution(expRPN, result.RPN);
        result.typecode = goaltypecode();
        return result;
    }
    // Return true if a move satisfies disjoint variable hypotheses.
    bool checkDV(Assertion const & ass, bool verbose = false) const;
    // Return the disjoint variable hypotheses of a move.
    Disjvars findDV(Assertion const & ass) const;
    // Hypothesis (must be of type THM)
    strview hyplabel(Hypsize index) const { return theorem().hyplabel(index); }
    bool hypfloats(Hypsize index) const { return theorem().hypfloats(index); }
    Symbol3 hypvar(Hypsize index) const
    {
        Expression const & hypexp = theorem().hypexp(index);
        return hypexp.size() == 2 ? hypexp[1] : Symbol3();
    }
    // Subgoal the move needs
    Hypsize subgoalcount() const
    {
        return isthm() ? theorem().hypcount() :
                isconj() ? absconjs.size() : isdefer();
    }
    Hypsize esssubgoalcount() const
    {
        return isthm() ? theorem().esshypcount() :
                isconj() ? absconjs.size() : isdefer();
    }
    std::string subgoallabel(Hypsize index) const
    {
        if (index >= subgoalcount())
            return "";
        if (isthm())
            return theorem().hyplabel(index);
        if (isconj())
            if (index == conjcount()) return strcombination;
            else return strconjecture + util::hex(index);
        return isdefer() ? "DEFER" : "";
    }
    bool subgoalfloats(Hypsize index) const
    { return isthm() ? theorem().hypfloats(index) : false; }
    strview subgoaltypecode(Hypsize index) const
    {
        return index >= subgoalcount() ? "" :
                isthm() ? theorem().hyptypecode(index) :
                isconj() ? absconjs[index].typecode : "";
    }
    // Subgoal the move needs (must be of type THM or CONJ)
    Goal subgoal(Hypsize index) const
    {
        if (!isthm() && !isconj() || index >= subgoalcount())
            return Goal();
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
        if (!isthm() && !isconj())
            return subgoalcount();
        Hypsize i = 0;
        for ( ; i < subgoalcount(); ++i)
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
    // # of conjectures made (must be of type CONJ)
    Hypsize conjcount() const { return isconj() * (absconjs.size() - 1); }
    // Abstract variables in use
    Expression absvars(Bank const & bank) const;
    // Add conjectures to a bank (must be of type CONJ).
    // Return iterators to the hypotheses.
    Hypiters addconjsto(Bank & bank) const;
private:
    // Size of a substitution
    Proofsize substitutionsize(Proofsteps const & src) const;
    // Make a substitution.
    void makesubstitution(Proofsteps const & src, Proofsteps & dest) const;
};

// List of moves
typedef std::vector<Move> Moves;

#endif // MOVE_H_INCLUDED
