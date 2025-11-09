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

// Data associated with the goal
struct Goaldata;
// Map: goal -> Evaluation
typedef std::map<Goal, Goaldata> Goals;
// Pointer to a goal
typedef Goals::pointer Goalptr;

// Map: context -> evaluation
typedef std::map<struct Environ const *, Goaldata> Goaldatas;
typedef Goaldatas::pointer Goaldataptr;
// Map: goal -> context -> evaluation
typedef std::map<Goal, Goaldatas> Goals2;
typedef Goals2::pointer Goal2ptr;

#endif // GOAL_H_INCLUDED
