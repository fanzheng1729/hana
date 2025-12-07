#include <algorithm>    // for std::reverse and std::sort
#include <iostream>
#include <numeric>      // for std::partial_sum
#include "ass.h"
#include "getproof.h"
#include "util/filter.h"
#include "util/find.h"
#include "util/for.h"
#include "util/msg.h"
#include "scope.h"

// Determine if there is only one scope. If not then print error.
bool Scopes::isouter(const char * msg) const
{
    if (size() != 1 && msg)
        std::cerr << msg << std::endl;
    return size() == 1;
}

// Return true if nonempty. Otherwise return false and print error.
bool Scopes::pop_back()
{
    if (size() > 1)
    {
        std::vector<Scope>::pop_back();
        return true;
    }
    std::cerr << extraendscope << std::endl;
    return false;
}

// Find active floating hypothesis corresponding to variable.
// Return its iterator or NULL if there isn't one.
Hypiter Scopes::getfloatinghyp(strview var) const
{
    FOR (const_reference scope, *this)
    {
        std::map<strview, Hypiter>::const_iterator const loc
        = scope.floatinghyp.find(var);
        if (loc != scope.floatinghyp.end())
            return loc->second;
    }
    return Hypiter();
}

// Determine if a string is an active variable.
bool Scopes::isactivevariable(strview var) const
{
    FOR (const_reference scope, *this)
        if (scope.activevariables.count(var) > 0)
            return true;
    return false;
}

// Determine if a string is the label of an active hypothesis.
// If so, return the pointer to the hypothesis. Otherwise return NULL.
pHyp Scopes::activehypptr(strview label) const
{
    FOR (const_reference scope, *this)
    {
        Hypiters::const_iterator const
            iterhypvec(util::find(scope.activehyp, label));
        if (iterhypvec != scope.activehyp.end())
            return &**iterhypvec;
    }
    return NULL;
}

// Determine if a floating hypothesis on a string can be added.
// Return 0 if Okay. Otherwise return error code.
int Scopes::erraddfloatinghyp(strview var) const
{
    if (!isactivevariable(var))
        return VARNOTACTIVE;
    return (getfloatinghyp(var) != Hypiter()) * VARDEFINED;
}

// Determine if there is an active disjoint variable restriction on
// two different variables.
bool Scopes::hasdvr(strview var1, strview var2) const
{
    if (var1 == var2)
        return false;

    FOR (const_reference scope, *this)
        FOR (Symbol2s const & vars, scope.disjvars)
            if (vars.count(var1) && vars.count(var2))
                return true;

    return false;
}

// Determine mandatory disjoint variable restrictions.
Disjvars Scopes::disjvars(Symbol2s const & varsused) const
{
    Disjvars result;

    FOR (const_reference scope, *this)
        FOR (Symbol2s const & vars, scope.disjvars)
        {
            Symbol2s dset(vars & varsused);
            for (Symbol2s::iterator diter
                 (dset.begin()); diter != dset.end(); ++diter)
            {
                Symbol2s::iterator diter2(diter);
                for (++diter2; diter2 != dset.end(); ++diter2)
                    result.insert(std::make_pair(*diter, *diter2));
            }
        }

    return result;
}

// Return (# free variables in the hypotheses, total # free variables).
// varusage should be NOT EMPTY.
static Hypsizes nfreevars(Varusage const & varusage)
{
    Hypsizes result(varusage.begin()->second.size(), 0);

    FOR (Varusage::const_reference vardata, varusage)
    {
        if (vardata.second.back())
            continue; // Skip variables in the expression.
        std::transform(result.begin(), result.end(), vardata.second.begin(),
                       result.begin(), std::plus<Hypsize>());
        ++result.back();
    }

    return result;
}

struct Hypcomp
{
    Assertion const & ass;
    bool operator()(Hypsize i, Hypsize j) const
    {
        Hypsizes const & v = ass.nfreevars;
        return v[i] > v[j] || v[i] == v[j] && ass.hyplen(i) < ass.hyplen(j);
    }
};

// Complete an Assertion from its Expression. That is, determine the
// mandatory hypotheses and disjoint variable restrictions and the #.
void Scopes::completeass(struct Assertion & ass) const
{
    Expression const & exp = ass.expression;

    // Determine variables used and find mandatory hypotheses
    Symbol2s vars;
    FOR (Symbol3 const var, exp)
        if (var.id > 0)
        {
            vars.insert(var);
            ass.varusage[var].assign(1, true);
//std::cout << var << " ";
        }

    bool hasfreevar = false;

    for (const_reverse_iterator iter = rbegin(); iter != rend(); ++iter)
    {
        Hypiters const & hypvec = iter->activehyp;
        for (Hypiters::const_reverse_iterator iter2
            = hypvec.rbegin(); iter2 != hypvec.rend(); ++iter2)
        {
            Hypiter const hypiter = *iter2;
//std::cout << "Checking hypothesis " << hypiter->first << std::endl;
            Hypothesis const & hyp = hypiter->second;
            Expression const & hypexp = hyp.expression;
            if (hyp.floats && vars.count(hypexp[1]))
            {
//std::cout << "Mandatory floating Hypothesis: " << hypexp;
                ass.hypiters.push_back(hypiter);
            }
            else if (!hyp.floats)
            {
//std::cout << "Essential hypothesis: " << hypexp;
                ass.hypiters.push_back(hypiter);
                // Add variables used in hypotheses
                FOR (Symbol3 const var, hypexp)
                    if (var.id > 0)
                    {
                        hasfreevar |= vars.insert(var).second;
                        Bvector & usage = ass.varusage[var];
                        usage.resize(ass.hypcount() + 1);
                        usage.back() = true;
//std::cout << " " << var;
                    }
            }
        }
    }
    // Reverse order of hypotheses.
    std::reverse(ass.hypiters.begin(), ass.hypiters.end());
    // Reverse variable appearance vectors.
    FOR (Varusage::reference vardata, ass.varusage)
    {
        vardata.second.resize(ass.hypcount() + 1);
        std::reverse(vardata.second.begin(), vardata.second.end());
    }
    // Find disjoint variable hypotheses
    ass.disjvars = disjvars(vars);
    if (hasfreevar)
    {
        ass.nfreevars = nfreevars(ass.varusage);
        // Sort hypotheses in order of decreasing # free variables
        Hypsizes & hypsorder = ass.hypsorder;
        hypsorder.assign(ass.hypcount(), 1);
        hypsorder[0] = 0;
        std::partial_sum(hypsorder.begin(), hypsorder.end(), hypsorder.begin());
        Hypcomp comp = {ass};
        std::sort(hypsorder.begin(), hypsorder.end(), comp);
    }
}
