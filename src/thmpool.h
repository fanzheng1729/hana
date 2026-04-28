#ifndef THMPOOL_H_INCLUDED
#define THMPOOL_H_INCLUDED

#include "types.h"
#include "util/simptree.h"

typedef std::pair<RPN, Assiters> Theorempoolnode;
struct Theorempool : private SimpTree<Theorempoolnode>
{
    //
};

#endif // THMPOOL_H_INCLUDED
