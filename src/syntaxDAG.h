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
    typedef DAG<Buckets> BucketsDAG;
    BucketsDAG const & buckets() const { return m_buckets; }
    // Add a syntaxiom and put it in a bucket.
    void addsyntax(strview syntaxiom, strview bucket)
    { bucketbysyntaxiom[syntaxiom] = *m_buckets.insert(bucket).first; }
    // Add a definition to the DAG of syntaxioms.
    void adddef(strview label, Proofsteps const & defRPN)
    {
        FOR (Proofstep step, defRPN)
            if (step.type == Proofstep::THM)
                link(label, step.phyp->first);
    }
    // Add an expression to the set of maximal buckets.
    void addexp(Buckets & buckets, Proofsteps const & expRPN) const
    {
        // Buckets of all syntaxioms in the expression
        Buckets expbuckets;
        FOR (Proofstep step, expRPN)
            if (step.type == Proofstep::THM)
                expbuckets.insert(bucketbysyntaxiom.at(step.phyp->first));
        // Add buckets of all syntaxioms in the expression
        FOR (strview expbucket, expbuckets)
        {
            if (!ismaximal(expbucket, buckets))
                continue;
            // expbucket is maximal.
            Bucketiter const newiter = buckets.insert(expbucket).first;
            // Remove non-maximal buckets.
            for (Bucketiter iter = buckets.begin(); iter != buckets.end(); )
                if (iter == newiter || ismaximal(*iter, buckets))
                    ++iter;
                else
                    buckets.erase(iter++);
        }
    }
    // Return true if the bucket is maximal among the buckets.
    bool ismaximal(strview bucket, Buckets const & buckets) const
    {
        // Locate the bucket.
        Bucketiter const to = this->buckets().find(bucket);
        if (to == this->buckets().end())
            return true; // Bucket unseen.
        // Bucket seen.
        FOR (strview frombucket, buckets)
        {
            Bucketiter const from = this->buckets().find(frombucket);
            if (from != this->buckets().end() &&
                this->buckets().reachable(from, to))
                return false;
        }
        return true;
    }
    // Add an edge between syntaxioms. Returns true if edge is added.
    bool link(strview from, strview to)
    {
        strview frombucket  = bucketbysyntaxiom.at(from);
        Bucketiter fromiter = m_buckets.find(frombucket);
        if (fromiter == m_buckets.end())
            return false; // Bucket unseen
        strview tobucket    = bucketbysyntaxiom.at(to);
        Bucketiter toiter   = m_buckets.find(tobucket);
        if (toiter == m_buckets.end())
            return false; // Bucket unseen
        return m_buckets.link(fromiter, toiter);
    }
    // Return true if a node reaches the other.
    bool reachable(strview from, strview to) const
    {
        strview frombucket  = bucketbysyntaxiom.at(from);
        Bucketiter fromiter = m_buckets.find(frombucket);
        if (fromiter == m_buckets.end())
            return false; // Bucket unseen
        strview tobucket    = bucketbysyntaxiom.at(to);
        Bucketiter toiter   = m_buckets.find(tobucket);
        if (toiter == m_buckets.end())
            return false; // Bucket unseen
        return m_buckets.reachable(fromiter, toiter);
    }
private:
    // DAG of classes of syntaxioms
    DAG<Buckets> m_buckets;
    // Map: syntaxiom -> bucket
    std::map<strview, strview> bucketbysyntaxiom;
};

#endif
