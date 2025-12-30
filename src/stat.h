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
Assertions::size_type maxsymboldefnumber
    (Proofsteps const & RPN, T const & definitions,
     Syntaxioms const & syntaxioms, Assertions::size_type n)
{
    Assertions::size_type max = 1;
//std::cout << definitions << syntaxioms;
    FOR (Proofstep const step, RPN)
        if (step.isthm() && step.pass)
            if (const char * const label = step.pass->first.c_str)
    {
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
        if (number > max)
            max = number;
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
    for (Hypsize i = 0; i < ass.nhyps(); ++i)
    {
        Assertions::size_type const maxi =
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

// Return max # of syntax axiom in a revPolish notation.
inline Assertions::size_type maxsymboldefnumber
    (Proofsteps const & RPN, Syntaxioms const & syntaxioms)
{
    Assertions::size_type max = 0;

    FOR (Proofstep const step, RPN)
        if (step.isthm() && step.pass)
            if (const char * const label = step.pass->first.c_str)
                if (syntaxioms.count(label))
                    max = std::max(max, step.pass->second.number);

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

typedef std::vector<strview> Labels;
// Find all labels of definitions.
template<class T>
Labels labels(T const & definitions)
{
    Labels dflabel(definitions.size());
    std::transform(definitions.begin(), definitions.end(), dflabel.begin(),
                    util::mem_fn(&T::value_type::first));
    return dflabel;
}

// Add the syntax axioms of a rev-Polish notation to the frequency count.
template<class T>
void addfreqcount(Proofsteps const & RPN, T & definitions)
{
    FOR (Proofstep const step, RPN)
        if (step.isthm() && step.pass)
            if (const char * const label = step.pass->first.c_str)
            {
                typename T::iterator const iter = definitions.find(label);
                if (iter != definitions.end())
                    ++iter->second.freqcount;
            }
}

// Add the syntax axioms of an assertion to the frequency count.
template<class T>
void addfreqcount(Assertion const & ass, T & definitions)
{
    addfreqcount(ass.expRPN, definitions);
    for (Hypsize i = 0; i < ass.nhyps(); ++i)
        if (!ass.hypfloats(i))
            addfreqcount(ass.hypRPN(i), definitions);
}

// Count the syntax axioms in a rev-Polish notation.
inline void addfreqcounts
    (Proofsteps const & RPN, Labels const & labels, Freqcounts & counts)
{
    FOR (Proofstep const step, RPN)
        if (step.isthm() && step.pass)
            if (const char * const label = step.pass->first.c_str)
            {
                Labels::size_type const i
                = util::findsorted(labels, label) - labels.begin();
                if (i < labels.size())
                    ++counts[i];
            }
}

// Count the syntax axioms in all hypotheses of an assertion.
inline Freqcounts hypsfreqcounts(Assertion const & ass, Labels const & labels)
{
    Freqcounts counts(labels.size());
    for (Hypsize i = 0; i < ass.nhyps(); ++i)
        if (!ass.hypfloats(i))
            addfreqcounts(ass.hypRPN(i), labels, counts);
    return counts;
}

typedef double Frequency;
typedef std::valarray<Frequency> Frequencies;
// Find frequencies of definitions.
template<class T>
Frequencies frequencies(T const & definitions)
{
    if (definitions.empty())
        return Frequencies();

    Freqcount total = 0, size = definitions.size();
    FOR (typename T::const_reference rdef, definitions)
        total += rdef.second.freqcount;
    if (total == 0)
        return Frequencies(1./size, size);
    
    Frequencies freqs(size);
    
    size = 0;
    FOR (typename T::const_reference rdef, definitions)
        freqs[size++] = rdef.second.freqcount/static_cast<Frequency>(total);

    return freqs;
}

// Find the weights of a definition
template<class T>
void addweight(T & definitions, typename T::mapped_type & definition)
{
    if (!definition.pdef)
        definition.weight = 1;
    else if (definition.weight == 0)
    {
        Weight & sum = definition.weight;
        FOR (Proofstep const step, definition.rhs)
            if (step.ishyp() && step.phyp)
                ++sum;
            else if (step.isthm() && step.pass)
                if (const char * const label = step.pass->first.c_str)
                {
                    typename T::iterator const iter = definitions.find(label);
                    if (iter == definitions.end())
                        continue;
                    addweight(definitions, iter->second);
                    sum += iter->second.weight;
                }
        sum -= (definition.lhs.size() - 1);
    }
}

// Weight of a rev-Polish notation.
template<class T>
Weight weight(Proofsteps const & RPN, T const & definitions)
{
    Weight sum = 0;

    FOR (Proofstep const step, RPN)
        if (step.ishyp())
            ++sum;
        else if (step.isthm() && step.pass)
            if (const char * const label = step.pass->first.c_str)
            {
                typename T::const_iterator const iter = definitions.find(label);
                if (iter != definitions.end())
                    sum += iter->second.weight;
            }

    return sum;
}

#endif // STAT_H_INCLUDED
