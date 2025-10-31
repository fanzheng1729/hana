#ifndef ASS_H_INCLUDED
#define ASS_H_INCLUDED

#include <algorithm>    // for std::sort
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
    // # free variables
    Hypsize nfreevar;
    // Map: variable used in statement -> (is used in hyp i, is used in exp)
    Varusage varusage;
    // Index of key hypotheses: essential ones containing all free variables
    std::vector<Hypsize> keyhyps;
    // revPolish of expression, proof steps
    Proofsteps expRPN, proofsteps;
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
    // RPN of a hypothesis
    Proofsteps const & hypRPN(Hypsize index) const { return hyp(index).RPN; }
    // AST of a hypothesis
    AST const & hypAST(Hypsize index) const { return hyp(index).ast; }
    // Total length of RPNs of hypotheses, skipping trimmed ones
    Expression::size_type hypslen(Bvector const & hypstotrim = Bvector()) const
    {
        Expression::size_type len = 0;
        for (Hypsize i = 0; i < hypstotrim.size(); ++i)
            len += !hypstotrim[i] * hypRPN(i).size();
        for (Hypsize i = hypstotrim.size(); i < hypcount(); ++i)
            len += hypRPN(i).size();
        return len;
    }
    // Total length of RPNs of hypotheses, skipping trimmed ones
    Expression::size_type esshypslen(Bvector const & hypstotrim = Bvector()) const
    {
        Expression::size_type len = 0;
        for (Hypsize i = 0; i < hypstotrim.size(); ++i)
            len += !hypfloats(i) * !hypstotrim[i] * hypRPN(i).size();
        for (Hypsize i = hypstotrim.size(); i < hypcount(); ++i)
            len += !hypfloats(i) * hypRPN(i).size();
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
        std::string result;
        for (Hypsize i = 0; i < labels.size(); ++i)
            result += labels[i];
        return result;
    }
// Modifying functions
    // Set the hypotheses, trimming away specified ones.
    void sethyps(Assertion const & ass, Bvector const & hypstotrim = Bvector());
};

#endif // ASS_H_INCLUDED
