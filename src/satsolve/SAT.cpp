#include "DPLL.h"
typedef DPLL_solver Solver_used;

// Return true if the SAT instance is satisfiable.
bool CNFClauses::sat() const
{
    return empty() || (!hasemptyclause() && Solver_used(*this).sat());
}
// Return true if the SAT instance and the conclusion are satisfiable.
bool CNFClauses::sat(CNFClauses & conclusion) const
{
    if (empty() && conclusion.empty())
        return true;
    if (hasemptyclause() || conclusion.hasemptyclause())
        return false;
    conclusion.insert(conclusion.end(), begin(), end());
    return Solver_used(conclusion).sat();
}

// Map: free atoms -> truth value.
// Return the empty vector if unsuccessful.
Bvector CNFClauses::truthtable(Atom const nfree) const
{
    return Solver_used(*this).truthtable(nfree);
}
