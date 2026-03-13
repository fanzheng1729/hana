#ifndef ANALYZE_H_INCLUDED
#define ANALYZE_H_INCLUDED

#include "../types.h"

// Return the AST.
// Retval[i] = {index of hyp1, index of hyp2, ...}
// Return an empty AST if not okay. Only for uncompressed proofs
AST ast(RPN const & exp);

// Return the indentations of all the subASTs.
Indentations indentations(AST const & ast);

// Return true if the RPN of an expression matches a template.
bool findsubst(RPNspanAST exp, RPNspanAST tmp, Stepranges & subst);

// Find all maximal abstractions governed by a syntax axiom.
GovernedSteprangesbystep maxabs(Steprange range, AST ast);

#endif // ANALYZE_H_INCLUDED
