#ifndef SYNTAXDAG_H_INCLUDED
#define SYNTAXDAG_H_INCLUDED

#include <map>
#include <set>
#include "util/strview.h"
#include "util/DAG.h"

// DAG built from syntaxioms.
// x -> y if definition of x ultimatedly uses y.
class SyntaxDAG
{
    // Classes of syntaxioms
    typedef std::set<std::string> Buckets;
    // DAG of classes of syntaxioms
    DAG<Buckets> buckets;
    // Map: syntaxiom -> bucket
    std::map<strview, strview> bucketbysyntaxiom;
public:
    // Add a syntaxiom and put it in a bucket.
    void addsyntax(strview syntaxiom, strview bucket)
    { bucketbysyntaxiom[syntaxiom] = *buckets.insert(bucket).first; }
    // Add an edge. Returns true if edge is added.
    bool link(strview from, strview to)
    {
        strview frombucket  = bucketbysyntaxiom.at(from);
        strview tobucket    = bucketbysyntaxiom.at(to);
        Buckets::const_iterator const fromiter  = buckets.find(frombucket);
        if (fromiter == buckets.end())  return false;
        Buckets::const_iterator const toiter    = buckets.find(tobucket);
        if (toiter == buckets.end())    return false;
        return buckets.link(fromiter, toiter);
    }
    // Return true if a node reaches the other.
    bool reachable(strview from, strview to) const
    {
        strview frombucket  = bucketbysyntaxiom.at(from);
        strview tobucket    = bucketbysyntaxiom.at(to);
        Buckets::const_iterator const fromiter  = buckets.find(frombucket);
        if (fromiter == buckets.end())  return false;
        Buckets::const_iterator const toiter    = buckets.find(tobucket);
        if (toiter == buckets.end())    return false;
        return buckets.reachable(fromiter, toiter);
    }
};

#endif
