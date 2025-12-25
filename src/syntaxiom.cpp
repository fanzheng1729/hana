#include <algorithm>
#include "database.h"
#include "disjvars.h"
#include "io.h"
#include "parse.h"
#include "proof/analyze.h"
#include "proof/verify.h"
#include "syntaxiom.h"
#include "typecode.h"
#include "util/for.h"
#include "util/iter.h"
#include "util/progress.h"

// Map syntax axioms.
void Syntaxioms::_map()
{
    FOR (const_reference axiom, *this)
    {
        // set of constants in the syntax axiom.
        Constants const & constants = axiom.second.constants;
        // Find the constant whose vector is shortest.
        typedef M_map_type::mapped_type Minvec;
        Minvec * pminvec = NULL;

        FOR (strview str, constants)
        {
            // *iter2 = a constant in the set.
            Minvec & vec = m_map[str];
            // Update shortest vector and its size.
            if (!pminvec || vec.size() < pminvec->size())
            {
                pminvec = &vec;
                if (vec.empty())
                    break; // New constant found.
            }
        }
        // Add the syntax axiom.
        if (!pminvec) // No constant in the syntax axiom.
            pminvec = &m_map[""];
        pminvec->push_back(&axiom);
    }
}

// Filter assertions for syntax axioms, i.e.,
// those starting with a primitive type code and having no essential hypothesis
// and find their var types.
Syntaxioms::Syntaxioms
    (Assertions const & assertions, class Database const & database)
{
//std::cout << "Syntax axioms:" << std::endl;
    for (Assiter iter = assertions.begin(); iter != assertions.end(); ++iter)
    {
        if ((iter->second.type & Asstype::AXIOM) == 0)
            continue; // Skip theorems.

        Expression const & exp = iter->second.expression;
        if (unexpected(exp.empty(), "Empty expression", iter->first))
            continue;
        // Check if it starts with a primitive type code and has no essential hyps.
        if (database.typecodes().isprimitive(exp[0]) == TRUE &&
            iter->second.nEhyps() == 0)
        {
            Syntaxiom & syntaxiom = (*this)[iter->first];
            syntaxiom.pass = &*iter;
            // Fill constants of the syntax axiom.
            std::remove_copy_if(exp.begin() + 1, exp.end(),
                                end_inserter(syntaxiom.constants), id);
//std::cout << iter->first << ' ';
        }
    }
//std::cout << std::endl;
    _map();
}

void Syntaxioms::addfromvec
    (Constants const & expconstants, std::vector<const_pointer> const * pv)
{
    if (!pv) return;

    FOR (const_pointer p, *pv)
    {
//std::cout << p->first << ' ';
        Constants const & constants = p->second.constants;
        // If the constants in the syntax axiom are all found in exp,
        if (std::includes(expconstants.begin(), expconstants.end(),
                          constants.begin(), constants.end()))
        {
            // add it to the set of filtered syntax axioms.
            insert(*p);
//std::cout << "\\/ ";
        }
    }
}

// Filter syntax axioms whose constants are all in exp.
Syntaxioms Syntaxioms::filterbyexp(Expression const & exp) const
{
    Syntaxioms filtered;
// std::cout << "Filtering syntax axioms against: " << exp;
    // Constants in exp
    Constants exp2;
    std::remove_copy_if(exp.begin() + 1, exp.end(),
                        end_inserter(exp2), id);
    // Scan and add syntax axioms with no constant symbols.
    filtered.addfromvec(exp2, keyis(""));
    // Scan and add syntax axioms whose key appears in exp.
    FOR (strview str, exp2)
    {
// std::cout << str << ' ';
        filtered.addfromvec(exp2, keyis(str));
    }
// std::cout << std::endl;
    return filtered;
}

// Return the revPolish notation of exp. Return the empty proof iff not okay.
Proofsteps Syntaxioms::RPN
    (Expression const & exp, Assertion const & ass) const
{
    if (exp.empty()) return Proofsteps();

    Syntaxioms const & filtered(filterbyexp(exp));
    Subexprecords recs;
    Substframe::Subexpends const & ends
        (RPNmap(exp[0], exp.begin() + 1, exp.end(), exp, ass, filtered, recs));

    Substframe::Subexpends::const_iterator const iter(ends.find(exp.end()));
    return iter == ends.end() ? Proofsteps() : iter->second;
}

