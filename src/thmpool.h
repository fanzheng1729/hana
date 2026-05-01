#ifndef THMPOOL_H_INCLUDED
#define THMPOOL_H_INCLUDED

#include <algorithm>    // for std::max_element and std::sort
#include "types.h"
#include "util/for.h"
// #include "io.h"
#include "util/simptree.h"

// Node in theorem pool = {RPN, iterators to corresponding assertions}
typedef std::pair<RPN, Assiters> Theorempoolnode;
// Node are compared only by RPN.
inline bool operator==(Theorempoolnode const & x, Theorempoolnode const & y)
{ return x.first == y.first; }
inline bool operator< (Theorempoolnode const & x, Theorempoolnode const & y)
{ return x.first < y.first; }

typedef std::vector<RPN> Profile;
typedef std::vector<Profile> Profiles;

inline Profile profile(Profiles const & profiles, RPNstep root)
{
// std::cout << "profiles " << root << std::endl;
    if (profiles.empty())
        return Profile(1, RPN(1, root));

    Profile result;

    std::vector<Profile::size_type> sizes(profiles.size());
    for (Profiles::size_type i = 0; i < profiles.size(); ++i)
        sizes[i] = profiles[i].size();
    Profile::size_type const maxsize = *std::max_element(sizes.begin(), sizes.end());
    result.resize(maxsize + 1);
    result[0].assign(1, root);
    for (Profile::size_type level = 1; level <= maxsize; ++level)
    {
        RPN & rpn = result[level];
        for (Profiles::size_type i = 0; i < profiles.size(); ++i)
            if (level - 1 < sizes[i])
                rpn += profiles[i][level - 1];
    }
// std::cout << "profile is " << result;
    return result;
}

inline Profile profile(RPNspanAST exp)
{
// std::cout << "profile " << exp.first;
    RPNstep const root = exp.RPNroot();
    if (root.id()) // variable
        return Profile(1, RPN(1));

    ASTnode::size_type const n = exp.nchild();
    Profiles profiles(n);
    for (ASTnode::size_type i = 0; i < n; ++i)
        profiles[i] = profile(exp.child(i));
    return profile(profiles, root);
}

// i-th bit of mask set if i-th sub-expression is a variable.
inline RPNsize varmask(RPNspanASTs const & subexps)
{
    RPNsize mask = 0;

    for (RPNsize i = 0; i < subexps.size(); ++i)
        if (subexps[i].RPNroot().id())
            mask += static_cast<RPNsize>(1) << i;

    return mask;
}

// RPN for child
inline RPN childRPN(RPNspanASTs const & subexps, RPNsize mask)
{
    RPN result(subexps.size());

    for (RPNsize i = 0; i < subexps.size(); ++i)
        if (mask & (static_cast<RPNsize>(1) << i))
            result[i] = subexps[i].RPNroot();
        else
            result[i] = RPNstep();

    return result;
}

inline RPNspanASTs childsubexps(RPNspanASTs const & subexps, RPNsize mask)
{
    RPNspanASTs result;

    for (RPNsize i = 0; i < subexps.size(); ++i)
    {
        if (!(mask & static_cast<RPNsize>(1) << i))
            continue;
        for (ASTnode::size_type j = 0; j < subexps[i].nchild(); ++j)
            result.push_back(subexps[i].child(j));
    }

    return result;
}

inline void addassiters(Assiters & dest, Assiters const & src, nAss limit = -1)
{
    dest.reserve(dest.size() + src.size());
    FOR (Assiter const & iter, src)
        if (iter->second.number < limit)
            dest.push_back(iter);
}

struct Theorempool : private SimpTree<Theorempoolnode>
{
    Theorempool() : SimpTree(Theorempoolnode()) {}
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
        Assiters matches(RPNspanASTs const & subexps, nAss limit = -1) const
        {
            if (subexps.empty())
                return Assiters();

            Assiters result;

            typedef RPNspanASTs::size_type Stack, Stackindex;
            Stackindex const n = subexps.size(); // > 0
            // mask for variables
            Stack const mask = varmask(subexps);
            // Iterate through all possible children.
            Stack const max = static_cast<Stack>(1) << n;
            for (Stack stack = 0; stack < max; ++stack)
            {
                if (stack & mask) continue;
                RPN const & rpn(childRPN(subexps, stack));
                // Find child.
                pNode const child = at(rpn);
                if (!child) continue;
                // assertions at child
                addassiters(result, child->second, limit);
                // sub-expression for child
                addassiters(result,
                            child.matches(childsubexps(subexps, stack), limit));
            }

            return result;
        }
    };
    pNode root() const
    { return static_cast<SimpTree const *>(this)->root(); }
    pNode operator[](Profile const & rpns)
    {
        pNode p = root();
        FOR (RPN const & rpn, rpns) p = p[rpn];
        return p;
    }
    pNode at(Profile const & rpns) const
    {
        pNode p = root();
        FOR (RPN const & rpn, rpns) p = p.at(rpn);
        return p;
    }
    friend bool operator<(Assiter x, Assiter y)
    { return x->second.number < y->second.number; }
    Assiters matches(RPNspanAST exp, nAss limit = -1) const
    {
        Assiters result(root().matches(RPNspanASTs(1, exp), limit));
        std::sort(result.begin(), result.end());
        return result;
    }
};

// Map: typecode -> theorems
typedef std::map<strview, Assiters> Thmpool;
typedef std::map<strview, Theorempool> Theorempools;

#endif // THMPOOL_H_INCLUDED
