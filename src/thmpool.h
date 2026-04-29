#ifndef THMPOOL_H_INCLUDED
#define THMPOOL_H_INCLUDED

#include <algorithm>    // for std::max_element
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
            out << (step.empty() ? "0" : step) << ' ';
        out << std::endl;
    }
    return out;
}

RPNs profile(std::vector<RPNs> profiles, RPNstep root)
{
// std::cout << "profiles " << root << std::endl;
    if (profiles.empty())
        return RPNs(1, RPN(1, root));

    RPNs result;

    std::vector<RPNs::size_type> sizes(profiles.size());
    for (std::vector<RPNs>::size_type i = 0; i < profiles.size(); ++i)
        sizes[i] = profiles[i].size();
    RPNs::size_type const maxsize = *std::max_element(sizes.begin(), sizes.end());
    result.resize(maxsize + 1);
    result[0].assign(1, root);
    for (RPNs::size_type level = 1; level <= maxsize; ++level)
    {
        RPN & rpn = result[level];
        for (std::vector<RPNs>::size_type i = 0; i < profiles.size(); ++i)
            if (level - 1 < sizes[i])
                rpn += profiles[i][level - 1];
    }
// std::cout << "profile is " << result;
    return result;
}

RPNs profile(RPNspanAST exp)
{
// std::cout << "profile " << exp.first;
    RPNs result;
    RPNstep const root = exp.RPNroot();
    if (root.id()) // variable
        return RPNs(1, RPN(1));

    ASTnode::size_type const n = exp.nchild();
    std::vector<RPNs> profiles(n);
    for (ASTnode::size_type i = 0; i < n; ++i)
        profiles[i] = profile(exp.child(i));
    return profile(profiles, root);
}

struct Theorempool : private SimpTree<Theorempoolnode>
{
    Theorempool() : SimpTree(Theorempoolnode(RPN(1), Assiters())) {}
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
    Assiters matches(RPNspanAST exp) const;
};

#endif // THMPOOL_H_INCLUDED
