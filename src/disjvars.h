#ifndef DISJVARS_H_INCLUDED
#define DISJVARS_H_INCLUDED

#include <algorithm>    // for std::remove_copy_if
#include <iosfwd>
#include "types.h"
#include "util/iter.h"

std::ostream & operator<<(std::ostream & out, Disjvars const & disjvars);

// Print disjoint variable hypothesis violation error.
void printDVerr(Expression const & exp1, Expression const & exp2);

// Restrict disjoint variables hypotheses to a set of variables.
Disjvars operator &(Disjvars const & disjvars, Varusage const & varusage);

// Return true if non-dummy variables in two expressions are disjoint.
bool checkDV
    (Symbol3s const & set1, Symbol3s const & set2, Disjvars const & DV,
     Varusage const * varusage, bool verbose = true);
template<class It> bool checkDV
    (std::pair<It, It> exp1, std::pair<It, It> exp2, Disjvars const & DV,
     Varusage const & varusage)
{
    Symbol3s set1, set2;
    std::remove_copy_if(exp1.first, exp1.second,
                        end_inserter(set1), util::not1(id));
    std::remove_copy_if(exp2.first, exp2.second,
                        end_inserter(set2), util::not1(id));
    if (!checkDV(set1, set2, DV, &varusage))
    {
        Expression const expression1(exp1.first, exp1.second);
        Expression const expression2(exp2.first, exp2.second);
        printDVerr(expression1, expression2);
        return false;
    }
    return true;
}

#endif // DISJVARS_H_INCLUDED
