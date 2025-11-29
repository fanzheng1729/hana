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
    typedef DAG<Buckets> BucketsDAG;
    BucketsDAG const & buckets() const { return m_buckets; }
    // Add a syntaxiom and put it in a bucket.
    void addsyntax(strview syntaxiom, strview bucket)
    { bucketbysyntaxiom[syntaxiom] = *m_buckets.insert(bucket).first; }
    // Add a definition.
    void adddef(strview label, Proofsteps const & defRPN)
    {
        FOR (Proofstep step, defRPN)
            if (step.type == Proofstep::THM)
                link(label, step.phyp->first);
    }
    // Add an edge between syntaxioms. Returns true if edge is added.
    bool link(strview from, strview to)
    {
        strview frombucket  = bucketbysyntaxiom.at(from);
        strview tobucket    = bucketbysyntaxiom.at(to);
        Buckets::const_iterator const fromiter  = m_buckets.find(frombucket);
        if (fromiter == m_buckets.end())
            return false;
        Buckets::const_iterator const toiter    = m_buckets.find(tobucket);
        if (toiter == m_buckets.end())
            return false;
        return m_buckets.link(fromiter, toiter);
    }
    // Return true if a node reaches the other.
    bool reachable(strview from, strview to) const
    {
        strview frombucket  = bucketbysyntaxiom.at(from);
        strview tobucket    = bucketbysyntaxiom.at(to);
        Buckets::const_iterator const fromiter  = m_buckets.find(frombucket);
        if (fromiter == m_buckets.end())
            return false;
        Buckets::const_iterator const toiter    = m_buckets.find(tobucket);
        if (toiter == m_buckets.end())
            return false;
        return m_buckets.reachable(fromiter, toiter);
    }
private:
    // DAG of classes of syntaxioms
    DAG<Buckets> m_buckets;
    // Map: syntaxiom -> bucket
    std::map<strview, strview> bucketbysyntaxiom;
};

#endif
