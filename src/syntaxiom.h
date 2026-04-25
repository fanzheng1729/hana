#ifndef SYNTAXIOM_H_INCLUDED
#define SYNTAXIOM_H_INCLUDED

#include "ass.h"
#include "util/incl.h"

// Set of constants
typedef std::set<strview> Constants;

// A syntax axiom
struct Syntaxiom
{
    pAss pass;
    Constants constants;
    operator Assertions::size_type() const { return pass->second.number; }
};

// Map: label -> syntax axioms
struct Syntaxioms : std::map<strview, Syntaxiom>
{
    Syntaxioms() {}
// Filter assertions for syntax axioms, i.e.,
// those starting with a primitive type code and having no essential hypothesis
// and find their var types.
    Syntaxioms(Assertions const & assertions, class Database const & database);
private:
    util::WeakInc<strview, const_pointer> m_map;
public:
    Syntaxioms filterbyexp(Expression const & exp) const;
// Return the revPolish notation of exp. Return the empty proof iff not okay.
    RPN parse(Expression const & exp, Assertion const & ass) const;
// Add the revPolish notation and its AST. Return true if okay.
    bool RPNAST
        (Expression const & exp, Assertion const & ass,
         RPN & rpn, AST & tree) const;
// Add the revPolish notation of the whole assertion. Return true if okay.
    bool addRPN(Assertion & ass, struct Typecodes const & typecodes) const;
// Add the revPolish notation of a set of assertions. Return true if okay.
    bool addRPN(Assertions & assertions, struct Typecodes const & typecodes) const;
// Check the syntax of an assertion (& all hypotheses). Return true if okay.
    bool checkRPN
        (Assertion ass, struct Typecodes const & typecodes) const;
private:
    friend class Database;
    void addfromvec
        (Constants const & expconstants, std::vector<const_pointer> const * pv);
};

#endif // SYNTAXIOM_H_INCLUDED