// Add the revPolish notation and its AST. Return true if okay.
bool Syntaxioms::RPNAST
    (Expression const & exp, Assertion const & ass,
     Proofsteps & rpn, AST & tree) const
{
    rpn = RPN(exp, ass);
    if (unexpected(rpn.empty(), "RPN error", exp))
        return false;

    tree = ast(rpn);
    return !unexpected(tree.empty(), "AST error", rpn);
}

// Add the revPolish notation of the whole assertion. Return true if okay.
bool Syntaxioms::addRPN
    (Assertion & ass, struct Typecodes const & typecodes) const
{
    Expression exp(ass.expression);
    if (exp.empty())
        return false;
    // Parse the expression.
    exp[0] = typecodes.normalize(exp[0]);
    if (!RPNAST(exp, ass, ass.expRPN, ass.expAST))
        return false;
    ass.expmaxabs = maxabs(ass.expRPN, ass.expAST);
    // Parse the hypotheses.
    for (Hypsize i = 0; i < ass.hypcount(); ++i)
    {
        if (ass.hypexp(i).empty())
            return false;

        Hypothesis & hyp = const_cast<Hypothesis &>(ass.hyp(i));
        if (!hyp.RPN.empty() && hyp.ast.size() == hyp.RPN.size())
            continue; // Hypothesis already parsed
        // Parse the hypothesis.
        if (ass.hypfloats(i))
        {
            // Floating hypothesis
            hyp.RPN.assign(1, ass.hypptr(i));
            hyp.ast.assign(1, ASTnode());
        }
        else
        {
            // Essential hypothesis
            exp = ass.hypexp(i);
            exp[0] = typecodes.normalize(exp[0]);
            if (!RPNAST(exp, ass, hyp.RPN, hyp.ast))
                return false;
        }
    }

    return true;
}

// Add the revPolish notation of a set of assertions. Return true if okay.
bool Syntaxioms::addRPN
    (Assertions & assertions, struct Typecodes const & typecodes) const
{
    Progress progress;

    Assertions::size_type count = 0, all = assertions.size();
    FOR (Assertions::reference ass, assertions)
    {
        if (!addRPN(ass.second, typecodes))
        {
            printass(ass, count);
            std::cerr << "\nRPN error!" << std::endl;
            return false;
        }
        progress << ++count/static_cast<Ratio>(all);
    }

    return true;
}

// Determine if proof is the revPolish notation for the expression of ass.
static bool checkRPN(Assertion const & ass, SteprangeAST exp)
{
    Stepranges stepranges(ass.maxvarid() + 1);
    bool const ok = findsubstitutions(exp, exp, stepranges);
    return !unexpected(!ok, "failed unification test for",
                        Proofsteps(exp.first.first, exp.first.second));
}

// Check the syntax of an assertion (& all hypotheses). Return true if okay.
bool Syntaxioms::checkRPN
    (Assertion ass, struct Typecodes const & typecodes) const
{
    Expression & exp = ass.expression;
    if (exp.empty())
        return false;

    exp[0] = typecodes.normalize(exp[0]);
    if (!::checkRPN(ass, ass.expRPNAST()))
        return false;

    for (Hypsize i = 0; i < ass.hypcount(); ++i)
    {
        if (ass.hypfloats(i))
            continue;

        exp = ass.hypexp(i);
        if (exp.empty())
            return false;

        exp[0] = typecodes.normalize(exp[0]);
        if (!::checkRPN(ass, ass.hypRPNAST(i)))
            return false;
    }

    return true;
}

// Test syntax parser. Return true if okay.
bool Database::checkRPN() const
{
    Progress progress;

    Assertions::size_type count = 0, all = assertions().size();
    FOR (Assertions::const_reference rass, assertions())
    {
        if (!syntaxioms().checkRPN(rass.second, typecodes()))
        {
            printass(rass, count);
            std::cerr << "\nRPN error!" << std::endl;
            return false;
        }
        progress << ++count/static_cast<Ratio>(all);
    }

    return true;
}
