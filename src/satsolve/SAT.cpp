#include "DPLL.h"
typedef DPLL_solver Solver_used;

// Return true if the SAT instance is satisfiable.
bool CNFClauses::sat() const
{
    return empty() || Solver_used(*this).sat();
}

// Map: free atoms -> truth value.
// Return the empty vector if not okay.
Bvector CNFClauses::truthtable(Atom const nfree) const
{
    return Solver_used(*this).truthtable(nfree);
}
