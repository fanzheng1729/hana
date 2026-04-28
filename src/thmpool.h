#ifndef THMPOOL_H_INCLUDED
#define THMPOOL_H_INCLUDED

#include "types.h"
#include "util/for.h"
#include "io.h"
#include "util/simptree.h"

// Node in theorem pool = {RPN, iterators to corresponding assertions}
typedef std::pair<RPN, Assiters> Theorempoolnode;
// Node are compared only by RPN.
inline bool operator==(Theorempoolnode const & x, Theorempoolnode const & y)
{ return x.first == y.first; }
inline bool operator< (Theorempoolnode const & x, Theorempoolnode const & y)
{ return x.first < y.first; }

typedef std::vector<RPN> RPNs;

std::ostream & operator<<(std::ostream & out, RPNs const & rpns)
{
    FOR (RPN const & rpn, rpns)
    {
        FOR (RPNstep const step, rpn)
            out << (step.empty() ? '0' : step) << ' ';
        out << std::endl;
    }
    return out;
}

struct Theorempool : private SimpTree<Theorempoolnode>
{
    struct pNode : SimpTree::pNode
    {
        pNode(SimpTree::pNode node) : SimpTree::pNode(node) {}
        pNode operator[](RPN const & rpn) const
        {
            Theorempoolnode node(rpn, Assiters());
            return insertordered(node);
        }
        pNode at(RPN const & rpn) const
        {
            Theorempoolnode node(rpn, Assiters());
            return findordered(node);
        }
    };
    pNode operator[](RPNs const & rpns)
    {
        pNode p = root();
        FOR (RPN const & rpn, rpns)
            p = p[rpn];
        return p;
    }
    pNode at(RPNs const & rpns) const
    {
        pNode p = root();
        FOR (RPN const & rpn, rpns)
            p = p.at(rpn);
        return p;
    }
};

#endif // THMPOOL_H_INCLUDED
