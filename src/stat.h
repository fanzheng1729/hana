#ifndef STAT_H_INCLUDED
#define STAT_H_INCLUDED

#include <algorithm>    // for std::max
#include "def.h"
#include "util/for.h"
// #include "io.h"
#include "syntaxiom.h"

// Check if all symbols in a revPolish notation are defined.
// If so, return max # of def/syntax axiom.
// If a symbol has no definition, its # is n. Otherwise return 0.
template<class T>
Assertions::size_type maxsymboldefnumber
    (Proofsteps const & RPN, T const & definitions,
     Syntaxioms const & syntaxioms, Assertions::size_type n)
{
    Assertions::size_type max = 1;
//std::cout << definitions << syntaxioms;
    FOR (Proofstep step, RPN)
    {
//std::cout << step << ':';
        if (step.type != Proofstep::THM || !step.pass)
            continue;
        strview const label = step.pass->first;
        if (!label.c_str)
            continue;

        Assertions::size_type number = 0;
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
        max = std::max(max, number);
    }

    return max;
}

// Check if all symbols in an assertion are defined.
// If so, return max # of def/syntax axiom.
// If a symbol has empty definition, return n. Otherwise return 0.
template<class T>
Assertions::size_type maxsymboldefnumber
    (Assertion const & ass, T const & definitions,
     Syntaxioms const & syntaxioms, Assertions::size_type const n = 0)
{
    Assertions::size_type max = 0;

    max = maxsymboldefnumber(ass.expRPN, definitions, syntaxioms, n);
//std::cout << ass.expression << "has number " << max << std::endl;
    if (max == 0)
        return 0;
    // Check the hypotheses.
    for (Hypsize i = 0; i < ass.hypcount(); ++i)
    {
        Assertions::size_type const maxi =
            maxsymboldefnumber(ass.hypRPN(i), definitions, syntaxioms, n);
        if (maxi == 0)
            return 0;
// std::cout << "hyp " << i << ':' << maxi << '\t';
        max = std::max(max, maxi);
// std::cout << "has number " << max << std::endl;
    }

    return max;
}

// Return max # of syntax axiom in a revPolish notation.
inline Assertions::size_type maxsymboldefnumber
    (Proofsteps const & RPN, Syntaxioms const & syntaxioms)
{
    Assertions::size_type max = 0;

    FOR (Proofstep step, RPN)
    {
//std::cout << step << ':';
        if (step.type != Proofstep::THM)
            continue;

        if (syntaxioms.count(strview(step)))
            max = std::max(max, step.pass->second.number);
    }

    return max;
}

// Return true if an assertion is hard, i.e., uses a new syntax in its proof.
inline bool isasshard(Assertion const & ass, Syntaxioms const & syntaxioms)
{
    // Largest # of syntax axiom in the assertion
    Assertions::size_type const symbolnumber
        (maxsymboldefnumber(ass, Definitions(), syntaxioms));
    if (symbolnumber == 0)
        return false; // Undefined syntax
    // Largest # of syntax axiom in the proof
    Assertions::size_type const proofsymbolnumber
        (maxsymboldefnumber(ass.proof, syntaxioms));
    if (proofsymbolnumber == 0)
        return false; // Undefined syntax
    return proofsymbolnumber > symbolnumber;
}

#endif // STAT_H_INCLUDED
