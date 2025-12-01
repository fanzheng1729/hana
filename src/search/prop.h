#ifndef PROP_H_INCLUDED
#define PROP_H_INCLUDED

#include <new>      // for std::nothrow
#include "environ.h"
#include "../util/find.h"
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
        Propctors::size_type const propcount = propctors.size();
        // Preallocate for efficiency.
        propctorlabels.reserve(propcount);
        propctorfreqs.reserve(propcount);
        // Total frequency counts
        Proofsize total = 0;
        // Initialize propositional syntax axiom labels.
        FOR (Propctors::const_reference propctor, propctors)
        {
            propctorlabels.push_back(propctor.first);
            total += propctor.second.count;
        }
        // Initialize propositional syntax axiom frequencies.
        if (total == 0 && propcount > 0)
            propctorfreqs.assign(propcount, 1./propcount);
        else
            FOR (Propctors::const_reference propctor, propctors)
                propctorfreqs.push_back
                    (static_cast<double>(propctor.second.count)/total);
        // Initialize propositional syntax axiom counts in hypotheses.
        for (Hypsize i = 0; i < ass.hypcount(); ++i)
            if (!ass.hypfloats(i))
                FOR (Proofstep step, ass.hypRPN(i))
                    if (step.type == Proofstep::THM && step.pass)
                        if (const char * label = step.pass->first.c_str)
                        {
                            std::vector<strview>::size_type const i
                            = util::find(propctorlabels, label)
                            - propctorlabels.begin();
                            if (i < hypspropctorcounts.size())
                                ++hypspropctorcounts[i];
                        }
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
        std::vector<Proofsize> propctorcounts = hypspropctorcounts;
        Proofsteps const & RPN = game.goal().RPN;
        FOR (Proofstep step, RPN)
            if (step.type == Proofstep::THM && step.pass)
                if (const char * label = step.pass->first.c_str)
                {
                    std::vector<strview>::size_type const i
                    = util::find(propctorlabels, label) - propctorlabels.begin();
                    if (i < propctorcounts.size())
                        ++propctorcounts[i];
                }
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
    // Propositional syntax axiom frequencies
    std::vector<double> propctorfreqs;
    // Propositional syntax axiom counts in hypotheses
    std::vector<Proofsize> hypspropctorcounts;
};

#endif // PROP_H_INCLUDED
