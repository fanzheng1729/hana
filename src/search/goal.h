#ifndef GOAL_H_INCLUDED
#define GOAL_H_INCLUDED

#include "../util/algo.h"   // for util::compare
#include "../proof/verify.h"

// Proof goal
typedef std::pair<Proofsteps const &, strview> Goalview;
struct Goal
{
    Proofsteps RPN;
    strview typecode;
    AST mutable tree;
    Goal(Goalview view) : RPN(view.first), typecode(view.second) {}
    Proofsize size() const { return RPN.size(); }
    Expression expression() const
    {
        Expression result(verify(RPN));
        if (!result.empty()) result[0] = typecode;
        return result;
    }
};
inline bool operator==(Goal const & x, Goal const & y)
{
    return x.RPN == y.RPN && x.typecode == y.typecode;
}
inline bool operator<(Goal const & x, Goal const & y)
{
    int const cmp
    = util::compare(x.RPN.begin(), x.RPN.end(), y.RPN.begin(), y.RPN.end());
    return cmp < 0 || cmp == 0 && x.typecode < y.typecode;
}


// Polymorphic context, with move generation and goal evaluation
struct Environ;
// Data associated with the goal
class Goaldata;
// Map: context -> evaluation
struct Goaldatas : std::map<Environ const *, Goaldata>
{
    // Proof that holds in the problem context
    Proofsteps proof;
    bool proven() const { return !proof.empty(); }
};
typedef Goaldatas::pointer pGoal;

// Map: goal -> context -> evaluation
typedef std::map<Goal, Goaldatas> Goals;
typedef Goals::pointer pBIGGOAL;

#endif // GOAL_H_INCLUDED
