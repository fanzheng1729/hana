#ifndef SKELETON_H_INCLUDED
#define SKELETON_H_INCLUDED

// #include "../io.h"
#include "../proof/compranges.h"
#include "../util/algo.h"   // for util::equal
#include "../util/tribool.h"
#include "../bank.h"

enum Splitretval {KEEPRANGE = 0, SPLITREC = 1, SPLITALL = 2};

// Add the skeleton of an RPN to result.
// Return UNKNOWN if unsuccessful.
// Otherwise return if anything has been abstracted.
template<class T> Tribool skeleton
    (SteprangeAST exp, T cansplit, Bank & bank, Proofsteps & result)
{
    Tribool retval = FALSE;
// std::cout << "Analyzing " << Proofsteps(exp.first.first, exp.first.second);
    Proofstep const root = *(exp.first.second - 1);
    switch (root.type)
    {
    case Proofstep::HYP:
        result.push_back(root);
// std::cout << "Result " << result;
        return FALSE;
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
            result.push_back(root);
// std::cout << "Result " << result;
            return retval;
        case KEEPRANGE:
            // Don't split and abstract.
            Proofsteps const subRPN(exp.first.first, exp.first.second);
            // Find the abstracting variable.
            Symbol3 const var = bank.addRPN(subRPN);
// std::cout << "varid " << var.id << std::endl;
            if (var.id == 0) // bad step
                return UNKNOWN;
            // Add the root
            result.push_back(var.phyp);
// std::cout << "Result " << result;
            return TRUE;
        }
    default:
        return UNKNOWN;
    }
}

struct Keeprange
{
    Steprange range;
    Keeprange(Steprange steprange) : range(steprange) {}
    Splitretval operator()(Steprange exp) const
    {
        if (range == exp)
            return KEEPRANGE;
        if (*(range.second - 1) == *(exp.second - 1))
            return SPLITALL;
        return SPLITREC;
    }
};

#endif
