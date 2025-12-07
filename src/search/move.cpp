#include "../disjvars.h"
#include "move.h"

static Symbol3s symbols(Proofsteps const & RPN)
{
    Symbol3s set;

    FOR (Proofstep step, RPN)
        if (Symbol3 var = step.var())
            set.insert(var);

    return set;
}

// Return true if a move satisfies disjoint variable hypotheses.
bool Move::checkDV(Assertion const & ass, bool verbose) const
{
    if (!pthm)
        return true;
// std::cout << "Checking DV of move " << label() << std::endl;
    FOR (Disjvars::const_reference vars, theorem().disjvars)
    {
        const Proofsteps & RPN1 = substitutions[vars.first];
        const Proofsteps & RPN2 = substitutions[vars.second];
// std::cout << vars.first << ":\t" << RPN1 << vars.second << ":\t" << RPN2;
        if (!::checkDV
            (symbols(RPN1), symbols(RPN2), ass.disjvars, ass.varusage, verbose))
            return false;
    }

    return true;
}

// Abstract variables in use
Expression Move::absvars(Bank const & bank) const
{
    Expression result;
    // Preallocate for efficiency
    result.reserve(substitutions.size());
    for (Substitutions::size_type id = 1; id < substitutions.size(); ++id)
        if (Symbol3 const var = bank.var(id))
            result.push_back(var);
    return result;
}

// Add conjectures to a bank (must be of type CONJ).
// Return iterators to the hypotheses.
Hypiters Move::addconjsto(Bank & bank) const
{
    if (!isconj())
        return Hypiters();
    Hypiters result(conjcount());
    for (Hypsize i = 0; i < result.size(); ++i)
        result[i] = bank.addhyp(absconjs[i].RPN, absconjs[i].typecode);
    return result;
}

// Size of a substitution
Proofsize Move::substitutionsize(Proofsteps const & src) const
{
    Proofsize size = 0;
    // Make the substitution
    FOR (Proofstep const step, src)
    {
        Symbol2::ID const id = step.id();
        if (id > 0 && !substitutions[id].empty())
            size += substitutions[id].size();
        else
            ++size;
    }
    return size;
}

// Make a substitution.
void Move::makesubstitution(Proofsteps const & src, Proofsteps & dest) const
{
    if (substitutions.empty())
        return dest.assign(src.begin(), src.end());
    // Preallocate for efficiency
    dest.reserve(substitutionsize(src));
    // Make the substitution
    FOR (Proofstep const step, src)
    {
        Symbol2::ID const id = step.id();
        if (id > 0 && !substitutions[id].empty())
            dest += substitutions[id];  // variable with an id
        else
            dest.push_back(step);     // constant with no id
    }
}
