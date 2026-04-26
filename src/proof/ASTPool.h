#ifndef ASTPOOL_H_INCLUDED
#define ASTPOOL_H_INCLUDED

#include "../types.h"
#include "../util/simptree.h"

typedef std::vector<RPNstep> RPNheads;

template<class T>
struct ASTNode : std::pair<RPNheads, T>
{
    bool operator==(ASTNode const & other) const
    { return first == other.first; }
    bool operator!=(ASTNode const & other) const
    { return !(*this == other); }
    bool operator< (ASTNode const & other) const
    { return first < other.first; }
};

template<class T>
struct ASTPool : private util::SimpTree<ASTNode<T> >
{
    struct pASTNode : util::SimpTree<ASTNode<T> >::pNode
    {
        pASTNode operator[](RPNheads const & heads);
        pASTNode at(RPNheads const & heads) const;
    };
    void addRPN(RPNspanAST exp, T const & item);
};

#endif // ASTPOOL_H_INCLUDED
