#ifndef ASS_H_INCLUDED
#define ASS_H_INCLUDED

#include <algorithm>// for std::sort and std::upper_bound
#include <numeric>  // for std::accumulate
#include "types.h"

// Hypothesis label delimiter
static const std::string hypdelim = "~";

// An axiom or a theorem
struct Assertion
{
    Assertion()
    : number(0), tokenpos(0), type(0) {}
// Essential properties:
    // Hypotheses of this axiom or theorem
    Hypiters hypiters;
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
    // (# free variables in hyp i, total # free variables)
    Hypsizes nfreevars;
    // Indices of hypotheses containing most to fewest free variables
    Hypsizes hypsorder;
    // revPolish of expression, proof steps
    Proofsteps expRPN, proof;
    // Abstract syntax tree of expression
    AST expAST;
    // Type (propositional, predicate, etc)
    unsigned type;
// Functions:
    // Typecode of the conclusion
    strview exptypecode() const
        { return expression.empty() ? strview() : expression[0]; }
    // # of hypotheses
    Hypsize hypcount() const {return hypiters.size();}
    // # of variables
    Varusage::size_type varcount() const { return varusage.size(); }
    // # of essential hypotheses
    Hypsize esshypcount() const { return hypcount() - varcount(); }
    // label of a hypothesis
    strview hyplabel(Hypsize index) const { return hypiters[index]->first; }
    // Hypothesis
    Hypothesis const & hyp(Hypsize index) const { return hypiters[index]->second; }
    // Return true if a hypothesis is floating
    bool hypfloats(Hypsize index) const { return hyp(index).floats; }
    // Expression of a hypothesis
    Expression const & hypexp(Hypsize index) const { return hyp(index).expression; }
    // Typecode of a hypothesis
    strview hyptypecode(Hypsize index) const
        { return hypexp(index).empty() ? strview() : hypexp(index)[0]; }
    // RPN of a hypothesis
    Proofsteps const & hypRPN(Hypsize index) const { return hyp(index).RPN; }
    // AST of a hypothesis
    AST const & hypAST(Hypsize index) const { return hyp(index).ast; }
    // Length of a hypothesis
    Proofsize hyplen(Hypsize index) const { return hypRPN(index).size(); }
    // Total length of RPNs of hypotheses
    Proofsize hypslen() const
    {
        Proofsize sum = 0;
        for (Hypsize i = 0; i < hypcount(); ++i)
            sum += hyplen(i);
        return sum;
    }
    // Return true if the expression matches a hypothesis.
    bool istrivial() const
    {
        Hypsize i = 0;
        for ( ; i < hypcount(); ++i)
            if (hypexp(i) == expression) return true;
        return false;
    }
    // # free variables
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
    // Remove unnecessary variables.
    Bvector trimvars
        (Bvector const & hypstotrim, Proofsteps const & conclusion) const
    {
        Bvector result(hypstotrim);
        return trimvars(result, conclusion);
    }
    Bvector & trimvars
        (Bvector & hypstotrim, Proofsteps const & conclusion) const;
    // Returns a label for a collection of hypotheses.
    std::string hypslabel(Bvector const & hypstotrim = Bvector()) const
    {
        // Preallocate for efficiency.
        std::vector<std::string> labels;
        labels.reserve(hypiters.size());
        for (Hypsize i = 0; i < hypstotrim.size(); ++i)
            if (!hypstotrim[i])
                labels.push_back(hypdelim + std::string(hypiters[i]->first));
        for (Hypsize i = hypstotrim.size(); i < hypiters.size(); ++i)
            labels.push_back(hypdelim + std::string(hypiters[i]->first));

        std::sort(labels.begin(), labels.end());
        return std::accumulate(labels.begin(), labels.end(), std::string());
    }
    // Return true if all variables in the assertion have been substituted.
    bool allvarsfilled(Stepranges const & stepranges) const
    {
        for (Varusage::const_iterator iter = varusage.begin();
             iter != varusage.end(); ++iter)
            if (stepranges[iter->first].first == stepranges[iter->first].second)
                return false;
        return true;
    }
// Modifying functions
    // Set the hypotheses, trimming away specified ones.
    void sethyps(Assertion const & ass, Bvector const & hypstotrim = Bvector());
};

#endif // ASS_H_INCLUDED
