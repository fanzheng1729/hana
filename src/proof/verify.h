#ifndef VERIFY_H_INCLUDED
#define VERIFY_H_INCLUDED

#include <algorithm>    // for std::max
#include "printer.h"
#include "../util/for.h"

// Extract proof steps from a compressed proof.
Proofsteps compressed
    (Proofsteps const & labels, Proofnumbers const & proofnumbers);
// Extract proof steps from a regular proof.
Proofsteps regular
    (Proof const & proof,
     Hypotheses const & hypotheses, Assertions const & assertions);

// Return true if there are enough items on the stack for hypothesis verification.
bool enoughitemonstack
    (std::size_t hypcount, std::size_t stacksize, strview label);

void printunificationfailure
    (strview label, strview thmlabel, Hypothesis const & hyp,
     Expression const & dest, Expression const & stackitem);

// Append a subexpression to an expression.
template<class Iter>
Expression & operator+=(Expression & exp, std::pair<Iter, Iter> subexp)
{
    exp.insert(exp.end(), subexp.first, subexp.second);
    return exp;
}

// Preallocate space for substitutions.
template<class SUB>
void prealloc(std::vector<SUB> & substitutions, Varusage const & vars)
{
    if (vars.empty()) return;
    // Maximal id of variables appearing
    Symbol2::ID const maxid=std::max_element(vars.begin(), vars.end())->first;
    // Allocate space for 0 (not used), 1, ..., maxid.
    substitutions.assign(maxid + 1, SUB());
}

template<class TOK, class SUB, class IDFUN>
void makesubstitution
    (std::vector<TOK> const & src, std::vector<TOK> & dest,
     std::vector<SUB> const & substitutions, IDFUN idfun)
{
    if (substitutions.empty())
        return dest.assign(src.begin(), src.end());
    // Make the substitution
    FOR (TOK symbol, src)
        if (Symbol2::ID const id = idfun(symbol))
            dest += substitutions[id];  // variable with an id
        else
            dest.push_back(symbol);     // constant with no id
}

// Find the substitution. Increase the size of the stack by 1.
// Return index of the base of the substitution in the stack.
// Return the size of the stack if not okay.
template<class HYPS, class EXP, class SUB>
typename std::vector<EXP>::size_type findsubstitutions
    (strview label, strview thmlabel, HYPS const & hypotheses,
     std::vector<EXP> & stack, std::vector<SUB> & substitutions)
{
    typename HYPS::size_type const hypcount = hypotheses.size();
    if (!enoughitemonstack(hypcount, stack.size(), label))
        return stack.size();

    // Space for new statement onto stack
    stack.resize(stack.size() + 1);

    typename HYPS::size_type const base(stack.size() - 1 - hypcount);

    // Determine substitutions and check that we can unify
    for (typename HYPS::size_type i = 0; i < hypcount; ++i)
    {
        Hypothesis const & hypothesis = hypotheses[i]->second;
        if (hypothesis.floats)
        {
            // Floating hypothesis of the referenced assertion
            if (hypothesis.expression[0] != stack[base + i][0])
            {
                printunificationfailure(label, thmlabel, hypothesis,
                                        hypothesis.expression, stack[base + i]);
                return stack.size();
            }
            Symbol2::ID const id = hypothesis.expression[1];
            substitutions.resize(std::max(id + 1, substitutions.size()));
            substitutions[id] = SUB(&stack[base + i][1],
                                    &stack[base + i].back() + 1);
        }
        else
        {
            // Essential hypothesis
            Expression dest;
            makesubstitution(hypothesis.expression, dest, substitutions,
                util::mem_fn(&Symbol3::id));
            if (dest != stack[base + i])
            {
                printunificationfailure(label, thmlabel, hypothesis,
                                        dest, stack[base + i]);
                return stack.size();
            }
        }
    }

    return base;
}

// Subroutine for proof verification. Verify proof steps.
Expression verify(Proofsteps const & proof, Printer & printer, pAss pass = NULL);
inline Expression verify(Proofsteps const & proof, pAss pass = NULL)
{
    Printer printer;
    return verify(proof, printer, pass);
}
// Verify a regular proof. The "proof" argument should be a non-empty sequence
// of valid labels. Return the statement the "proof" proves.
// Return the empty expression if the "proof" is invalid.
// Expression verifyregular
//     (strview label, class Database const & database,
//      Proof const & proof, Hypotheses const & hypotheses);

// Return if "conclusion" == "expression" to be proved.
bool checkconclusion
    (strview label,
     Expression const & conclusion, Expression const & expression);

#endif // VERIFY_H_INCLUDED
