#ifndef STAT_H_INCLUDED
#define STAT_H_INCLUDED

#include <algorithm>    // for std::max
#include <valarray>
#include "def.h"
#include "util/find.h"
#include "util/for.h"
// #include "io.h"
#include "syntaxiom.h"

// Check if all symbols in a revPolish notation are defined.
// If so, return max # of def/syntax axiom.
// If a symbol has no definition, its # is n. Otherwise return 0.
template<class T>
nAss maxsymboldefnumber
    (RPN const & exp, T const & definitions,
     Syntaxioms const & syntaxioms, nAss n)
{
    nAss max = 1;
//std::cout << definitions << syntaxioms;
    FOR (RPNstep const step, exp)
        if (step.isthm() && step.pass)
            if (const char * const label = step.pass->first.c_str)
    {
        nAss number = 0;
        typename T::const_iterator iterdf = definitions.find(label);
//std::cout << "sa";
        if (iterdf != definitions.end())
            number = iterdf->second.pdef ? iterdf->second : n;
        else
        {
//std::cout << "ud";
            Syntaxioms::const_iterator itersyn=syntaxioms.find(strview(step));
            if (itersyn != syntaxioms.end())
                number = itersyn->second; // found in syntax axioms
            else
                return 0; // undefined symbol
        }
//std::cout << number << '\t';
        if (number > max)
            max = number;
    }

    return max;
}

// Check if all symbols in an assertion are defined.
// If so, return max # of def/syntax axiom.
// If a symbol has empty definition, return n. Otherwise return 0.
template<class T>
nAss maxsymboldefnumber
    (Assertion const & ass, T const & definitions,
     Syntaxioms const & syntaxioms, nAss n = 0)
{
    nAss max = 0;

    max = maxsymboldefnumber(ass.expRPN, definitions, syntaxioms, n);
//std::cout << ass.expression << "has number " << max << std::endl;
    if (max == 0)
        return 0;
    // Check the hypotheses.
    for (Hypsize i = 0; i < ass.nhyps(); ++i)
    {
        nAss const maxi =
            maxsymboldefnumber(ass.hypRPN(i), definitions, syntaxioms, n);
        if (maxi == 0)
            return 0;
// std::cout << "hyp " << i << ':' << maxi << '\t';
        if (maxi > max)
            max = maxi;
// std::cout << "has number " << max << std::endl;
    }

    return max;
}

#endif // STAT_H_INCLUDED
