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
    typedef std::set<std::string> Buckets;
    typedef Buckets::const_iterator Rankiter;
    typedef std::map<strview, strview>::const_iterator Mapiter;
    typedef DAG<Buckets> RanksDAG;
    RanksDAG const & ranks() const { return m_ranks; }
    // Add a syntaxiom and put it in a rank.
    void addsyntax(strview syntaxiom, strview rank)
    { syntaxiomranks[syntaxiom] = *m_ranks.insert(rank).first; }
    // Add a definition to the DAG of syntaxioms.
    void adddef(strview label, Proofsteps const & defRPN)
    {
        FOR (Proofstep step, defRPN)
            if (step.type == Proofstep::THM)
                link(label, step.pass->first);
    }
    // Add an expression to the set of maximal ranks.
    void addexp(Buckets & maxranks, Proofsteps const & RPN) const
    {
        FOR (strview exprank, expranks(RPN))
        {
            if (!ismaximal(exprank, maxranks))
                continue;
            // expbucket is maximal.
            Rankiter const newiter = maxranks.insert(exprank).first;
            // Remove non-maximal buckets.
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
    Rankiter bucketiter(strview label) const
    {
        Mapiter const mapiter = syntaxiomranks.find(label);
        if (mapiter == syntaxiomranks.end())
            return ranks().end();
        return ranks().find(mapiter->second);
    }
    // Return the ranks of an expression.
    Buckets expranks(Proofsteps const & RPN) const
    {
        Buckets result;
        FOR (Proofstep step, RPN)
            if (step.type == Proofstep::THM)
            {
                Mapiter const iter = syntaxiomranks.find(step.pass->first);
                if (iter != syntaxiomranks.end())
                    result.insert(iter->second);
            }
        return result;
    }
    // Return true if the bucket is maximal among the buckets.
    bool ismaximal(strview bucket, Buckets const & buckets) const
    {
        // Locate the bucket.
        Rankiter const to = this->ranks().find(bucket);
        if (to == this->ranks().end())
            return true; // Bucket unseen.
        // Bucket seen.
        FOR (strview frombucket, buckets)
        {
            Rankiter const from = this->ranks().find(frombucket);
            if (from != this->ranks().end() &&
                this->ranks().reachable(from, to))
                return false;
        }
        return true;
    }
    // Add an edge between syntaxioms. Returns true if edge is added.
    bool link(strview from, strview to)
    {
        Rankiter fromiter = bucketiter(from);
        if (fromiter == m_ranks.end())
            return false; // Bucket unseen
        Rankiter toiter   = bucketiter(to);
        if (toiter == m_ranks.end())
            return false; // Bucket unseen
        return m_ranks.link(fromiter, toiter);
    }
    // Return true if a node reaches the other.
    bool reachable(strview from, strview to) const
    {
        Rankiter fromiter = bucketiter(from);
        if (fromiter == m_ranks.end())
            return false; // Bucket unseen
        Rankiter toiter   = bucketiter(to);
        if (toiter == m_ranks.end())
            return false; // Bucket unseen
        return ranks().reachable(fromiter, toiter);
    }
private:
    // DAG of classes of syntaxioms
    DAG<Buckets> m_ranks;
    // Map: syntaxiom -> bucket
    std::map<strview, strview> syntaxiomranks;
};

#endif
