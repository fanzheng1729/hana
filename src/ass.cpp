#include "ass.h"
// #include "io.h"
#include "util/filter.h"

// Remove unnecessary variables.
Bvector & Assertion::trimvars
    (Bvector & hypstotrim, Proofsteps const & conclusion) const
{
// std::cout << conclusion;
    hypstotrim.resize(hypcount());
    hypstotrim.flip();
    for (Hypsize i = 0; i < hypcount(); ++i)
    {
        Hypothesis const & hyp = hypiters[i]->second;
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

        Hypiter const iter = ass.hypiters[i];
        // *iter is used.
        hypiters.push_back(iter);
        if (ass.hypfloats(i))
        {
            // Floating hypothesis of used variable
            Symbol3 const var = iter->second.expression[1];
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
