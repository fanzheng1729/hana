#ifndef SKELETON_H_INCLUDED
#define SKELETON_H_INCLUDED

// #include "../io.h"
#include "compranges.h"
#include "../util/tribool.h"

enum Splitretval {KEEPRANGE = 0, SPLITREC = 1, SPLITALL = 2};

// Add the skeleton of an RPN to result.
// Return UNKNOWN if unsuccessful.
// Otherwise return if anything has been abstracted.
template<class T, class B>
Tribool skeleton
    (SteprangeAST exp, T cansplit, B & bank, Proofsteps & result)
{
    Tribool retval = FALSE;
// std::cout << "Analyzing " << Proofsteps(exp.first.first, exp.first.second);
    Proofstep const root = exp.RPNroot();
    switch (root.type)
    {
    case Proofstep::HYP:
        return result.push_back(root), FALSE;
    case Proofstep::THM:
        switch (cansplit(exp.first))
        {
        case SPLITALL:
            // Copy the whole range.
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
            return var ? (result.push_back(var.iter), TRUE) : UNKNOWN;
        }
    default:
        return UNKNOWN;
    }
}

struct Keeprange
{
    Steprange range;
    Keeprange(Steprange tokeep) : range(tokeep) {}
    Splitretval operator()(Steprange exp) const
    {
        if (range == exp)
            return KEEPRANGE;
        if (range.size() > exp.size())
            return SPLITALL;
        return SPLITREC;
    }
};

// Bank with only 1 variable
struct Bank1var : Symbol3
{
    Bank1var(Symbol3 var) { *static_cast<Symbol3 *>(this) = var; }
    Symbol3 addabsvar(Steprange) const { return *this; }
};

#endif
