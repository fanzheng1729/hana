#include "../disjvars.h"
#include "goaldata.h"
#include "move.h"
#include "../util/for.h"
#include "../io.h"

static Symbol3s symbols(RPN const & exp)
{
    Symbol3s set;

    FOR (RPNstep step, exp)
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
        RPN const & RPN1 = substitutions[vars.first];
        RPN const & RPN2 = substitutions[vars.second];

        if (!::checkDV
            (symbols(RPN1),symbols(RPN2),ass.disjvars,ass.varusage,verbose))
            return false;
    }

    return true;
}

// Find the disjoint variable hypotheses of a CONJ move.
Disjvars Move::findDV(Assertion const & ass) const
{
    Disjvars result;

    Varusage const & varusage = ass.varusage;

    for (Varusage::const_iterator iter1 = varusage.begin(); 
         iter1 != varusage.end(); ++iter1)
    {
        Symbol3 const var1 = iter1->first;
        RPN const & RPN1 = abstraction(var1);

        Varusage::const_iterator iter2 = iter1;
        for (++iter2; iter2 != varusage.end(); ++iter2)
        {
            Symbol3 const var2 = iter2->first;
            RPN const & RPN2 = abstraction(var2);

            if (::checkDV
                (symbols(RPN1),symbols(RPN2),ass.disjvars,ass.varusage, false))
                result.insert(std::make_pair(var1, var2));
        }
    }

    return result;
}

// Return pointer to proof of subgoal. Return null if out of bound.
RPN const * Move::psubgoalproof(Hypsize index) const
{
    return index >= nsubgoals() ? NULL :
            subgoalfloats(index) ? &substitutions[hypvar(index)] :
            &subgoals[index]->second.proofsrc();
}

// Size of a substitution
RPNsize Move::substsize(RPN const & src) const
{
    RPNsize size = 0;

    // Make the substitution
    FOR (RPNstep const step, src)
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
void Move::makesubst(RPN const & src, RPN & dest) const
{
    if (substitutions.empty())
        return dest.assign(src.begin(), src.end());

    // Preallocate for efficiency
    dest.reserve(substsize(src));

    // Make the substitution
    FOR (RPNstep const step, src)
    {
        Symbol2::ID const id = step.id();
        if (id > 0 && !substitutions[id].empty())
            dest += substitutions[id];  // variable with an id
        else
            dest.push_back(step);     // constant with no id
    }
}

// Size of full proof (must be of type CONJ)
RPNsize Move::fullproofsize(pProofs const & phyps) const
{
    if (phyps.empty() || !phyps.back())
        return 0;

    RPNsize sum = 0;
    FOR (RPNstep step, *phyps.back())
        switch (step.type)
        {
        case RPNstep::THM:
            ++sum;
            break;
        case RPNstep::HYP:
            if (step.phyp->second.floats)
                if (RPNsize size = substitutions[step.id()].size())
                    sum += size;
                else
                    ++sum;
            else
            {
                Hypsize const index = findabsconj(step.phyp->second);
                if (index >= nconjs())
                    ++sum;
                else if (!phyps[index] || phyps[index]->empty())
                    return 0;
                else
                    sum += phyps[index]->size();
            }
        }
    return sum;
}

// Write proof (must be of type CONJ).
bool Move::writeproof(RPN & dest, pProofs const & phyps) const
{
    dest.clear();

    RPNsize const size = fullproofsize(phyps);
    if (size == 0)
        return false;
    // Preallocate for efficiency.
    dest.reserve(size);

    FOR (RPNstep step, *phyps.back())
        switch (step.type)
        {
        case RPNstep::THM:
            dest.push_back(step);
            break;
        case RPNstep::HYP:
            if (step.phyp->second.floats)
            {
                // Floating hypothesis. Check if it refers to an abstract var.
                RPN const & subst = substitutions[step.id()];
                if (!subst.empty())
                    dest += subst; // Abstract variable
                else
                    dest.push_back(step); // Concrete variable
            }
            else
            {
                // Essential hypothesis. Check if it is abstract.
                Hypsize const index = findabsconj(step.phyp->second);
                if (index >= nconjs())
                    dest.push_back(step); // Concrete hypothesis
                else
// printconj(), std::cin.get(),
                    dest += *phyps[index];// Abstract hypothesis
            }
        }
    
    return true;
}

void Move::printconj() const
{
    if (!isconj())
        return;

    static const char arrow[] = "-> ";
    std::cout << "Conjectured ";
    for (Hypsize i = 0; i < nconjs(); ++i)
    {
        std::cout << subgoal(i).expression() << arrow;
        std::cout << absconjs[i].expression();
    }
    std::cout << "in order to prove ";
    std::cout << goal().expression() << arrow;
    std::cout << absconjs.back().expression();
}
