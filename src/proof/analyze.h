#ifndef ANALYZE_H_INCLUDED
#define ANALYZE_H_INCLUDED

#include "../types.h"

// Return the AST.
// Retval[i] = {index of hyp1, index of hyp2, ...}
// Return an empty AST if not okay. Only for uncompressed proofs
AST ast(Proofsteps const & proof);

// Return the indentations of all the subASTs.
Indentations indentations(AST const & ast);

// Return true if the RPN of an expression matches a template.
bool findsubstitutions
    (Proofsteps const & exp, AST const & expAST,
     Proofsteps const & pattern, AST const & patternAST,
     Stepranges & result);

// Find all maximal ranges governed by a syntax axiom.
GovernedSteprangesbystep maxranges(Steprange range, AST ast);

#endif // ANALYZE_H_INCLUDED
