#include "../disjvars.h"
#include "goaldata.h"
#include "move.h"
#include "../util/filter.h"

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
            (symbols(RPN1),symbols(RPN2),ass.disjvars,ass.varusage,verbose))
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

// Return pointer to proof of subgoal. Return null if out of bound.
Proofsteps const * Move::psubgoalproof(Hypsize index) const
{
    return index >= subgoalcount() ? NULL :
            subgoalfloats(index) ? &substitutions[hypvar(index)] :
            &subgoals[index]->second.proofsrc();
}

// Size of full proof (must be of type CONJ)
Proofsize Move::fullproofsize(pProofs const & phyps) const
{
    if (util::filter(phyps)(static_cast<Proofsteps *>(NULL)))
        return 0;
    Proofsize sum = 0;
    FOR (Proofstep step, *phyps.back())
        switch (step.type)
        {
        case Proofstep::THM:
            ++sum;
            break;
        case Proofstep::HYP:
            if (step.phyp->second.floats)
                if (Proofsize size = substitutions[step.id()].size())
                    sum += size;
                else
                    ++sum;
            else
            {
                Hypsize const index = findabsconj(step.phyp->second);
                if (index >= conjcount())
                    ++sum;
                else
                    sum += phyps[index]->size();
            }
        }
    return sum;
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
