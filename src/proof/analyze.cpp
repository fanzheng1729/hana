#include "../ass.h"
#include "../io.h"
#include "../util/arith.h"
#include "../util/filter.h"
#include "../util/util.h"   // for util::equal
#include "verify.h"

// Subroutine for building AST.
// Process a proof step referencing an assertion (i.e., not a hypothesis).
// Add the corresponding node to the AST.
// Return true if okay.
static bool addASTnode
    (Assertion const & assertion, std::vector<Proofsize> & stack,
     AST::reference node)
{
    Hypsize const hypcount = assertion.hypcount();
    if (!enoughitemonstack(hypcount, stack.size(), ""))
        return false;

    ASTnode::size_type const base = stack.size() - hypcount;
    Proofsize const index = stack.empty() ? 0 : stack.back() + 1;
    node.assign(stack.begin() + base, stack.end());
    // Remove hypotheses from stack.
    stack.erase(stack.begin() + base, stack.end());
    // Add conclusion to stack.
    stack.push_back(index);

    return true;
}

// Return the AST.
// Retval[i] = {index of hyp1, index of hyp2, ...}
// Return an empty AST if not okay. Only for uncompressed proofs
AST ast(Proofsteps const & steps)
{
    std::vector<Proofsize> stack;
    // Preallocate for efficiency
    stack.reserve(steps.size());
    AST tree(steps.size());

    for (Proofsize i = 0; i < steps.size(); ++i)
    {
        Proofstep::Type type = steps[i].type;
        if (type == Proofstep::HYP)
            stack.push_back(i);
        else if (type == Proofstep::ASS &&
                 addASTnode(steps[i].pass->second, stack, tree[i]))
            continue;
        else return AST();
    }

    return stack.size() == 1 ? tree : AST();
}

// Return the indentations of all the subASTs.
static void indentation(ASTiter begin, ASTiter end,
                        std::vector<Proofsize> & result)
{
    // Iterator to root of subAST
    ASTiter const back = end - 1;
    if (back->empty())
        return;
    // Index and level of root
    Proofsize const index = back->back() + 1, level = result[index];
    for (ASTnode::size_type i = 0; i < back->size(); ++i)
    {
        // Index of root of subAST
        Proofsize const subroot = (*back)[i];
        // Indent one more level.
        result[subroot] = level + 1;
        // Recurse to the subAST.
        ASTiter const newend = end - (index - subroot);
        ASTiter const newbegin = i == 0 ? begin : end - (index - (*back)[i-1]);
        indentation(newbegin, newend, result);
    }
}
std::vector<Proofsize> indentation(AST const & ast)
{
    std::vector<Proofsize> result(ast.size());
    if (!ast.empty())
        indentation(ast.begin(), ast.end(), result);
    return result;
}

// Return true if the RPN of an expression matches a template.
static bool findsubstitutions
    (SteprangeAST exp, SteprangeAST tmp, Stepranges & result)
{
// std::cout << "Matching " << Proofsteps(exp.first.first, exp.first.second);
// std::cout << "Against " << Proofsteps(tmp.first.first, tmp.first.second);
    Proofstep exproot = *(exp.first.second-1), tmproot = *(tmp.first.second-1);
    switch(tmproot.type)
    {
    case Proofstep::HYP:
        {
            Symbol2::ID const id = tmproot.id();
// std::cout << "Var " << tmproot << " ID = " << id << std::endl;
            if (id == 0)
                return false;
            // Template hypothesis is floating. Check if it has been seen.
            if (result[id].second > result[id].first) // seen
                return util::equal(exp.first.first, exp.first.second,
                                   result[id].first, result[id].second);
            result[id] = exp.first;// unseen
            return true;
        }
    case Proofstep::ASS:
//std::cout << "Ctor " << tmproot << std::endl;
        if (exproot != tmproot)
            return false;
        // Check the children.
        {
            // Ends of both ASTs
            ASTiter expASTend = exp.first.second - exp.first.first + exp.second;
            ASTiter tmpASTend = tmp.first.second - tmp.first.first + tmp.second;
            // Backs of both ASTs
            ASTnode const & expASTroot = *(expASTend - 1);
            ASTnode const & tmpASTroot = *(tmpASTend - 1);
// std::cout << expASTroot << std::endl << tmpASTroot << std::endl;
            if (expASTroot.size() != tmpASTroot.size())
                return false; // Children size mismatch
            // Match the children.
            for (ASTnode::size_type i = 0; i < expASTroot.size(); ++i)
                if (!findsubstitutions(exp.child(i), tmp.child(i), result))
                    return false;
            // All children match.
            return true;
        }
    default:
        return false;
    }

    return true;
}

// Return true if the RPN of an expression matches a template.
bool findsubstitutions
    (Proofsteps const & exp, AST const & expAST,
     Proofsteps const & pattern, AST const & patternAST,
     Stepranges & result)
{
    if (exp.empty() || exp.size() != expAST.size() ||
        pattern.empty() || pattern.size() != patternAST.size())
        return false;
    SteprangeAST a(Steprange(exp.begin(), exp.end()), expAST.begin());
    SteprangeAST b(Steprange(pattern.begin(), pattern.end()), patternAST.begin());
    return findsubstitutions(a, b, result);
}
