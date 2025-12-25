#ifndef PROP_H_INCLUDED
#define PROP_H_INCLUDED

#include <algorithm>// for std::accumulate
#include <new>      // for std::nothrow
#include "environ.h"
#include "../util/find.h"
#include "../util/for.h"
// #include "../io.h"
// Propositional proof search, using SAT pruning
struct Prop : Environ
{
    Prop(Assertion const & ass, Database const & db,
         std::size_t maxsize, double freqbias, bool staged = false) :
        Environ(ass, db, maxsize, staged),
        hypscnf(db.propctors().hypscnf(ass, hypnatoms)),
        propctorlabels(labels(database.propctors())),
        propctorfreqs(frequencies(database.propctors())),
        hypspropctorcounts(hypsfreqcounts(ass, propctorlabels)),
        frequencybias(freqbias)
    {
// std::cout << "newEnv " << name << ' ' << ass.varusage;
        // Reinitialize weights of all hypotheses combined.
        hypsweight = 0;
        for (Hypsize i = 0; i < ass.nhyps(); ++i)
            hypsweight += weight(ass.hypRPN(i));
    }
    // Return true if an assertion is on topic/useful.
    virtual bool ontopic(Assertion const & ass) const
    { return ass.type & Asstype::PROPOSITIONAL; }
    // Determine status of a goal.
    virtual Goalstatus status(Goal const & goal) const;
    // Return the hypotheses of a goal to trim.
    virtual Bvector hypstotrim(Goal const & goal) const;
    // Weight of the goal
    virtual Weight weight(Proofsteps const & RPN) const
    {
        return RPN.size();
        // return ::weight(RPN, database.propctors());
    }
    // Evaluate leaf games, and record the proof if proven.
    virtual Eval evalourleaf(Game const & game) const;
    // Allocate a new context constructed from an assertion on the heap.
    // Return its address.
    virtual Prop * makeEnv(Assertion const & ass) const
    {
        return new(std::nothrow)
        Prop(ass, database, m_maxmoves, frequencybias, staged);
    }
private:
    // Add moves with free variables.
    // Return true if it has no open hypotheses.
    virtual bool addhardmoves
        (pAss pthm, Proofsize size, Move & move, Moves & moves) const;
    // The CNF of all hypotheses combined
    Hypscnf const hypscnf;
    Atom hypnatoms;
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
