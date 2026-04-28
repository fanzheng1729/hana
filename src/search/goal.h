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
    Goal() : rpn(), typecode("") {}
    Goal(Goalview view) : rpn(view.first), typecode(view.second) {}
    void fillast() const { if (ast.empty()) ast = ::ast(rpn); }
    RPNsize size() const { return rpn.size(); }
    operator RPNspanAST() const
    { fillast(); return RPNspanAST(rpn, ast); }
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
inline bool operator!=(Goal const & x, Goal const & y)
{
    return !(x == y);
}
inline bool operator<(Goal const & x, Goal const & y)
{
    int const cmp
    = util::compare(x.rpn.begin(), x.rpn.end(), y.rpn.begin(), y.rpn.end());
    return cmp < 0 || cmp == 0 && x.typecode < y.typecode;
}

#endif // GOAL_H_INCLUDED
