#ifndef SYNTAXIOM_H_INCLUDED
#define SYNTAXIOM_H_INCLUDED

#include "ass.h"

// Set of constants
typedef std::set<strview> Constants;

// A syntax axiom
struct Syntaxiom
{
    Assiter assiter;
    Constants constants;
    operator Assertions::size_type() const { return assiter->second.number; }
};

// Map: name -> syntax axioms
struct Syntaxioms : std::map<strview, Syntaxiom>
{
    Syntaxioms() {}
// Filter assertions for syntax axioms, i.e.,
// those starting with a primitive type code and having no essential hypothesis
// and find their var types.
    Syntaxioms(Assertions const & assertions, class Database const & database);
// Find syntax axioms with a specific key. Return NULL if none is found.
private:
    typedef std::map<strview, std::vector<const_pointer> > M_map_type;
public:
    M_map_type::mapped_type const * keyis(key_type key) const
    {
        M_map_type::const_iterator iter(m_map.find(key));
        return iter == m_map.end() ? NULL : &iter->second;
    }
    Syntaxioms filterbyexp(Expression const & exp) const;
// Return the revPolish notation of exp. Return the empty proof iff not okay.
    Proofsteps RPN
        (Expression const & exp, Assertion const & ass) const;
// Add the revPolish notation and its AST. Return true iff okay.
    bool RPNAST
        (Expression const & exp, Assertion const & ass,
         Proofsteps & rpn, AST & tree) const;
// Add the revPolish notation of the whole assertion. Return true iff okay.
    bool addRPN(Assertion & ass, struct Typecodes const & typecodes) const;
// Add the revPolish notation of a set of assertions. Return true iff okay.
    bool addRPN(Assertions & assertions, struct Typecodes const & typecodes) const;
// Check the syntax of an assertion (& all hypotheses). Return true iff okay.
    bool checkRPN
        (strview label, Assertion ass, struct Typecodes const & typecodes) const;
private:
    M_map_type m_map;
    friend class Database;
// Map syntax axioms.
    void _map();
    void addfromvec
        (Constants const & expconstants, std::vector<const_pointer> const * pv);
};

#endif // SYNTAXIOM_H_INCLUDED
