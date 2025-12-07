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

    FOR (Disjvars::const_reference vars, theorem().disjvars)
    {
        Proofsteps const & RPN1 = substitutions[vars.first];
        Proofsteps const & RPN2 = substitutions[vars.second];

        if (!::checkDV
            (symbols(RPN1), symbols(RPN2), ass.disjvars, ass.varusage, verbose))
            return false;
    }

    return true;
}

// Return the disjoint variable hypotheses of a move.
Disjvars Move::findDV(Assertion const & ass) const
{
    Disjvars result;

    Varusage const & varusage = ass.varusage;

    for (Varusage::const_iterator iter1 = varusage.begin(); 
         iter1 != varusage.end(); ++iter1)
    {
        Symbol3 const var1 = iter1->first;
        for (Varusage::const_iterator iter2 = (++iter1, iter1--);
             iter2 != varusage.end(); ++iter2)
        {
            Symbol3 const var2 = iter2->first;

            Proofsteps const & subst1 = substitutions[var1];
            Proofsteps const & subst2 = substitutions[var2];

            Proofsteps const & RPN1
            = subst1.empty() ? var1.iter->second.RPN : subst1;
            Proofsteps const & RPN2
            = subst2.empty() ? var2.iter->second.RPN : subst2;

        if (::checkDV
            (symbols(RPN1), symbols(RPN2), ass.disjvars, ass.varusage, false))
            result.insert(std::make_pair(var1, var2));
        }
    }

    return result;
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
