#ifndef ASTPOOL_H_INCLUDED
#define ASTPOOL_H_INCLUDED

#include "../types.h"
#include "../util/simptree.h"

typedef std::vector<RPNstep> RPNheads;

template<class T>
struct ASTPool : private util::SimpTree<std::pair<RPNheads, T> >
{
    void addRPN(RPNspanAST exp, T const & item);
};

#endif // ASTPOOL_H_INCLUDED
