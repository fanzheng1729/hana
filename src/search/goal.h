#ifndef GOAL_H_INCLUDED
#define GOAL_H_INCLUDED

#include "../proof/verify.h"

// Proof goal
typedef std::pair<Proofsteps const &, strview> Goalview;
struct Goal
{
    Proofsteps RPN;
    strview typecode;
    AST mutable tree;
    Goal(Goalview view) : RPN(view.first), typecode(view.second) {}
    Proofsteps::size_type size() const { return RPN.size(); }
    Expression expression() const
    {
        Expression result(verify(RPN));
        if (!result.empty()) result[0] = typecode;
        return result;
    }
};
inline bool operator==(Goal const & x, Goal const & y)
    { return x.RPN == y.RPN && x.typecode == y.typecode; }
inline bool operator<(Goal const & x, Goal const & y)
    { return x.RPN < y.RPN || x.RPN == y.RPN && x.typecode < y.typecode; }


// Polymorphic context
struct Environ;
// Data associated with the goal
struct Goaldata;
// Map: context -> evaluation
struct Goaldatas : std::map<Environ *, Goaldata>
{
    // Proof that holds in the problem context
    Proofsteps proof;
    Goaldatas(Proofsteps const & prf = Proofsteps()) : proof(prf) {}
};
typedef Goaldatas::pointer Goalptr;

// Map: goal -> context -> evaluation
typedef std::map<Goal, Goaldatas> Goals;
typedef Goals::pointer BigGoalptr;

#endif // GOAL_H_INCLUDED
