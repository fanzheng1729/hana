#ifndef SKELETON_H_INCLUDED
#define SKELETON_H_INCLUDED

// #include "../io.h"
#include "../proof/compranges.h"
#include "../util/algo.h"   // for util::equal
#include "../util/tribool.h"
#include "../varbank.h"

enum {KEEPRANGE = 0, SPLITRANGE = 1, SPLITALL = 2};

// Add the skeleton of an RPN to result.
// Return UNKNOWN if unsuccessful.
// Otherwise return if anything has been abstracted.
template<class T> Tribool skeleton
    (SteprangeAST exp, T cansplit, Varbank & varbank, Proofsteps & result)
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
        {
            if (cansplit(exp.first))
            {
                // Split and recurse to children.
                for (ASTnode::size_type i = 0; i < exp.ASTroot().size(); ++i)
                {
                    switch (skeleton(exp.child(i), cansplit, varbank, result))
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
            }
            else
            {
                // Don't split and abstract.
                Proofsteps const subRPN(exp.first.first, exp.first.second);
                // Find the abstracting variable.
                Symbol3 const var = varbank.addRPN(subRPN);
// std::cout << "varid " << var.id << std::endl;
                if (var.id == 0) // bad step
                    return UNKNOWN;
                // Add the root
                result.push_back(var.phyp);
// std::cout << "Result " << result;
                return TRUE;
            }
        }
    default:
        return UNKNOWN;
    }
}

struct Keeprange
{
    Steprange range;
    Keeprange(Steprange steprange) : range(steprange) {}
    bool operator()(Steprange other) const
    {
        return range != other;
    }
};

#endif
