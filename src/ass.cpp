#include "ass.h"
#include "disjvars.h"
#include "io.h"
#include "util/filter.h"
#include "util/for.h"

// Remove unnecessary variables.
Bvector & Assertion::trimvars
    (Bvector & hypstotrim, Proofsteps const & conclusion) const
{
// std::cout << conclusion;
    hypstotrim.resize(hypcount());
    hypstotrim.flip();
    for (Hypsize i = 0; i < hypcount(); ++i)
    {
        if (!hypfloats(i)) continue;
        // Use of the variable in hypotheses
        Bvector const & usage = varusage.at(hypexp(i)[1]);
// std::cout << "use of " << hypexp(i)[1] << ' ';
        Hypsize j = hypcount() - 1;
        for ( ; j != static_cast<Hypsize>(-1); --j)
            if (hypstotrim[j] && usage[j]) break;
// std::cout << j << ' ';
        hypstotrim[i] = (j != static_cast<Hypsize>(-1) ||
                        util::filter(conclusion)(hyplabel(i).c_str));
    }
    hypstotrim.flip();
// std::cout << hypstotrim;
    return hypstotrim;
}

// Return the simplified assertion with hypotheses trimmed.
Assertion Assertion::makeAss(Bvector const & hypstotrim) const
{
    Assertion result;
    result.number = number;
    result.sethyps(*this, hypstotrim);
    result.disjvars = disjvars & result.varusage;
    return result;
}

// Set the hypotheses, trimming away specified ones.
void Assertion::sethyps(Assertion const & ass, Bvector const & hypstotrim)
{
    if (hypstotrim.empty())
        return;
    // # hypotheses in new assertion
    Hypsize const newhypcount
    = ass.hypcount() - std::count(hypstotrim.begin(), hypstotrim.end(), true);

    hypiters.clear();
    // Preallocate for efficiency
    hypiters.reserve(newhypcount);
    varusage.clear();
    for (Hypsize i = 0; i < ass.hypcount(); ++i)
    {
        if (i < hypstotrim.size() && hypstotrim[i])
            continue;
        // hypothesis in use
        hypiters.push_back(ass.hypiters[i]);
        if (ass.hypfloats(i))
        {
            // var in use
            Symbol3 const var = ass.hypexp(i)[1];
            // Use of var in new assertion
            Bvector & usage = varusage[var];
            usage.clear();
            // Preallocate for efficiency
            usage.reserve(newhypcount);
            Bvector const & assusage = ass.varusage.at(var);
            for (Hypsize j = 0; j < ass.hypcount(); ++j)
                if (!hypstotrim[j])
                    usage.push_back(assusage[j]);
        }
    }
}

// Set the hypotheses, adding new variables and new hypotheses.
void Assertion::sethyps(Assertion const & ass,
                        Expression const & newvars, Hypiters const & newhypiters)
{
    hypiters.clear();
    // Preallocate for efficiency
    hypiters.reserve(ass.hypcount() + newvars.size() + newhypiters.size());
    varusage.clear();
    // Add floating hypotheses for new variables.
    FOR (Symbol3 const var, newvars)
        if (var.id > 0 && ass.varusage.count(var) == 0)
        {
            std::cout << var.phyp->second.RPN;
        }
}
