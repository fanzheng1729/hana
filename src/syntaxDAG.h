#ifndef SYNTAXDAG_H_INCLUDED
#define SYNTAXDAG_H_INCLUDED

#include <map>
#include "util/DAG.h"
#include "util/for.h"
#include "types.h"

// DAG built from syntaxioms.
// x -> y if definition of x ultimatedly uses y.
struct SyntaxDAG
{
    // Classes of syntaxioms
    typedef std::set<std::string> Ranks;
    typedef Ranks::const_iterator Rankiter;
    typedef std::map<strview, strview>::const_iterator Mapiter;
    typedef DAG<Ranks> RanksDAG;
    RanksDAG const & ranks() const { return m_ranks; }
    // Add a syntaxiom and put it in a rank.
    void addsyntax(strview syntaxiom, strview rank)
    { syntaxiomranks[syntaxiom] = *m_ranks.insert(rank).first; }
    // Add the definition of salabel to the DAG of syntaxioms.
    void adddef(strview salabel, Proofsteps const & defRPN)
    {
        FOR (Proofstep step, defRPN)
            if (step.type == Proofstep::THM)
                link(salabel, step.pass->first);
    }
    // Add an expression to the set of maximal ranks.
    void addexp(Ranks & maxranks, Proofsteps const & RPN) const
    { addranks(maxranks, expranks(RPN)); }
    // Add ranks to the set of maximal ranks.
    void addranks(Ranks & maxranks, Ranks const & newranks) const
    {
        FOR (strview exprank, newranks)
        {
            if (!ismaximal(exprank, maxranks))
                continue;
            // exprank is maximal.
            Rankiter const newiter = maxranks.insert(exprank).first;
            // Remove non-maximal ranks.
            for (Rankiter iter = maxranks.begin();
                iter != maxranks.end(); )
                if (iter == newiter || ismaximal(*iter, maxranks))
                    ++iter;
                else
                    maxranks.erase(iter++);
        }
    }
    // Find the rank of a syntaxiom.
    // Return ranks().end() if not found.
    Rankiter rankiter(strview label) const
    {
        Mapiter const mapiter = syntaxiomranks.find(label);
        if (mapiter == syntaxiomranks.end())
            return ranks().end();
        return ranks().find(mapiter->second);
    }
    // Return the ranks of an expression.
    Ranks expranks(Proofsteps const & RPN) const
    {
        Ranks result;
        FOR (Proofstep step, RPN)
            if (step.type == Proofstep::THM)
            {
                Mapiter const iter = syntaxiomranks.find(step.pass->first);
                if (iter != syntaxiomranks.end())
                    result.insert(iter->second);
            }
        return result;
    }
    // Return true if there is no x in ranks such that rank < x.
    bool ismaximal(strview rank, Ranks const & ranks) const
    {
        // Locate the rank.
        Rankiter const to = this->ranks().find(rank);
        if (to == this->ranks().end())
            return true; // Rank unseen.
        // Rank seen.
        FOR (strview fromrank, ranks)
        {
            Rankiter const from = this->ranks().find(fromrank);
            if (from != this->ranks().end() && this->ranks().reachable(from, to))
                return false;
        }
        return true;
    }
    // Add an edge between syntaxioms. Returns true if edge is added.
    bool link(strview from, strview to)
    {
        Rankiter fromiter = rankiter(from);
        if (fromiter == m_ranks.end())
            return false; // Rank unseen
        Rankiter toiter   = rankiter(to);
        if (toiter == m_ranks.end())
            return false; // Rank unseen
        return m_ranks.link(fromiter, toiter);
    }
    // Return true if a node reaches the other.
    bool reachable(strview from, strview to) const
    {
        Rankiter fromiter = rankiter(from);
        if (fromiter == m_ranks.end())
            return false; // Rank unseen
        Rankiter toiter   = rankiter(to);
        if (toiter == m_ranks.end())
            return false; // Rank unseen
        return ranks().reachable(fromiter, toiter);
    }
    // Ranks A < B if B is non-empty and
    // for all a in A, there is b in B such that a < b.
    bool simplerthan(Ranks const & A, Ranks const & B) const
    {
        if (B.empty())
            return false;
        FOR (strview a, A)
            if (ismaximal(a, B))
                return false;
        return true;
    }
private:
    // DAG of classes of syntaxioms
    DAG<Ranks> m_ranks;
    // Map: syntaxiom -> rank
    std::map<strview, strview> syntaxiomranks;
};

#endif
