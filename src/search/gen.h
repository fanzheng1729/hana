#ifndef GEN_H_INCLUDED
#define GEN_H_INCLUDED

#include "../syntaxiom.h"

// vector of generated terms
typedef std::vector<Proofsteps> Terms;
// map: type -> terms
typedef std::map<strview, Terms> Genresult;
// Counts[type][i] = # of terms up to size i
typedef std::map<strview, std::vector<Terms::size_type> > Termcounts;
// Argtypes[i] = typecode of argument i
typedef std::vector<strview> Argtypes;
// Genstack[i] = # of terms substituted for argument i
typedef std::vector<Terms::size_type> Genstack;

// Virtual base class for adder
struct Adder
{
    virtual void operator()(Argtypes const & types, Genresult const & result,
                            Genstack const & stack) = 0;
};

// Term generator
struct Gen
{
    Gen(Varusage const & varusage, Proofnumber maxcount) :
        m_varusage(varusage), m_maxmoves(maxcount) {}
    Varusage    const m_varusage;
    Proofnumber const m_maxmoves;
    Syntaxioms  syntaxioms;
    Genresult   mutable genresult;
    Termcounts  mutable termcounts;
// Return a lower bound of the number of potential substitutions.
    Proofsize substcount(Argtypes const & argtypes, Genstack const & stack) const;
// Advance the stack and return true if it can be advanced.
// Clear the stack and return false if it cannot be advanced.
    bool  next(Argtypes const & argtypes, Proofsize size, Genstack & stack) const;
// Generate all terms of size 1.
    Terms generateupto1(strview type) const;
// Generate all terms with RPN up to a given size.
// Stop and return false when max count is exceeded.
    bool generateupto(strview type, Proofsize size) const;
// Generate all terms for all arguments with RPN up to a given size.
// Stop and return false when max count is exceeded.
    bool dogenerate(Argtypes const & argtypes, Proofsize size, Adder & adder) const;
};

#endif // GEN_H_INCLUDED
