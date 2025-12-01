#ifndef PROP_H_INCLUDED
#define PROP_H_INCLUDED

#include <algorithm>// for std::accumulate
#include <new>      // for std::nothrow
#include "environ.h"
#include "../util/find.h"
#include "../util/for.h"

// Propositional proof search, using SAT pruning
struct Prop : Environ
{
    Prop(Assertion const & ass, Database const & db,
         std::size_t maxsize, double freqbias, bool staged = false);
    // Return true if an assertion is on topic/useful.
    virtual bool ontopic(Assertion const & ass) const
    { return ass.type & Asstype::PROPOSITIONAL; }
    // Determine status of a goal.
    virtual Goalstatus status(Goal const & goal) const;
    // Return the hypotheses of a goal to trim.
    virtual Bvector hypstotrim(Goal const & goal) const;
    // Weight of the goal
    virtual Weight weight(Proofsteps const & RPN) const;
    // Evaluate leaf games, and record the proof if proven.
    virtual Eval evalourleaf(Game const & game) const;
    // Allocate a new context constructed from an assertion on the heap.
    // Return its address.
    virtual Prop * makeEnv(Assertion const & ass) const
    {
        return new(std::nothrow)
        Prop(ass, database, m_maxmoves, frequencybias, staged);
    }
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
    std::vector<strview> const propctorlabels;
    // Propositional syntax axiom frequencies
    Frequencies const propctorfreqs;
    // Propositional syntax axiom counts in hypotheses
    Freqcounts const hypspropctorcounts;
    // Frequency bias
    double const frequencybias;
};

#endif // PROP_H_INCLUDED
