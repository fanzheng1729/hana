#include <algorithm>    // for std::replace_copy_if
#include "../ass.h"
#include "../io.h"
#include "../search/goal.h"
#include "../typecode.h"
#include "analyze.h"
#include "compspan.h"
#include "verify.h"

// Subroutine for building AST.
// Process a proof step referencing an assertion (i.e., not a hypothesis).
// Add the corresponding node to the AST. Return true if okay.
static bool addASTnode
    (Assertion const & assertion, std::vector<RPNsize> & stack,
     AST::reference node)
{
    Hypsize const nhyps = assertion.nhyps(), stacksz = stack.size();
    if (!enoughitemonstack(nhyps, stacksz, ""))
        return false;

    ASTnode::size_type const base = stack.size() - nhyps;
    RPNsize const index = stack.empty() ? 0 : stack.back() + 1;
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
AST ast(RPN const & exp)
{
    std::vector<RPNsize> stack;
    // Preallocate for efficiency
    stack.reserve(exp.size());
    AST tree(exp.size());

    for (RPNsize i = 0; i < exp.size(); ++i)
    {
        RPNstep const step = exp[i];
        if (step.id())
            stack.push_back(i);
        else
        if (step.isthm() &&
            addASTnode(exp[i].pass->second, stack, tree[i]))
            continue;
        else
            return AST();
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
    RPNsize const index = back->back() + 1, level = result[index];
    for (ASTnode::size_type i = 0; i < back->size(); ++i)
    {
        // Index of root of subAST
        RPNsize const subroot = (*back)[i];
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
extern std::size_t nfind = 0;
// Return true if the RPN of an expression matches a template.
bool findsubst(RPNspanAST exp, RPNspanAST tmp, RPNspans & subst)
{
    ++nfind;
    if (exp.empty() || tmp.empty() || exp.size() < tmp.size())
        return false;
// std::cout << "Matching " << exp.first << "Against " << tmp.first;
    RPNstep exproot = *(exp.first.second-1), tmproot = *(tmp.first.second-1);
    switch(tmproot.type)
    {
    case RPNstep::HYP:
        if (Symbol2::ID const id = tmproot.id())
        {
// std::cout << "Var " << tmproot << " ID = " << id << std::endl;
            // Template hypothesis is floating. Check if it has been seen.
            if (subst[id].second > subst[id].first) // seen
                return exp.first == subst[id];
            subst[id] = exp.first;// unseen
            return true;
        }
        else
            return false;
    case RPNstep::THM:
//std::cout << "Ctor " << tmproot << std::endl;
        if (exproot != tmproot)
            return false;
        // Match children.
        for (ASTnode::size_type i = 0; i < exp.nchild(); ++i)
            if (!findsubst(exp.child(i), tmp.child(i), subst))
                return false;
        return true;
    default:
        return false;
    }
}
bool findsubst
    (Goal const & goal, Assiter iter, RPNspans & subst)
{
    Assertion const & ass = iter->second;
    subst.assign(ass.maxvarid + 1, RPNspan());
    return findsubst(goal, ass.expRPNAST(), subst);
}
Assiters findsubst
    (Goal const & goal, Assiters const & assiters, nAss limit)
{
    Assiters result;

    FOR (Assiter const iter, assiters)
    {
        nAss n = iter->second.number;
        if (n == 0 || n >= limit)
            continue;
        RPNspans subst;
        if (findsubst(goal, iter, subst))
            result.push_back(iter);
    }

    return result;
}

// Return true if span1 has all the variables in span2
static bool hasallvars(RPNspan span1, RPNspan span2)
{
    for (RPNiter iter2 = span2.first; iter2 < span2.second; ++iter2)
    {
        if (!iter2->id())
            continue;
        if (std::find(span1.first, span1.second, *iter2) == span1.second)
            return false;
    }
    return true;
}

static Hypsize number(RPNstep x)
{ return x.ishyp() ? 0-x.var().id : x.isthm() ? x.pass->second.number : 0; }
static bool compstepstable(RPNstep x, RPNstep y)
{ return number(x) < number(y); }
static bool compspanstable(RPNspan x, RPNspan y)
{
    return std::lexicographical_compare
    (x.first, x.second, y.first, y.second, compstepstable);
}

// Map: RPNstep -> is in a tree governed by the step
typedef std::map<RPNstep, bool, std::less<const char *> > Instep;

static void maxabs
(RPNspanAST subexp, RPNspan exp, Instep & instep,
    GovernedRPNspansbystep & result)
{
    RPNstep const root = subexp.RPNroot();
    if (!root.isthm())
        return;
    // Root is THM.
    bool * pinstep = NULL;
    if (root != exp.root())
    {
        // Not the same root as full expression
        pinstep = &instep[root];
        if (!*pinstep && hasallvars(subexp.first, exp))
        {
            // Record the spans.
            GovernedRPNspans & spans = result[root];
            if (spans.empty())
                spans = GovernedRPNspans(compspanstable);
            spans[subexp.first] = ast(subexp.first);
        }
        *pinstep = true;
    }
    // Recurse to all children.
    for (ASTnode::size_type i = 0; i < subexp.nchild(); ++i)
        maxabs(subexp.child(i), exp, instep, result);
    if (pinstep)
        *pinstep = false;
}

// All maximal abstractions governed by a syntax axiom.
GovernedRPNspansbystep maxabs(RPNspanAST exp)
{
    GovernedRPNspansbystep result(compstepstable);

    Instep instep;
    maxabs(exp, exp.first, instep, result);

    return result;
}

Theorempool theorempool
    (Assiters const & assiters, struct Typecodes const & typecodes)
{
    Theorempool result;

    FOR (Assiter const iter, assiters)
    {
        if (iter == Assiter())
            continue;
    
        Assertion const & ass = iter->second;
        if (ass.testtype(Asstype::USELESS))
            continue;
    
        strview typecode = ass.exptypecode();
        if (typecodes.isprimitive(typecode) != FALSE)
            continue;

        RPN rpn(ass.expRPN.size());
        std::replace_copy_if(ass.expRPN.begin(), ass.expRPN.end(),
                             rpn.begin(), id, RPNstep());

        Goalview goal(rpn, typecode);
        result[goal].push_back(iter);
    }

    return result;
}

// All RPN skeletons of an RPN
typedef std::set<RPN> RPNs;

static RPNs profile(std::vector<RPNs> const & children, RPNstep root)
{
    RPNs result;

    if (root.id())
    {
        result.insert(RPN(1));
        return result;
    }
    if (root.isthm())
        if (children.empty())
        {
            result.insert(RPN(1));
            result.insert(RPN(1, root));
            return result;
        }
        else
        {
            result.insert(RPN(1));

            typedef std::vector<RPNs::const_iterator> Stack;
            Stack stack(children.size());
            for (Stack::size_type i = 0; i < stack.size(); ++i)
                stack[i] = children[i].begin();
            do
            {
                // New RPN
                RPN rpn;
                for (Stack::size_type i = 0; i < stack.size(); ++i)
                    rpn += *stack[i];
                rpn.push_back(root);
                result.insert(rpn);
                // Move stack forward.
                Stack::size_type i = stack.size() - 1;
                while (++stack[i] == children[i].end())
                {
                    stack[i] = children[i].begin();
                    if (i == 0) return result;
                    --i;
                }
            } while (true);
        }

    return result;
}

// Find all RPN skeletons of an RPN.
static RPNs profile(RPNspanAST exp)
{
    ASTnode::size_type const n = exp.nchild();
    std::vector<RPNs> children(n);
    for (ASTnode::size_type i = 0; i < n; ++i)
        children[i] = profile(exp.child(i));
RPNs result(profile(children, exp.RPNroot()));
std::cout << "size " << result.size() << std::endl;
    return result;
    return profile(children, exp.RPNroot());
}

static void additers(Assiters const & src, Assiters & dest, nAss limit)
{
    FOR (Assiter const iter, src)
        if (iter != Assiter())
            if (nAss const n = iter->second.number)
                if (n < limit)
                    dest.push_back(iter);
}

static bool compassiter(Assiter x, Assiter y)
{
    return (x == Assiter() ? 0 : x->second.number)
            < (y == Assiter() ? 0 : y->second.number);
}

// Find all assertions matching a set of skeletons.
static Assiters usabletheorems
    (Theorempool const & pool, nAss limit,
     strview typecode, RPNs const & rpns)
{
    Assiters result;
    result.reserve(limit);

    FOR (RPN const & rpn, rpns)
    {
        Goalview goal(rpn, typecode);
        Theorempool::const_iterator iter = pool.find(goal);
        if (iter != pool.end())
            additers(iter->second, result, limit);
    }

    std::sort(result.begin(), result.end(), compassiter);

    return result;
}

// Find all assertions matching an expression.
Assiters usabletheorems
    (Theorempool const & pool, nAss limit,
     strview typecode, RPNspanAST exp)
{
    return usabletheorems(pool, limit, typecode, profile(exp));
}
