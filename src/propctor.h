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
    Atom argcount;
};

std::ostream & operator<<(std::ostream & out, Propctor const & propctor);

// Map: propositional syntax constructor label -> data
struct Propctors : std::map<strview, Propctor>
{
// Return true if data for all propositional syntax constructor are okay.
    bool check(Definitions const & definitions) const;
// Print all propositional syntax constructors with truth tables.
    void printtruthtables() const;
// Add a batch of definitions with a given truth table. Return # items added.
    size_type addbatch
    (struct Relations const & batch, bool const truthtable[]);
// Add data for the definitions.
// Records already present are overwritten if there is definition for it.
// Records already present are preserved if there is no definition for it.
// Return # propositional constructors added.
    size_type adddefs(Definitions const & definitions)
    {
        size_type count = 0;
        FOR (Definitions::const_reference def, definitions)
            count += (adddef(definitions, def) != end());
        return count;
    }
// Add a definition. Return the iterator to the entry. Otherwise return end.
    Propctors::const_iterator adddef
        (Definitions const & defs, Definitions::const_reference labeldef);
// Return the truth table of a definition. Return empty Bvector if not okay.
    Bvector truthtable(Definitions const & defs, Definition const & def);
// Evaluate *iter at arg. Return UNKNOWN if not okay.
    int calcbool
        (Definitions const & defs, Definition const & def, TTindex arg);
// Add CNF clauses from reverse Polish notation.
// # of auxiliary atoms start from natom.
// Return true if okay. First auxiliary atom = hyps.size()
    bool addclause
        (Proofsteps const & RPN, Hypiters const & hyps,
         CNFClauses & cnf, Atom & natom) const;
    bool addclause
        (Proofsteps const & RPN, AST const & ast, Hypiters const & hyps,
         CNFClauses & cnf, Atom & natom) const;
// Translate the hypotheses of a propositional assertion to the CNF of an SAT.
    Hypscnf hypscnf(Assertion const & ass, Atom & natom,
                    Bvector const & hypstotrim = Bvector()) const;
// Translate a propositional assertion to the CNF of an SAT instance.
    CNFClauses cnf
        (Assertion const & ass, Proofsteps const & conclusion,
         Bvector const & hypstotrim = Bvector()) const;
// Return true if an expression is valid given a propositional assertion.
    bool checkpropsat(Assertion const & ass,
                      Proofsteps const & conclusion) const;
// Return true if all propositional assertions are sound.
    bool checkpropsat(Assertions const & assertions,
                      struct Typecodes const & typecodes) const;
};

// Return the propositional skeleton of an RPN.
Proofsteps propskeleton
    (Proofsteps const & RPN, AST const & ast, class Bank & bank);

#endif // PROPCTOR_H_INCLUDED
