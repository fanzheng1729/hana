#ifndef VERIFY_H_INCLUDED
#define VERIFY_H_INCLUDED

#include <algorithm>// for std::max and std::max_element
#include "../types.h"
#include "../util/for.h"

// Proof is a sequence of labels.
typedef std::vector<std::string> Proof;

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

// Append a subexpression to an expression.
template<class Iter>
Expression & operator+=(Expression & exp, std::pair<Iter, Iter> subexp)
{
    exp.insert(exp.end(), subexp.first, subexp.second);
    return exp;
}

struct Printer;
// Subroutine for proof verification. Verify proof steps.
Expression verify(Proofsteps const & proof, Printer & printer, pAss pass = NULL);
Expression verify(Proofsteps const & proof, pAss pass = NULL);

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
