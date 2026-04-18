#ifndef PROPCTOR_H_INCLUDED
#define PROPCTOR_H_INCLUDED

#include <iosfwd>
#include "CNF.h"
#include "def.h"
#include "util/for.h"

// Propositional syntax constructor
struct Propctor: Definition
{
    // Truth table of the propositional connective
    Bvector truthtable;
    // CNF clauses reflecting the truth table
    CNFClauses cnf;
    // # arguments of the propositional connective
    Atom nargs;
};

std::ostream & operator<<(std::ostream & out, Propctor const & propctor);

// Map: propositional syntax constructor label -> data
struct Propctors : std::map<strview, Propctor>
{
// Return true if data for all propositional syntax constructor are okay.
    bool check(Definitions const & definitions) const;
// Print all propositional syntax constructors with truth tables.
    void printtruthtables() const;
// Add a batch of definitions with a given truth table.
    void addbatch
        (struct Relations const & batch, bool const truthtable[]);
// Add data for the definitions.
// Records already present are overwritten if there is definition for it.
// Records already present are preserved if there is no definition for it.
    void adddefs(Definitions const & definitions)
    {
        FOR (Definitions::const_reference def, definitions)
            adddef(definitions, def);
    }
// Add a definition. Return the iterator to the entry. Otherwise return end.
    Propctors::const_iterator adddef
        (Definitions const & defs, Definitions::const_reference labeldef);
// Return the truth table of a definition. Return empty Bvector if not okay.
    Bvector truthtable(Definitions const & defs, Definition const & def);
// Evaluate *iter at arg. Return UNKNOWN if not okay.
    int calcbool
        (Definitions const & defs, Definition const & def, TTindex arg);
// Add CNF clauses from reverse Polish notation of a formula.
// Return true if okay. First auxiliary atom = natom
    bool addformula
        (RPN const & rpn, AST const & ast, Hypiters const & hyps,
         CNFClauses & cnf, Atom & natom) const;
// Translate the hypotheses of a propositional assertion to the CNF of an SAT.
    HypsCNF hypscnf(Assertion const & ass, Atom & natom,
                    Bvector const & hypstotrim = Bvector()) const;
// Translate a propositional assertion to the CNF of an SAT instance.
    CNFClauses cnf(Assertion const & ass) const
    {
        // Add hypotheses.
        Atom natom = 0;
        CNFClauses cnf(hypscnf(ass, natom, Bvector()).first);
        // Add and negate conclusion.
        if (addformula(ass.expRPN, ass.expAST, ass.hypiters, cnf, natom))
            cnf.closeoff(natom - 1, true);
        else
            cnf.clear();
        return cnf;
    }
// Return true if a propositional assertion is sound.
    bool checkpropsat(Assertion const & ass) const;
// Return true if all propositional assertions are sound.
    bool checkpropsat(Assertions const & assertions,
                      struct Typecodes const & typecodes) const;
};

// Return the propositional skeleton of an RPN.
RPN propskeleton(RPN const & rpn, AST const & ast, class Bank & bank);

#endif // PROPCTOR_H_INCLUDED
