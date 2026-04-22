#ifndef MOVE_H_INCLUDED
#define MOVE_H_INCLUDED

#include "../ass.h"
#include "../bank.h"
#include "goal.h"
#include "../util/hex.h"

static const std::string strconj = "CONJ";
static const std::string strcomb = "COMB";

// Move in proof search tree
struct Move
{
    enum Type {NONE, THM, CONJ, DEFER};
    typedef std::vector<RPN> Substitutions;
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
    // mutable std::vector<pGoal> subgoals;
    // Workaround for some compilers
    mutable std::vector<void *> subgoals;
    // Move of specified type, defaulted to NONE
    Move(Type t = NONE) : type(t), pthm() {}
    // Move applying a theorem, on our turn
    template<class SUBST>
    Move(pAss ptr, SUBST const & subst) : type(THM), pthm(ptr)
    {substitutions.assign(subst.begin(), subst.end());}
    // Move making nconj conjectures with nabs abstractions, on our turn
    Move(Conjectures::size_type nconj, Substitutions::size_type nabs) :
        type(nconj > 0 ? CONJ : NONE), pthm(), absconjs(nconj),
        substitutions(nabs) {}
    // Move making conjectures, on our turn
    template<class ABS>
    Move(Conjectures const & conjs, ABS const & abs) :
        type(conjs.empty() ? NONE : CONJ), pthm(), absconjs(conjs)
    {substitutions.assign(abs.begin(), abs.end());}
    // Move verifying a hypothesis, on their turn
    Move(Hypsize i) : index(i), pthm() {}
    bool isthm() const  { return type == THM; }
    bool isconj() const { return type == CONJ; }
    bool isdefer() const{ return type == DEFER; }
    bool isthmorconj() const { return isthm() || isconj(); }
    strview thmlabel() const { return pthm ? pthm->first : ""; }
    strview label() const
    {
        static const char * const msg[]
        = {"NONE", "THM", "CONJ", "DEFER"};
        return pthm ? pthm->first : msg[type];
    }
    // Theorem the move uses (must be of type THM)
    Assertion const & theorem() const { return pthm->second; }
    // Type code of goal the move proves
    strview goaltypecode() const
    {
        if (pthm)
            return pthm->second.exptypecode();
        if (isconj())
            return absconjs.empty() ? "" : absconjs.back().typecode;
        return "";
    }
    // Goal the move proves (must be of type THM or CONJ)
    Goal goal() const
    {
        if (!isthmorconj())
            return Goal();
        Goal result;
        RPN const & exp = pthm ? pthm->second.expRPN : absconjs.back().rpn;
        makesubst(exp, result.rpn);
        result.typecode = goaltypecode();
        return result;
    }
    // Return true if a move satisfies disjoint variable hypotheses.
    bool checkDV(Assertion const & ass, bool verbose = false) const;
    // Find the disjoint variable hypotheses of a CONJ move.
    Disjvars findDV(Assertion const & ass) const;
    Symbol3 hypvar(Hypsize index) const
    {
        Expression const & hypexp = theorem().hypexp(index);
        return hypexp.size() == 2 ? hypexp[1] : Symbol3();
    }
    // Subgoal the move needs
    Hypsize nsubgoals() const
    {
        return pthm ? pthm->second.nhyps() :
                isconj() ? absconjs.size() : isdefer();
    }
    Hypsize nEsubgoals() const
    {
        return pthm ? pthm->second.nEhyps() :
                isconj() ? absconjs.size() : isdefer();
    }
    std::string subgoallabel(Hypsize index) const
    {
        if (index >= nsubgoals())
            return "";
        if (pthm)
            return pthm->second.hyplabel(index);
        if (isconj())
            if (index == nconjs()) return strcomb;
            else return strconj + util::hex(index);
        return isdefer() ? "DEFER" : "";
    }
    bool subgoalfloats(Hypsize index) const
    { return pthm && pthm->second.hypfloats(index); }
    strview subgoaltypecode(Hypsize index) const
    {
        return index >= nsubgoals() ? "" :
                pthm ? pthm->second.hyptypecode(index) :
                isconj() ? absconjs[index].typecode : "";
    }
    // Subgoal the move needs (must be of type THM or CONJ)
    Goal subgoal(Hypsize index) const
    {
        if (!isthmorconj() || index >= nsubgoals())
            return Goal();
        Goal result;
        RPN const & hyp = pthm ? theorem().hypRPN(index) : absconjs[index].rpn;
        makesubst(hyp, result.rpn);
        result.typecode = subgoaltypecode(index);
        return result;
    }
    // Index of subgoal (must be of type THM or CONJ)
    Hypsize matchsubgoal(Goal const & goal) const
    {
        Hypsize const n = nsubgoals();
        if (!isthmorconj())
            return n;
        Hypsize i = 0;
        for ( ; i < n; ++i)
            if (goal == subgoal(i))
                return i;
        return i;
    }
    // Return pointer to proof of subgoal. Return nullptr if out of bound.
    pProof psubgoalproof(Hypsize index) const;
    // # of conjectures made
    Hypsize nconjs() const { return isconj() * (absconjs.size() - 1); }
    // Abstract variables in use (must be of type CONJ)
    Expression absvars(Bank const & bank) const
    {
        Expression vars;
        vars.reserve(substitutions.size());
        for (Symbol2::ID id = 1; id < substitutions.size(); ++id)
            if (!substitutions[id].empty())
                vars.push_back(bank.var(id));
        return vars;
    }
    // Abstraction of a variable (must be of type CONJ)
    RPN const & abstraction(Symbol3 var) const
    {
        Symbol2::ID const id = var.id;
        if (id >= substitutions.size() || substitutions[id].empty())
            return var.iter->second.rpn;
        return substitutions[id];
    }
    // Add conjectures to a bank. Return iterators to the hypotheses.
    Hypiters addconjsto(Bank & bank) const
    {
        Hypiters result(nconjs());
        for (Hypsize i = 0; i < result.size(); ++i)
            if (Hypiter() ==
                (result[i] = bank.addhyp(absconjs[i].rpn, absconjs[i].typecode)))
                return Hypiters();
        return result;
    }
    // Find index of conjecture matching the abstract hypothesis.
    // Return nconjs() is not found.
    Hypsize findabsconj(Hypothesis const & hyp) const
    {
        for (Hypsize i = 0; i < nconjs(); ++i)
        {
            Expression const & hypexp = hyp.expression;
            if (!hypexp.empty() && absconjs[i].rpn == hyp.rpn
                && absconjs[i].typecode == hypexp[0])
                return i;
        }
        return nconjs();
    }
    // Pointers to proofs of sub-goals
    pProofs phyps() const
    {
        pProofs result(nsubgoals());
        for (Hypsize i = 0; i < result.size(); ++i)
            result[i] = psubgoalproof(i);
        return result;
    }
private:
    // Size of a substitution
    RPNsize substsize(RPN const & src) const;
    // Make a substitution.
    void makesubst(RPN const & src, RPN & dest) const;
    // Size of full proof (must be of type CONJ)
    RPNsize fullproofsize(pProofs const & phyps) const;
public:
    // Write proof.
    bool writeproof(RPN & dest, pProofs const & phyps) const;
    // Print a CONJ move.
    void printconj() const;
};

// List of moves
typedef std::vector<Move> Moves;

// Abstraction-substitution move
// = (move with substitution, skeleton with null variable)
typedef std::pair<Move, RPN> Absubstmove;
// Abstraction-substitution moves
typedef std::vector<Absubstmove> Absubstmoves;

#endif // MOVE_H_INCLUDED
