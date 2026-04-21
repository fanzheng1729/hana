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
        allhypsCNF(db.propctors().hypscnf(ass, hypnatoms)),
        propctorlabels(labels(propctors())),
        propctorfreqs(frequencies(propctors())),
        hypspropctorcounts(hypsfreqcounts(ass, propctorlabels)),
        frequencybias(freqbias)
    {
// std::cout << "newEnv " << name << ' ' << ass.varusage;
        // Reinitialize weights of all hypotheses combined.
        hypsweight = 0;
        for (Hypsize i = 0; i < ass.nhyps(); ++i)
            hypsweight += weight(ass.hypRPN(i));
    }
    // Return the propositional syntax constructors.
    Propctors const & propctors() const { return database.propctors(); }
    // Return true if an assertion is on topic/useful.
    virtual bool ontopic(Assertion const & ass) const
    { return ass.testtype(Asstype::PROPOSITIONAL); }
    // CNF of a goal. # of atoms starts from hypnatoms
    CNFClauses goalCNF(Goal const & goal, bool const neg = false) const
    {
        goal.fillast();
        CNFClauses cnf;
        // Add and negate Conclusion.
        Atom n = hypnatoms;
        if (propctors().addformula
            (goal.rpn, goal.ast, assertion.hypiters, cnf, n))
            cnf.closeoff(n - 1, neg);
        else
            cnf.clear();
// std::cout << "goalCNF\n" << cnf;
        return cnf;
    }
    // Determine status of a goal.
    virtual Goalstatus status(Goal const & goal) const
    {
        CNFClauses const & conclusion(goalCNF(goal, true));
        return conclusion.empty() ? printbadgoal(goal.rpn) :
                allhypsCNF.first.sat(conclusion) ? GOALFALSE : GOALTRUE;
    }
    // Return the CNF with some hypotheses trimmed
    CNFClauses hypsCNF(Bvector const & hypstotrim) const
    {
        CNFClauses const & hyps = allhypsCNF.first;
        if (hypstotrim.empty())
            return hyps;
        CNFClauses cnf;
        Proofnumbers const & ends = allhypsCNF.second;
        for (Hypsize j = 0; j < nhyps; ++j)
            if (!assertion.hypfloats(j) && !hypstotrim[j]) // Not floating nor trimmed
// std::cout << "Adding hypothesis " << assertion.hyplabel(j) << std::endl,
                cnf.insert(cnf.end(),
                    &hyps[j ? ends[j - 1] : 0], &hyps[ends[j]]);
// std::cout << "hypcnf\n" << allhypsCNF.first << "cnf\n" << cnf;
        return cnf;
    }
    // Return the hypotheses of a goal to trim.
    virtual Bvector hypstotrim(Goal const & goal) const
    {
        Bvector result(nhyps, false);
        CNFClauses conclusion;
        bool trimmed = false;
        for (Hypsize i = nhyps - 1; i != static_cast<Hypsize>(-1); --i)
        {
            if (assertion.hypfloats(i)) continue;
            // Add conclusion if not already there.
            if (conclusion.empty()) conclusion = goalCNF(goal, true);
// std::cout << "Trimming hypothesis " << assertion.hyplabel(i) << std::endl;
            result[i] = true;
            // If the conclusion still holds, the hypothesis can be trimmed.
            trimmed |= (result[i] = !hypsCNF(result).sat(conclusion));
        }
        return trimmed ? assertion.trimvars(result, goal.rpn) : Bvector();
    }
    // Weight of the goal
    virtual Weight weight(RPN const & goal) const
    {
        return goal.size();
        // return ::weight(goal, propctors());
    }
    // Evaluate leaf games, and record the proof if proven.
    virtual Eval evalourleaf(Game const & game) const;
    // Create a new context constructed from an assertion on the heap.
    // Return its address.
    virtual Prop * makeEnv(Assertion const & ass) const
    {
        return new(std::nothrow)
        Prop(ass, database, m_maxmoves, frequencybias, staged);
    }
private:
    // Add moves with free variables.
    // Return true if it has no open hypotheses.
    virtual bool addhardmoves(Move & move, RPNsize size, Moves & moves) const;
    // The CNF of all hypotheses combined
    HypsCNF const allhypsCNF;
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
