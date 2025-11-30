#ifndef SYNTAXDAG_H_INCLUDED
#define SYNTAXDAG_H_INCLUDED

#include <algorithm>    // for std::min
#include <map>
#include <set>
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
    // Map: syntaxiom -> rank
    typedef std::map<strview, strview> Syntaxranks;
    typedef Syntaxranks::const_iterator Mapiter;
    // DAG over map: rank -> min # of syntaxiom of that rank
    typedef DAG<std::map<std::string, std::size_t> > RanksDAG;
    typedef RanksDAG::const_iterator RankDAGiter;
    RanksDAG const & ranksDAG() const { return m_ranksDAG; }
    // Add a syntaxiom and put it in a rank.
    void addsyntax(strview syntaxiom, std::size_t number, strview rank)
    {
        std::pair<RanksDAG::iterator, bool> const result
        = m_ranksDAG.insert(std::make_pair(rank, number));
        if (!result.second) // Another syntaxiom of same rank found
            result.first->second = std::min(number, result.first->second);
        syntaxranks[syntaxiom] = result.first->first;
    }
    // Add the definition of salabel to the DAG of syntaxioms.
    void adddef(strview salabel, Proofsteps const & defRPN)
    {
        FOR (Proofstep step, defRPN)
            if (step.type == Proofstep::THM)
                link(salabel, step.pass->first);
    }
    // Add an expression to the set of maximal ranks.
    void addexp(Ranks & maxranks, Proofsteps const & RPN) const
    { addranks(maxranks, RPNranks(RPN)); }
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
    // Return the ranks of a rev-Polish notation.
    Ranks RPNranks(Proofsteps const & RPN) const
    {
        Ranks result;
        FOR (Proofstep step, RPN)
            if (step.type == Proofstep::THM)
            {
                Mapiter const iter = syntaxranks.find(step.pass->first);
                if (iter != syntaxranks.end())
                    result.insert(iter->second);
            }
        return result;
    }
    // Return true if there is no x in ranks such that rank < x.
    bool ismaximal(strview rank, Ranks const & ranks) const
    {
        // Locate the rank.
        RankDAGiter const to = ranksDAG().find(rank);
        if (to == ranksDAG().end())
            return true; // Rank unseen.
        // Rank seen.
        FOR (strview fromrank, ranks)
        {
            RankDAGiter const from = ranksDAG().find(fromrank);
            if (from != ranksDAG().end() && ranksDAG().reachable(from, to))
                return false;
        }
        return true;
    }
    // Find the rank of a syntaxiom.
    // Return ranksDAG().end() if not found.
    RankDAGiter rankDAGiter(strview label) const
    {
        Mapiter const mapiter = syntaxranks.find(label);
        if (mapiter == syntaxranks.end())
            return ranksDAG().end();
        return ranksDAG().find(mapiter->second);
    }
    // Add an edge between syntaxioms. Returns true if edge is added.
    bool link(strview from, strview to)
    {
        RankDAGiter fromiter = rankDAGiter(from);
        if (fromiter == m_ranksDAG.end())
            return false; // Rank unseen
        RankDAGiter toiter   = rankDAGiter(to);
        if (toiter == m_ranksDAG.end())
            return false; // Rank unseen
        return m_ranksDAG.link(fromiter, toiter);
    }
    // Return true if a node reaches the other.
    bool reachable(strview from, strview to) const
    {
        RankDAGiter fromiter = rankDAGiter(from);
        if (fromiter == m_ranksDAG.end())
            return false; // Rank unseen
        RankDAGiter toiter   = rankDAGiter(to);
        if (toiter == m_ranksDAG.end())
            return false; // Rank unseen
        return ranksDAG().reachable(fromiter, toiter);
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
    RanksDAG m_ranksDAG;
    // Map: syntaxiom -> rank
    Syntaxranks syntaxranks;
};

#endif
