#ifndef THMPOOL_H_INCLUDED
#define THMPOOL_H_INCLUDED

#include "types.h"
#include "util/simptree.h"

// Node in theorem pool = {RPN, iterators to corresponding assertions}
typedef std::pair<RPN, Assiters> Theorempoolnode;
// Node are compared only by RPN.
inline bool operator==(Theorempoolnode const & x, Theorempoolnode const & y)
{ return x.first == y.first; }
inline bool operator< (Theorempoolnode const & x, Theorempoolnode const & y)
{ return x.first < y.first; }

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
};

#endif // THMPOOL_H_INCLUDED
