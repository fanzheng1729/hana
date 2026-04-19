#ifndef GOAL_H_INCLUDED
#define GOAL_H_INCLUDED

#include "../util/algo.h"   // for util::compare
#include "../proof/analyze.h"
#include "../proof/verify.h"

// Proof goal
typedef std::pair<RPN const &, strview> Goalview;
struct Goal
{
    RPN rpn;
    strview typecode;
    AST mutable ast;
    // Maximal abstractions
    GovernedRPNspansbystep mutable maxabs;
    bool mutable maxabsfilled;
    Goal() : rpn(), typecode(""), maxabsfilled(false) {}
    Goal(Goalview view) :
        rpn(view.first), typecode(view.second), maxabsfilled(false) {}
    void fillast() const { if (ast.empty()) ast = ::ast(rpn); }
    void fillmaxabs() const
    {
        if (maxabsfilled) return;
        fillast();
        maxabs = ::maxabs(rpn, ast);
        maxabsfilled = true;
    }
    RPNsize size() const { return rpn.size(); }
    operator RPNspanAST() const { return RPNspanAST(rpn, ast); }
    Expression expression() const
    {
        Expression result(verify(rpn));
        if (!result.empty()) result[0] = typecode;
        return result;
    }
};
inline bool operator==(Goal const & x, Goal const & y)
{
    return x.rpn == y.rpn && x.typecode == y.typecode;
}
inline bool operator<(Goal const & x, Goal const & y)
{
    int const cmp
    = util::compare(x.rpn.begin(), x.rpn.end(), y.rpn.begin(), y.rpn.end());
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
    RPN proof;
    bool proven() const { return !proof.empty(); }
};

#endif // GOAL_H_INCLUDED
