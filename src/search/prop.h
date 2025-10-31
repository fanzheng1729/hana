#ifndef PROP_H_INCLUDED
#define PROP_H_INCLUDED

#include <new>      // for std::nothrow
#include "../database.h"
#include "environ.h"
#include "../util/for.h"

// Propositional proof search, using SAT pruning
struct Prop : Environ
{
    Prop(Assertion const & ass, Database const & db,
         std::size_t maxsize, bool staged = false) :
        Environ(ass, db, maxsize, staged),
        hypscnf(db.propctors().hypscnf(ass, hypatomcount))
    {
        // Relevant syntax axioms
        FOR (Syntaxioms::const_reference syntaxiom, database.syntaxioms())
            if (syntaxiom.second.assiter->second.number < assertion.number)
                syntaxioms.insert(syntaxiom);
    }
    // Return true if an assertion is on topic/useful.
    virtual bool ontopic(Assertion const & ass) const
    { return ass.type & Asstype::PROPOSITIONAL; }
    // Return true if a goal is valid.
    virtual bool valid(Proofsteps const & goal) const;
    // Return the hypotheses of a goal to trim.
    virtual Bvector hypstotrim(Goalptr goalptr) const;
    // Allocate a new sub environment constructed from a sub assertion on the heap.
    // Return its address.
    virtual Prop * makeenv(Assertion const & ass) const
    { return new(std::nothrow) Prop(ass, database, m_maxcount, staged); }
    // Return the simplified assertion for the goal of the game to hold.
    virtual Assertion makeass(Game const & game) const;
private:
    // Add a move with free variables. Return false.
    virtual bool addhardmoves
        (Assiter iter, Proofsize size, Move & move, Moves & moves) const;
    // The CNF of all hypotheses combined
    Hypscnf const hypscnf;
    Atom hypatomcount;
};

#endif // PROP_H_INCLUDED
