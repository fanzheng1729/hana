#ifndef PARSE_H_INCLUDED
#define PARSE_H_INCLUDED

#include "io.h"
#include "types.h"

// Stack frame used to find all substitutions in a syntax axiom.
struct Substframe
{
    // Variable substituted
    strview var;
    operator strview() const { return var; }
    // Iterator to beginning of substitutions
    Expiter begin;
    // Map: end of substitution -> its revPolish notation
    typedef std::map<Expiter, Proofsteps> Subexpends;
    Subexpends const & ends;
    // Iterator to current substitution
    Subexpends::const_iterator itersub;
    // Return true if the iterator above is good
    operator bool() const
    { return !unexpected(itersub == ends.end(), "substitution for", var);}
};

// Map: type -> vector[begin index] = possible ends of substitutions.
typedef std::map<strview,
    std::vector<std::pair<Substframe::Subexpends, bool> > > Subexprecords;

// Return possible substitutions from begin to end of the given type.
Substframe::Subexpends const & RPNmap
    (strview type, Expiter expbegin, Expiter expend,
     Expression const & exp, Assertion const & ass,
     struct Syntaxioms const & syntaxioms, Subexprecords & recs);

#endif // PARSE_H_INCLUDED
