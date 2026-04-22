#ifndef SKELETON_H_INCLUDED
#define SKELETON_H_INCLUDED

// #include "../io.h"
#include "compspans.h"
#include "../util/tribool.h"

enum Splitretval {KEEPRANGE = 0, SPLITREC = 1, SPLITALL = 2};

// Bank with only 1 variable
struct Bank1var : Symbol3
{
    Bank1var(Symbol3 const var) : Symbol3(var) {}
    Symbol3 addabsvar(RPNspan) const { return *this; }
};

// Add a step to an RPN and returns true.
inline Tribool addstep(RPN & rpn, RPNstep const step)
{ rpn.push_back(step); return TRUE; }

// Add the skeleton of an RPN to result.
// Return UNKNOWN if unsuccessful.
// Otherwise return if anything has been abstracted.
template<class T, class B>
Tribool skeleton
    (RPNspanAST const exp, T const cansplit, B & bank, RPN & result)
{
    Tribool retval = FALSE;
// std::cout << "Analyzing " << exp.first;
    RPNstep const root = exp.RPNroot();
    switch (root.type)
    {
    case RPNstep::HYP:
        return result.push_back(root), FALSE;
    case RPNstep::THM:
        switch (cansplit(exp.first))
        {
        case SPLITALL:
            // Copy the whole span.
            result.insert(result.end(), exp.first.first, exp.first.second);
            return FALSE;
        case SPLITREC:
            // Split and recurse to children.
            for (ASTnode::size_type i = 0; i < exp.ASTroot().size(); ++i)
            {
                switch (skeleton(exp.child(i), cansplit, bank, result))
                {
                case UNKNOWN:
                    return UNKNOWN;
                case FALSE:
                    continue;
                case TRUE:
                    retval = TRUE;
                }
            }
            // Add the root
            return result.push_back(root), retval;
        case KEEPRANGE:
            // Don't split and abstract. Find the abstracting variable.
            Symbol3 const var = bank.addabsvar(exp.first);
// std::cout << "varid " << var.id << std::endl;
            return var.id ? addstep(result, var.iter) : UNKNOWN;
        }
    default:
        return UNKNOWN;
    }
}

struct Keepspan
{
    RPNspan span;
    Keepspan(RPNspan const tokeep) : span(tokeep) {}
    Splitretval operator()(RPNspan const exp) const
    {
        if (span == exp)
            return KEEPRANGE;
        if (span.size() > exp.size())
            return SPLITALL;
        return SPLITREC;
    }
};

#endif
