#ifndef PROP_H_INCLUDED
#define PROP_H_INCLUDED

#include <new>      // for std::nothrow
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
        Propctors const & propctors = database.propctors();
        // Preallocate for efficiency.
        propctorlabels.reserve(propctors.size());
        propctorfreqs.reserve(propctors.size());
        // Total frequency counts
        Proofsize total = 0;
        // Initialize propositional syntax axiom labels.
        FOR (Propctors::const_reference propctor, propctors)
        {
            propctorlabels.push_back(propctor.first);
            total += propctor.second.freq;
        }
        // Initialize propositional syntax axiom frequency list.
        if (total == 0)
            propctorfreqs.resize(propctors.size());
        else
            FOR (Propctors::const_reference propctor, propctors)
                propctorfreqs.push_back
                    (static_cast<double>(propctor.second.freq)/total);
    }
    // Return true if an assertion is on topic/useful.
    virtual bool ontopic(Assertion const & ass) const
    { return ass.type & Asstype::PROPOSITIONAL; }
    // Determine status of a goal.
    virtual Goalstatus status(Goal const & goal) const;
    // Return the hypotheses of a goal to trim.
    virtual Bvector hypstotrim(Goal const & goal) const;
    // Evaluate leaf games, and record the proof if proven.
    virtual Eval evalourleaf(Game const & game) const
    {
        Proofsize len = game.env().hypslen + game.goal().size() + game.nDefer;
        return score(len);
    }
    // Allocate a new context constructed from an assertion on the heap.
    // Return its address.
    virtual Prop * makeEnv(Assertion const & ass) const
    { return new(std::nothrow) Prop(ass, database, m_maxmoves, staged); }
    // Return the simplified assertion for the goal of the game to hold.
    virtual Assertion makeAss(Bvector const & hypstotrim) const;
private:
    // Add a move with free variables. Return false.
    virtual bool addhardmoves
        (pAss pthm, Proofsize size, Move & move, Moves & moves) const;
    // The CNF of all hypotheses combined
    Hypscnf const hypscnf;
    Atom hypatomcount;
    // Propositional syntax axiom labels
    std::vector<strview> propctorlabels;
    // Propositional syntax axiom frequencies.
    std::vector<double> propctorfreqs;
};

#endif // PROP_H_INCLUDED
