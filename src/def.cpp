#include "comment.h"
#include "def.h"
#include "disjvars.h"
#include "io.h"
#include "proof/analyze.h"
#include "proof/verify.h"
#include "relation.h"
#include "syntaxiom.h"
#include "util/for.h"
#include "util/util.h"  // for util::equal

// Find the revPolish notation of (LHS, RHS).
Definition::Definition(Assertions::const_reference rass) : pdef(NULL), count(0)
{
    strview label = rass.first;
    Assertion const & ass = rass.second;
    Proofsteps const & expRPN = ass.expRPN;
    AST const & tree = ast(expRPN);

    if (tree.empty())
    {
        std::cerr << ass.expression << "has bad RPN:\n" << expRPN;
        if (expRPN.empty())
            std::cerr << "Run Database::addRPN first" << std::endl;
        return;
    }

    // Index of the syntax to be defined
    Proofsize const lhsend = tree.back()[0];
    // Index of beginning of RHS
    Proofsize const defbegin = lhsend + 1;

    if (unexpected(tree.back().size() != 2, "non-binary root", expRPN.back()))
        return;

    // Check if LHS = (var, ..., var, syntax to be defined).
    if (tree[lhsend].size() != lhsend)
    {
        std::cerr << label << "\tbad LHS: " << expRPN;
        return;
    }

    pdef = &rass;
    lhs.assign(expRPN.begin(), expRPN.begin() + defbegin); // !empty
    rhs.assign(expRPN.begin() + defbegin, expRPN.end() - 1);
}

// Check the required disjoint variable hypotheses (errs 3 & 4).
bool Definition::checkdv() const
{
    if (!pdef) return false;

    Varusage const & vars = pdef->second.varusage;
    Disjvars const & disjvars = pdef->second.disjvars;
    for (Varusage::const_iterator iter(vars.begin()); iter!=vars.end(); ++iter)
    {
        Symbol2 var(iter->first, iter->first);
        Varusage::const_iterator iter2 = iter;
        for (++iter2; iter2 != vars.end(); ++iter2)
        {
            Symbol2 var2(iter2->first, iter2->first);
            if (disjvars.count(std::make_pair(var, var2))
                != (isdummy(iter->first) || isdummy(iter2->first)))
            {
                static const char * const dummy[] = {" not", ""};
std::cout << var << dummy[isdummy(iter->first)] << " dummy, & "
          << var2 << dummy[isdummy(iter2->first)] << " dummy, "
          << "violate disjoint variable hypothesis\n";
                return false;
            }
        }
    }

    return true;
}

// Return true if all dummy variables are bound (not fully implemented).
bool Definition::checkdummyvar(struct Typecodes const & typecodes) const
{
    FOR (Varusage::const_reference var, pdef->second.varusage)
        if (isdummy(var.first)&&typecodes.isbound(var.first.typecode())!=TRUE)
            return false;
    return true;
}

static void printerr(strview label, int err)
{
    static const char * const errs[] =
    {
        "", "has essential hypotheses", "root symbol not equality",
        "definition does not parse", "definition is circular",
        "bad disjoint variables", "has dummy non-set variables"
    };
    std::cout << label << '\t' << errs[err] << std::endl;
}

// Return true if an assertion is a definition. Return 0 if OK.
// Otherwise return error code.
Definition::Definition
    (Assertions::const_reference rass,
        struct Typecodes const & typecodes,
        struct Relations const & equalities) : pdef(NULL), count(0)
{
    int err = 0;
    Assertion const & ass = rass.second;
    if (ass.hypcount() > ass.varcount())
        printerr(rass.first, err = 1);
    if (!equalities.count(ass.expRPN.back().pass->first))
        printerr(rass.first, err = 2);
    if (err)
        return;
    *this = Definition(rass);
    if (!pdef)
    {
        printerr(rass.first, err = 3);
        return;
    }
    if (iscircular())
        printerr(rass.first, err = 4);
    if (!checkdv())
        printerr(rass.first, err = 5);
    if (!checkdummyvar(typecodes))
        printerr(rass.first, err = 6);
    if (err)
        pdef = NULL;
}

Definitions::Definitions
    (Assertions const & assertions,
        struct Commentinfo const & commentinfo,
        struct Relations const & equalities)
{
    // Add definitions.
    Typecodes const & typecodes = commentinfo.typecodes;
    FOR (Assertions::const_reference ass, assertions)
    {
        if (ass.first.remove_prefix("df-"))
            adddef(ass, typecodes, equalities);
    }
    // Adjust definitions.
    Ctordefns const & ctordefns = commentinfo.ctordefns;
    FOR (Ctordefns::const_reference ctor, ctordefns)
    {
        // df defines sa.
        strview sa = ctor.first, df = ctor.second;
        if (df.empty()) // Skip primitive syntax.
            continue;
        // Find assertion.
        Assiter newiter = assertions.find(df);
        if (unexpected(newiter == assertions.end(), "syntax def", df))
            continue;
        // Adjust definition.
        std::cout << sa << "\tredefined by " << df << std::endl;
        adddef(*newiter, typecodes, equalities);
    }
}

// Add a definition. Return true if okay.
bool Definitions::adddef
    (Assertions::const_reference rass,
        struct Typecodes const & typecodes,
        struct Relations const & equalities)
{
    Definition def(rass, typecodes, equalities);
    if (!def.pdef)
        return false;
    // Check for redefinition.
    strview ctor = def.pdef->second.expRPN[def.lhs.size() - 1];
    Definition & rdef = (*this)[ctor];
    if (unexpected(rdef.pdef, "redefinition of", ctor))
        return false;
    // Add definition.
    rdef = def;
    return true;
}

// Return true if all the definitions are syntactically okay.
bool checkdefinitions
    (Definitions const & definitions, Typecodes const & typecodes)
{
    FOR (Definitions::const_reference rdef, definitions)
    {
//std::cout << "Checking definition for " << r.first << std::endl;
        Definition const & definition = rdef.second;
        // Points to the corresponding assertion.
        pAss pdef = definition.pdef;
        if (unexpected(!pdef, "empty definition for", rdef.first))
            continue;
        // Construct the statement from the revPolish notation.
        Proofsteps RPN(definition.lhs);
        RPN += definition.rhs;
        RPN.push_back(pdef->second.expRPN.back());
        Expression const & result(verify(RPN, pdef));
        // Check if it agrees with the expression.
        Expression const & exp = pdef->second.expression;
        bool okay(!result.empty() && result[0]==typecodes.normalize(exp[0]));
        okay &= util::equal(result.begin() + 1, result.end(),
                            exp.begin() + 1, exp.end());
        if (!okay)
        {
            std::cerr << exp << "Def syntax proof shows\n" << result;
            return false;
        }
    }

    return true;
}
