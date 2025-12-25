#include "ass.h"
#include "disjvars.h"
// #include "io.h"
#include "util/algo.h"  // for util::addordered
#include "util/filter.h"
#include "util/for.h"

// Remove unnecessary variables.
Bvector & Assertion::trimvars
    (Bvector & hypstotrim, Proofsteps const & conclusion) const
{
// std::cout << conclusion;
    hypstotrim.resize(nhyps());
    hypstotrim.flip();
    for (Hypsize i = 0; i < nhyps(); ++i)
    {
        if (!hypfloats(i)) continue;
        // Use of the variable in hypotheses
        Bvector const & usage = varusage.at(hypexp(i)[1]);
// std::cout << "use of " << hypexp(i)[1] << ' ';
        Hypsize j = nhyps() - 1;
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

typedef std::vector<std::string> Labels;

// Sort and concatenate labels.
static std::string sortconcat(Labels & labels)
{
    std::sort(labels.begin(), labels.end());

    std::string::size_type size = 0;
    FOR (std::string const & label, labels)
        size += label.size();
    // Preallocate for efficiency
    std::string result;
    result.reserve(size);
    FOR (std::string const & label, labels)
        result += label;

    return result;
}

// Label with hypotheses trimmed
std::string Assertion::hypslabel(Bvector const & hypstotrim) const
{
    // Preallocate for efficiency.
    Labels labels;
    labels.reserve(nhyps());

    for (Hypsize i = 0; i < hypstotrim.size(); ++i)
        if (!hypstotrim[i])
            labels.push_back(hypdelim + hyplabel(i).c_str);
    for (Hypsize i = hypstotrim.size(); i < nhyps(); ++i)
        labels.push_back(hypdelim + hyplabel(i).c_str);

    return sortconcat(labels);
}

static void sortedcopy(Expression const & src, Expression & dest)
{
    if (dest.size() >= src.size())
        return;
    dest.resize(src.size());
    std::partial_sort_copy(src.begin(), src.end(), dest.begin(), dest.end());
}

// Label with new variables and new hypotheses added
std::string Assertion::hypslabel
    (Expression const & newvars, Hypiters const & newhyps) const
{
    // Copy if there are variables in newhyps but not in newvars.
    Expression copy;
    FOR (Hypiter iter, newhyps)
        FOR (Symbol3 const var, iter->second.expression)
            if (var.id > 0 && varusage.count(var) == 0 &&
                !util::filter(newvars)(var))
            {
                sortedcopy(newvars, copy);
                util::addordered(copy, var);
            }
    // New variables in newvars and newhyps combined
    Expression const & allnewvars = copy.empty() ? newvars : copy;
    // # hypotheses in new assertion
    Hypsize const newnhyps = nhyps() + allnewvars.size() + newhyps.size();
    // Preallocate for efficiency.
    Labels labels;
    labels.reserve(newnhyps);
    // Floating hypotheses for new variables
    FOR (Symbol3 const var, allnewvars)
        if (var.id > 0 && varusage.count(var) == 0)
            labels.push_back(hypdelim + var.iter->first.c_str);
    // Hypothesis, old and new
    FOR (Hypiter iter, hypiters)
        labels.push_back(hypdelim + iter->first.c_str);
    FOR (Hypiter iter, newhyps)
        labels.push_back(hypdelim + iter->first.c_str);

    return sortconcat(labels);
}

// Return the simplified assertion with hypotheses trimmed.
Assertion Assertion::makeAss(Bvector const & hypstotrim) const
{
    Assertion ass(number);
    ass.sethyps(*this, hypstotrim);
    ass.disjvars = disjvars & ass.varusage;
    return ass;
}

// Set the hypotheses, trimming away specified ones.
void Assertion::sethyps(Assertion const & ass, Bvector const & hypstotrim)
{
    if (hypstotrim.empty())
        return;
    // # hypotheses in new assertion
    Hypsize const newnhyps
    = ass.nhyps() - std::count(hypstotrim.begin(), hypstotrim.end(), true);

    hypiters.clear();
    // Preallocate for efficiency
    hypiters.reserve(newnhyps);
    varusage.clear();
    for (Hypsize i = 0; i < ass.nhyps(); ++i)
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
            usage.reserve(newnhyps);
            Bvector const & assusage = ass.varusage.at(var);
            for (Hypsize j = 0; j < ass.nhyps(); ++j)
                if (!hypstotrim[j])
                    usage.push_back(assusage[j]);
        }
    }
}

// Add new variables in an expression.
static void addvarfromexp
    (Varusage & newvars, Expression const & exp, Varusage const & oldvars)
{
    FOR (Symbol3 const var, exp)
        if (var.id > 0 && oldvars.count(var) == 0)
            newvars[var];
}

// Set the hypotheses, adding new variables and new hypotheses.
void Assertion::sethyps(Assertion const & ass,
                        Expression const & newvars, Hypiters const & newhyps)
{
    varusage.clear();
    // std::cout << "newvars " << newvars;
    addvarfromexp(varusage, newvars, ass.varusage);
    FOR (Hypiter iter, newhyps)
        addvarfromexp(varusage, iter->second.expression, ass.varusage);
    // # hypotheses in new assertion
    Hypsize const newnhyps = ass.nhyps() + nvars() + newhyps.size();
    hypiters.clear();
    // Preallocate for efficiency
    hypiters.reserve(newnhyps);
    // Variable usage and floating hypotheses for new variables
    FOR (Varusage::reference rvar, varusage)
    {
        rvar.second.resize(newnhyps);
        hypiters.push_back(rvar.first.iter);
    }
    // Old variable usage
    // std::cout << "oldvars " << ass.varusage;
    FOR (Varusage::const_reference rvar, ass.varusage)
    {
        Bvector & usage = varusage[rvar.first];
        usage.resize(newnhyps);
        Bvector::const_iterator const begin = rvar.second.begin();
        std::copy
        (begin, begin + ass.nhyps(), usage.begin() + hypiters.size());
    }
    // Old hypotheses
    hypiters += ass.hypiters;
    // Use of old variables in new hypotheses
    FOR (Hypiter iter, newhyps)
    {
        FOR (Symbol3 var, iter->second.expression)
            if (var.id > 0)
                varusage[var][hypiters.size()] = true;
        hypiters.push_back(iter);
    }
}
