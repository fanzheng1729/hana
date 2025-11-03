#ifndef SCOPE_H_INCLUDED
#define SCOPE_H_INCLUDED

#include "types.h"

// Error codes used when parsing a floating hypothesis.
enum {TYPENOTCONST = 1, VARNOTACTIVE, VARDEFINED, EXTRATOKEN};

// See $4.2.8.
struct Scope
{
    // Active variables
    std::set<strview> activevariables;
    // Iterators to active hypotheses
    Hypiters activehyp;
    // Disjoint variables restrictions
    std::vector<Symbol2s> disjvars;
    // Map: variable -> label of active floating hypothesis
    std::map<strview, Hypiter> floatinghyp;
};

struct Scopes : std::vector<Scope>
{
    // Determine if there is only one scope. If not then print error.
    bool isouter(const char * msg = NULL) const;
    // Return true if nonempty. Otherwise return false and print error.
    bool pop_back();
    // Find active floating hypothesis corresponding to variable.
    // Return its pointer or NULL if there isn't one.
    Hypptr getfloatinghyp(strview var) const;
    // Determine if a string is an active variable.
    bool isactivevariable(strview var) const;
    // Determine if a string is the label of an active hypothesis.
    // If so, return the pointer to the hypothesis. Otherwise return NULL.
    Hypptr activehypptr(strview label) const;
    // Determine if a floating hypothesis on a string can be added.
    // Return 0 if Okay. Otherwise return error code.
    int erraddfloatinghyp(strview var) const;
    // Determine if there is an active disjoint variable restriction on
    // two different variables.
    bool hasdvr(strview var1, strview var2) const;
    // Determine mandatory disjoint variable restrictions.
    Disjvars disjvars(Symbol2s const & varusage) const;
    // Complete an Assertion from its Expression. That is, determine the
    // mandatory hypotheses and disjoint variable restrictions and the #.
    void completeass(struct Assertion & ass) const;
};

#endif // SCOPE_H_INCLUDED
