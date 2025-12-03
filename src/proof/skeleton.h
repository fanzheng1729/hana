#ifndef SKELETON_H_INCLUDED
#define SKELETON_H_INCLUDED

// #include "../io.h"
#include "../util/algo.h"   // for util::equal
#include "../varbank.h"

// Add the skeleton of an RPN to result. Return true if okay.
template<class T> bool skeleton
    (SteprangeAST exp, T cansplit, Varbank & varbank, Proofsteps & result)
{
// std::cout << "Analyzing " << Proofsteps(exp.first.first, exp.first.second);
    Proofstep const root = *(exp.first.second - 1);
    switch (root.type)
    {
    case Proofstep::HYP:
        result.push_back(root);
// std::cout << "Result " << result;
        return true;
    case Proofstep::THM:
        {
            if (cansplit(exp.first)) // Propositional constructor
            {
                // Recurse to children.
                for (ASTnode::size_type i = 0; i < exp.ASTroot().size(); ++i)
                    if (!skeleton(exp.child(i), cansplit, varbank, result))
                        return false;
                // Add the root
                result.push_back(root);
// std::cout << "Result " << result;
                return true;
            }
            else // Not propositional constructor
            {
                Proofsteps const subRPN(exp.first.first, exp.first.second);
                // Find the abstracting variable.
                Symbol3 const var = varbank.addRPN(subRPN);
// std::cout << "varid " << var.id << std::endl;
                if (var.id == 0) // bad step
                    return false;
                // Add the root
                result.push_back(var.phyp);
// std::cout << "Result " << result;
                return true;
            }
        }
    default:
        return false;
    }
}

struct Keeprange
{
    Steprange range;
    Keeprange(Steprange steprange) : range(steprange) {}
    bool operator()(Steprange other) const
    {
        return
        !util::equal(range.first, range.second, other.first, other.second);
    }
};

#endif
