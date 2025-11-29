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
    typedef Buckets::const_iterator Bucketiter;
    typedef std::map<strview, strview>::const_iterator Mapiter;
    typedef DAG<Buckets> BucketsDAG;
    BucketsDAG const & ranks() const { return m_buckets; }
    // Add a syntaxiom and put it in a bucket.
    void addsyntax(strview syntaxiom, strview bucket)
    { bucketbysyntaxiom[syntaxiom] = *m_buckets.insert(bucket).first; }
    // Add a definition to the DAG of syntaxioms.
    void adddef(strview label, Proofsteps const & defRPN)
    {
        FOR (Proofstep step, defRPN)
            if (step.type == Proofstep::THM)
                link(label, step.pass->first);
    }
    // Add an expression to the set of maximal buckets.
    void addexp(Buckets & maxbuckets, Proofsteps const & RPN) const
    {
        FOR (strview expbucket, expbuckets(RPN))
        {
            if (!ismaximal(expbucket, maxbuckets))
                continue;
            // expbucket is maximal.
            Bucketiter const newiter = maxbuckets.insert(expbucket).first;
            // Remove non-maximal buckets.
            for (Bucketiter iter = maxbuckets.begin();
                iter != maxbuckets.end(); )
                if (iter == newiter || ismaximal(*iter, maxbuckets))
                    ++iter;
                else
                    maxbuckets.erase(iter++);
        }
    }
    // Find the bucket of a syntaxiom.
    // Return ranks().end() if not found.
    Bucketiter bucketiter(strview label) const
    {
        Mapiter const mapiter = bucketbysyntaxiom.find(label);
        if (mapiter == bucketbysyntaxiom.end())
            return ranks().end();
        return ranks().find(mapiter->second);
    }
    // Return the buckets of an expression.
    Buckets expbuckets(Proofsteps const & RPN) const
    {
        Buckets result;
        FOR (Proofstep step, RPN)
            if (step.type == Proofstep::THM)
            {
                Mapiter const iter = bucketbysyntaxiom.find(step.pass->first);
                if (iter != bucketbysyntaxiom.end())
                    result.insert(iter->second);
            }
        return result;
    }
    // Return true if the bucket is maximal among the buckets.
    bool ismaximal(strview bucket, Buckets const & buckets) const
    {
        // Locate the bucket.
        Bucketiter const to = this->ranks().find(bucket);
        if (to == this->ranks().end())
            return true; // Bucket unseen.
        // Bucket seen.
        FOR (strview frombucket, buckets)
        {
            Bucketiter const from = this->ranks().find(frombucket);
            if (from != this->ranks().end() &&
                this->ranks().reachable(from, to))
                return false;
        }
        return true;
    }
    // Add an edge between syntaxioms. Returns true if edge is added.
    bool link(strview from, strview to)
    {
        Bucketiter fromiter = bucketiter(from);
        if (fromiter == m_buckets.end())
            return false; // Bucket unseen
        Bucketiter toiter   = bucketiter(to);
        if (toiter == m_buckets.end())
            return false; // Bucket unseen
        return m_buckets.link(fromiter, toiter);
    }
    // Return true if a node reaches the other.
    bool reachable(strview from, strview to) const
    {
        Bucketiter fromiter = bucketiter(from);
        if (fromiter == m_buckets.end())
            return false; // Bucket unseen
        Bucketiter toiter   = bucketiter(to);
        if (toiter == m_buckets.end())
            return false; // Bucket unseen
        return ranks().reachable(fromiter, toiter);
    }
private:
    // DAG of classes of syntaxioms
    DAG<Buckets> m_buckets;
    // Map: syntaxiom -> bucket
    std::map<strview, strview> bucketbysyntaxiom;
};

#endif
