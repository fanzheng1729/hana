#include <algorithm>    // for std::remove_copy_if
#include "io.h"
#include "types.h"
#include "util/filter.h"// for is_disjoint
#include "util/for.h"
#include "util/iter.h"

std::ostream & operator<<(std::ostream & out, Disjvars const & disjvars)
{
    FOR (Disjvars::value_type vars, disjvars)
        out << vars.first << ' ' << vars.second << '\n';
    return out;
}

// Print disjoint variable hypothesis violation error.
void printdisjvarserr(Expression const & exp1, Expression const & exp2)
{
    std::cerr << "Expression\n" << exp1 << "and ";
    std::cerr << "Expression\n" << exp2;
    std::cerr << "violate the disjoint variable hypothesis" << std::endl;
}

// Restrict disjoint variables hypotheses to a set of variables.
Disjvars operator &(Disjvars const & disjvars, Varusage const & varusage)
{
    Disjvars result;
    FOR (Disjvars::value_type vars, disjvars)
        if (varusage.count(vars.first) && varusage.count(vars.second))
            result.insert(vars);

    return result;
}

// Return true if two variables satisfy the disjoint variable hypothesis.
static bool checkdisjvars
    (strview var1, strview var2, Disjvars const & disjvars)
{
    if (var1 == var2) return false;
    if (disjvars.count(std::make_pair(var1, var2)) > 0) return true;
    if (disjvars.count(std::make_pair(var2, var1)) > 0) return true;
    return false;
}

// Return true if two sets satisfy the disjoint variable hypothesis.
static bool checkdisjvars
    (Symbol3s const & set1, Symbol3s const & set2,
     Disjvars const & disjvars, bool verbose = true)
{
    FOR (Symbol3 var1, set1)
        FOR (Symbol3 var2, set2)
            if (!checkdisjvars(var1, var2, disjvars))
            {
                if (verbose)
                {
                    std::cerr << var1 << " and " << var2;
                    std::cerr << " violate the disjoint variable hypothesis\n";
                }
                return false;
            }

    return true;
}

// Compare a symbol and an extended symbol.
static bool operator<(Symbol3 var1, Varusage::const_reference var2)
{
    return var1 < var2.first;
}
static bool operator<(Varusage::const_reference var1, Symbol3 var2)
{
    return var1.first < var2;
}

// Return true if non-dummy variables in two expressions are disjoint.
bool checkdisjvars
    (Symbol3s const & set1, Symbol3s const & set2,
     Disjvars const & disjvars, Varusage const * varusage,
     bool verbose = true)
{
    // Check if two expressions share a common variable.
    if (!is_disjoint(set1.begin(), set1.end(), set2.begin(), set2.end()))
    {
        if (verbose)
            std::cerr << set1 << "and" << set2 << "have a common variable\n";
        return false;
    }

    // Check disjoint variable hypotheses on variables used in the statement.
    return checkdisjvars(varusage ? set1 & *varusage : set1,
                         varusage ? set2 & *varusage : set2,
                         disjvars, verbose);
}
