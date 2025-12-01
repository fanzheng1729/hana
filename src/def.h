#ifndef DEFN_H_INCLUDED
#define DEFN_H_INCLUDED

#include "ass.h"
#include "util/filter.h"

// A definition
struct Definition
{
    // Pointer to the defining assertion, null -> no definition
    pAss pdef;
    // Left and right hand side of definition
    Proofsteps lhs, rhs;
    // # occurrences
    Proofsteps::size_type freq;
    // # of the defining assertion
    operator Assertions::size_type() const { return pdef->second.number; }
    Definition() : pdef(NULL), freq(0) {}
    // Find the revPolish notation of (LHS, RHS).
    Definition(Assertions::const_reference rass);
    Definition
        (Assertions::const_reference rass,
            struct Typecodes const & typecodes,
            struct Relations const & equalities);
    // Return true if the defined syntax appears on the RHS (rule 2).
    bool iscircular() const { return util::filter(rhs)(lhs.back()); }
    // Return true if a variable is dummy, i.e., does not appear on the LHS.
    bool isdummy(Symbol3 var) const
    { return !util::filter(lhs)(Proofstep(var.phyp)); }
    // Check the required disjoint variable hypotheses (rules 3 & 4).
    bool checkdv() const;
    // Return true if all dummy variables are bound (not fully implemented).
    bool checkdummyvar(struct Typecodes const & typecodes) const;
};

// Map: label of syntax axiom -> its definition
struct Definitions : std::map<strview, Definition>
{
    Definitions() {}
    Definitions
        (Assertions const & assertions,
            struct Commentinfo const & commendinfo,
            struct Relations const & equalities);
private:
    // Add a definition. Return true iff okay.
    bool adddef
        (Assertions::const_reference rass,
            struct Typecodes const & typecodes,
            struct Relations const & equalities);
};
// Iterator to definitions
typedef Definitions::const_iterator Defiter;

// Return true if all the definitions are syntactically okay.
bool checkdefinitions
    (Definitions const & definitions, Typecodes const & typecodes);

// Return propositional definitions in assertions.
Definitions propdefinitions
    (Assertions const & assertions,
     struct Commentinfo const & commentinfo,
     struct Relations const & equalities);

#endif // DEFN_H_INCLUDED
