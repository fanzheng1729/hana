#ifndef ASS_H_INCLUDED
#define ASS_H_INCLUDED

#include <algorithm>// for std::sort and std::upper_bound
#include <numeric>  // for std::accumulate
#include "types.h"

// An axiom or a theorem.
struct Assertion
{
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
        { return hypexp(index).empty() ? "" : hypexp(index)[0].c_str; }
    // RPN of a hypothesis
    Proofsteps const & hypRPN(Hypsize index) const { return hyp(index).RPN; }
    // AST of a hypothesis
    AST const & hypAST(Hypsize index) const { return hyp(index).ast; }
    // length of a hypothesis
    Proofsize hyplen(Hypsize index) const { return hypRPN(index).size(); }
    // Total length of RPNs of hypotheses, skipping trimmed ones
    Proofsize hypslen(Bvector const & hypstotrim = Bvector()) const
    {
        Proofsize len = 0;
        for (Hypsize i = 0; i < hypstotrim.size(); ++i)
            len += !hypstotrim[i] * hyplen(i);
        for (Hypsize i = hypstotrim.size(); i < hypcount(); ++i)
            len += hyplen(i);
        return len;
    }
    // Total length of RPNs of essential hypotheses, skipping trimmed ones
    Proofsize esshypslen(Bvector const & hypstotrim = Bvector()) const
    {
        Proofsize len = 0;
        for (Hypsize i = 0; i < hypstotrim.size(); ++i)
            len += !hypfloats(i) * !hypstotrim[i] * hyplen(i);
        for (Hypsize i = hypstotrim.size(); i < hypcount(); ++i)
            len += !hypfloats(i) * hyplen(i);
        return len;
    }
    // Return index the hypothesis matching the expression.
    // If there is no match, return # hypotheses.
    // Length of the rev Polish notation of all necessary hypotheses combined
    Hypsize matchhyp(Expression const & exp) const
    {
        Hypsize i = 0;
        for ( ; i < hypcount(); ++i)
            if (hypexp(i) == exp) break;
        return i;
    }
    Hypsize matchhyp(Proofsteps const & RPN, strview typecode) const
    {
        Hypsize i = 0;
        for ( ; i < hypcount(); ++i)
        {
            Expression const & exp = hypexp(i);
            if (!exp.empty() && exp[0] == typecode && hypRPN(i) == RPN)
                break;
        }
        return i;
    }
    // Return true if the expression matches a hypothesis.
    bool istrivial(Expression const & exp) const
        { return matchhyp(exp) < hypcount(); }
    bool istrivial(Proofsteps const & RPN, strview typecode) const
        { return matchhyp(RPN, typecode) < hypcount(); }
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
        static char const delim = '~';
        std::vector<std::string> labels;
        for (Hypsize i = 0; i < hypiters.size(); ++i)
            if (!(i < hypstotrim.size() && hypstotrim[i]))
                labels.push_back(delim + std::string(hypiters[i]->first));

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
