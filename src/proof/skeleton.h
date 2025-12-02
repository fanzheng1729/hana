#ifndef SKELETON_H_INCLUDED
#define SKELETON_H_INCLUDED

// #include "../io.h"
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
            Assertion const & ass = root.pass->second;
            if (cansplit(ass)) // Propositional constructor
            {
                ASTiter ASTend = exp.first.second - exp.first.first + exp.second;
                ASTnode ASTroot = *(ASTend - 1);
                // Recurse to children.
                for (ASTnode::size_type i = 0; i < ASTroot.size(); ++i)
                    if (!skeleton(exp.child(i), cansplit, varbank, result))
                        return false;
                // Add the root
                result.push_back(root);
// std::cout << "Result " << result;
                return true;
            }
            else // Not propositional constructor
            {
                Proofsteps subRPN(exp.first.first, exp.first.second);
                // Find the abstracting variable.
                Symbol3 var = varbank.addRPN(subRPN);
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

#endif
