#ifndef ASTPOOL_H_INCLUDED
#define ASTPOOL_H_INCLUDED

#include "../types.h"
#include "../util/simptree.h"

typedef std::vector<RPNstep> RPNheads;

inline RPNheads gotochildren(RPNspanASTs & parents)
{
    RPNheads heads;
    RPNspanASTs children;

    FOR (RPNspanAST parent, parents)
    {
        RPNstep const root = parent.RPNroot();
        if (root.isthm())
            heads.push_back(root);
        for (ASTnode::size_type i = 0; i < parent.nchild(); ++i)
            children.push_back(parent.child(i));
    }

    parents = children;
    return heads;
}

template<class T>
struct ASTPoolNode : std::pair<RPNheads, T>
{
    bool operator==(ASTPoolNode const & other) const
    { return first == other.first; }
    bool operator!=(ASTPoolNode const & other) const
    { return !(*this == other); }
    bool operator< (ASTPoolNode const & other) const
    { return first < other.first; }
};

template<class T>
struct ASTPool : private util::SimpTree<ASTPoolNode<T> >
{
    typedef util::SimpTree<ASTPoolNode<T> > Tree;
    struct pASTNode : Tree::pNode
    {
        pASTNode(typename Tree::pNode p) : Tree:pNode(p) {}
        pASTNode operator[](RPNheads const & heads)
        {
            ASTPoolNode value(heads, T());
            return this->insertordered(value);
        }
        pASTNode at(RPNheads const & heads) const;
    };
    // Root is atomic.
    ASTPool() : Tree(ASTPoolNode<T>()) {}
    void addRPN(RPNspanAST exp, T const & item)
    {
        RPNspanASTs subexps(1, exp);
        while (!subexps.empty())
            RPNheads const & heads(gotochildren(subexps));
    }
};

#endif // ASTPOOL_H_INCLUDED
