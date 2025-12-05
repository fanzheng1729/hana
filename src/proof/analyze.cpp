#include "../ass.h"
#include "../io.h"
#include "../util/algo.h"   // for util::equal
#include "compranges.h"
#include "verify.h"

// Subroutine for building AST.
// Process a proof step referencing an assertion (i.e., not a hypothesis).
// Add the corresponding node to the AST. Return true if okay.
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
AST ast(Proofsteps const & proof)
{
    std::vector<Proofsize> stack;
    // Preallocate for efficiency
    stack.reserve(proof.size());
    AST tree(proof.size());

    for (Proofsize i = 0; i < proof.size(); ++i)
    {
        Proofstep const step = proof[i];
        if (step.ishyp())
            stack.push_back(i);
        else if (step.isthm() &&
                 addASTnode(proof[i].pass->second, stack, tree[i]))
            continue;
        else return AST();
    }

    return stack.size() == 1 ? tree : AST();
}

// Return the indentations of all the subASTs.
static void indentations(ASTiter begin, ASTiter end, Indentations & result)
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
        ASTiter const newbegin = i ? end - (index - (*back)[i - 1]): begin;
        indentations(newbegin, newend, result);
    }
}
Indentations indentations(AST const & ast)
{
    Indentations result(ast.size());
    if (!ast.empty())
        indentations(ast.begin(), ast.end(), result);
    return result;
}

// Return true if the RPN of an expression matches a template.
static bool dofindsubstitutions
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
    case Proofstep::THM:
//std::cout << "Ctor " << tmproot << std::endl;
        if (exproot != tmproot)
            return false;
        // Check the children.
        {
            // Roots of both ASTs
            ASTnode const & expASTroot = exp.ASTroot();
            ASTnode const & tmpASTroot = tmp.ASTroot();
            // Check if children sizes match.
            if (expASTroot.size() != tmpASTroot.size())
                return false;
            // Match the children.
            for (ASTnode::size_type i = 0; i < expASTroot.size(); ++i)
                if (!dofindsubstitutions(exp.child(i), tmp.child(i), result))
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
    SteprangeAST const x(exp, expAST);
    SteprangeAST const y(pattern, patternAST);
    return !x.empty() && !y.empty() && dofindsubstitutions(x, y, result);
}

// Return true if range1 has all the variables in range2
static bool hasallvars(Steprange range1, Steprange range2)
{
    for (Stepiter iter2 = range2.first; iter2 < range2.second; ++iter2)
    {
        if (!iter2->ishyp())
            continue;
        if (std::find(range1.first, range1.second, *iter2) == range1.second)
            return false;
    }
    return true;
}

// Map: proofstep -> is in a tree governed by the step
typedef std::map<Proofstep, bool, std::less<const char *> > Instep;

static void maxranges
(SteprangeAST subexp, Steprange exp, Instep & instep,
    GovernedSteprangesbystep & result)
{
    Proofstep const root = *(subexp.first.second - 1);
    if (!root.isthm())
        return;
    // Root is THM.
    bool * pinstep = NULL;
    if (root != *(exp.second - 1))
    {
        // Not the same root as full expression
        pinstep = &instep[root];
        if (!*pinstep && hasallvars(subexp.first, exp))
        {
            // Record the range.
            GovernedStepranges & ranges = result[root];
            if (ranges.empty())
                ranges = GovernedStepranges(compranges);
            ranges[subexp.first] = true;
        }
        *pinstep = true;
    }
    // Recurse to all children.
    for (ASTnode::size_type i = 0; i < subexp.ASTroot().size(); ++i)
        maxranges(subexp.child(i), exp, instep, result);
    if (pinstep)
        *pinstep = false;
}

// Find all maximal ranges governed by a syntax axiom.
GovernedSteprangesbystep maxranges(Steprange range, AST ast)
{
    GovernedSteprangesbystep result;

    Instep instep;
    SteprangeAST exp(range, ast.begin());
    maxranges(exp, exp.first, instep, result);

    return result;
}
