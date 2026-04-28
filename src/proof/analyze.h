#ifndef ANALYZE_H_INCLUDED
#define ANALYZE_H_INCLUDED

#include "../types.h"

// Return the AST.
// Retval[i] = {index of hyp1, index of hyp2, ...}
// Return an empty AST if not okay. Only for uncompressed proofs
AST ast(RPN const & exp);

// Return the indentations of all the subASTs.
Indentations indentations(AST const & ast);

struct Goal;
// Return true if the RPN of an expression matches a template.
bool findsubst(RPNspanAST exp, RPNspanAST tmp, RPNspans & subst);
bool findsubst
    (Goal const & goal, Assiter iter, RPNspans & subst);

// All maximal abstractions governed by a syntax axiom.
GovernedRPNspansbystep maxabs(RPNspanAST exp);

// Map: goal -> theorems
typedef std::map<struct Goal, Assiters> Theorempool;

// Map usable theorems.
Theorempool theorempool
    (Assiters const & assiters, struct Typecodes const & typecodes);

// Find all assertions matching an expression.
Assiters usabletheorems
    (Theorempool const & pool, nAss limit,
     strview typecode, RPNspanAST exp);

#endif // ANALYZE_H_INCLUDED
