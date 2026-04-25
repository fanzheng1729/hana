#ifndef ASS_H_INCLUDED
#define ASS_H_INCLUDED

#include <algorithm>// for std::upper_bound
#include "types.h"

// Hypothesis label delimiter
static const std::string hypdelim = "~";

// An axiom or a theorem
struct Assertion
{
    Assertion(Assertions::size_type n = 0) : number(n), tokenpos(0), type(0) {}
// Essential properties:
    // Hypotheses of this axiom or theorem
    Hypiters hypiters;
    // Disjoint variable hypotheses of this axiom or theorem
    Disjvars disjvars;
    // Statement of axiom or theorem
    Expression expression;
    // # of axiom or theorem as ordered in the input file, starting from 1
    Assertions::size_type number;
    // Position in tokens
    std::size_t tokenpos;
// Derived properties:
    // Map: variable used in statement -> (is used in hyp i, is used in exp)
    Varusage varusage;
    // Max id of variables
    Symbol2::ID maxvarid;
    // (# free variables in hyp i, total # free variables)
    Hypsizes nfreevars;
    // Indices of hypotheses containing most to fewest free variables
    Hypsizes hypsorder;
    // revPolish of expression, proof steps
    RPN expRPN, proof;
    // Abstract syntax tree of expression
    AST expAST;
    // All maximal spans of the expression governed by a syntax axiom.
    GovernedRPNspansbystep expmaxabs;
    // Type (propositional, predicate, etc)
    unsigned type;
// Functions:
    RPNspanAST expRPNAST() const { return RPNspanAST(expRPN, expAST); }
    // Typecode of the conclusion
    strview exptypecode() const
        { return expression.empty() ? strview() : expression[0]; }
    // # of variables
    Hypsize nvars() const { return varusage.size(); }
    // # of hypotheses
    Hypsize nhyps() const {return hypiters.size();}
    // # of essential hypotheses
    Hypsize nEhyps() const { return nhyps() - nvars(); }
    // hypothesis pointer
    pHyp hypptr(Hypsize index) const { return &*hypiters[index]; }
    // label of a hypothesis
    strview hyplabel(Hypsize index) const { return hypptr(index)->first; }
    // Hypothesis
    Hypothesis const & hyp(Hypsize index) const { return hypptr(index)->second; }
    // Return true if a hypothesis is floating
    bool hypfloats(Hypsize index) const { return hyp(index).floats; }
    // Expression of a hypothesis
    Expression const & hypexp(Hypsize index) const { return hyp(index).expression; }
    // Typecode of a hypothesis
    strview hyptypecode(Hypsize index) const
        { return hypexp(index).empty() ? strview() : hypexp(index)[0]; }
    // rev-Polish notation and abstract syntax tree of a hypothesis
    RPN const & hypRPN(Hypsize index) const { return hyp(index).rpn; }
    AST const & hypAST(Hypsize index) const { return hyp(index).ast; }
    RPNspanAST hypRPNAST(Hypsize index) const
        { return RPNspanAST(hypRPN(index), hypAST(index)); }
    // Length of a hypothesis
    RPNsize hyplen(Hypsize index) const { return hypRPN(index).size(); }
    // Total length of RPNs of hypotheses
    RPNsize hypslen() const
    {
        RPNsize sum = 0;
        for (Hypsize i = 0; i < nhyps(); ++i)
            sum += hyplen(i);
        return sum;
    }
    // Return true if the expression matches a hypothesis.
    bool istrivial() const
    {
        Hypsize i = 0;
        for ( ; i < nhyps(); ++i)
            if (hypexp(i) == expression) return true;
        return false;
    }
    // # free variables in the conclusion
    Hypsize nfreevar() const
        { return nfreevars.empty() ? 0 : nfreevars.back(); }
    // # free variables in a hypothesis
    Hypsize nfreevar(Hypsize index) const
        { return nfreevars.empty() ? 0 : nfreevars[index]; }
    // # hypotheses containing free variables
    Hypsize nfreehyps() const
    {
        return hypsorder.rend() -
        std::upper_bound(hypsorder.rbegin() + 1, hypsorder.rend(), 0);
    }
    // Return true if there is a var only used in exp.
    bool hasnewvarinexp() const
    {
        for (Varusage::const_iterator iter = varusage.begin();
             iter != varusage.end(); ++iter)
        {
            Bvector const & usage = iter->second;
            Bvector::const_iterator const begin = usage.begin();
            Bvector::const_iterator const end = begin + nhyps();
            if (std::find(begin, end, true) == end)
                return true; // Variable used only in exp.
        }
        return false;
    }
    // Remove unnecessary variables.
    Bvector trimvars(Bvector const & hypstotrim, RPN const & conclusion) const
    {
        Bvector result(hypstotrim);
        return trimvars(result, conclusion);
    }
    Bvector & trimvars(Bvector & hypstotrim, RPN const & conclusion) const;
    // Label with hypotheses trimmed
    std::string hypslabel(Bvector const & hypstotrim = Bvector()) const;
    // Label with new variables and new hypotheses added
    std::string hypslabel
        (Expression const & newvars, Hypiters const & newhyps) const;
    // Simplified assertion with hypotheses trimmed
    Assertion makeAss(Bvector const & hypstotrim = Bvector()) const;
    // Test if mask is set in type.
    bool testtype(unsigned mask) const { return type & mask; }
// Modifying functions
    // Set the hypotheses, trimming away specified ones.
    void sethyps(Assertion const & ass, Bvector const & hypstotrim = Bvector());
    // Set the hypotheses, adding new variables and new hypotheses.
    void sethyps(Assertion const & ass,
                 Expression const & newvars, Hypiters const & newhyps);
    // Set the type mask. Return the current type.
    unsigned settype(unsigned mask) { return type |= mask; }
};

#endif // ASS_H_INCLUDED
