#ifndef ANALYZE_H_INCLUDED
#define ANALYZE_H_INCLUDED

#include "../types.h"

// Return true if a proof has only 1 step using 1 theorem.
inline bool is1step(Proofsteps const & steps)
{
    if (steps.empty()) return false;
    // i = the first ASS step
    Proofsize i = 0;
    for ( ; i < steps.size() && steps[i].type != Proofstep::ASS; ++i) ;
    // Check if that step is the last one.
    return i == steps.size() - 1;
}

// Return the AST.
// Retval[i] = {index of hyp1, index of hyp2, ...}
// Return an empty AST if not okay. Only for uncompressed proofs
AST ast(Proofsteps const & steps);

// Return the indentations of all the subASTs.
std::vector<Proofsize> indentation(AST const & ast);

// Return true if the RPN of an expression matches a template.
bool findsubstitutions
    (Proofsteps const & exp, AST const & expAST,
     Proofsteps const & pattern, AST const & patternAST,
     Stepranges & result);

#endif // ANALYZE_H_INCLUDED
