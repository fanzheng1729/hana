#ifndef DISJVARS_H_INCLUDED
#define DISJVARS_H_INCLUDED

#include <algorithm>    // for std::remove_copy_if
#include <iosfwd>
#include "types.h"
#include "util/iter.h"

std::ostream & operator<<(std::ostream & out, Disjvars const & disjvars);

// Print disjoint variable hypothesis violation error.
void printdisjvarserr(Expression const & exp1, Expression const & exp2);

// Restrict disjoint variables hypotheses to a set of variables.
Disjvars operator &(Disjvars const & disjvars, Varusage const & varusage);

// Return true if non-dummy variables in two expressions are disjoint.
bool checkdisjvars
    (Symbol3s const & set1, Symbol3s const & set2,
     Disjvars const & disjvars, Varusage const * varusage,
     bool verbose = true);
template<class It>
bool checkdisjvars
    (It begin1, It end1, It begin2, It end2, Disjvars const & disjvars,
     Varusage const * varusage = NULL)
{
    Symbol3s set1, set2;
    std::remove_copy_if(begin1, end1,
                        end_inserter(set1), util::not1(id));
    std::remove_copy_if(begin2, end2,
                        end_inserter(set2), util::not1(id));
    if (!checkdisjvars(set1, set2, disjvars, varusage))
    {
        printdisjvarserr(Expression(begin1, end1), Expression(begin2, end2));
        return false;
    }
    return true;
}

#endif // DISJVARS_H_INCLUDED
