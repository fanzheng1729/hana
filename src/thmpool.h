#ifndef THMPOOL_H_INCLUDED
#define THMPOOL_H_INCLUDED

#include "types.h"
#include "util/simptree.h"

typedef std::pair<RPN, Assiters> Theorempoolnode;
typedef SimpTree<Theorempoolnode> Theorempool;

#endif // THMPOOL_H_INCLUDED
