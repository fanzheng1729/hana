#ifndef COMPSTEPS_H_DEFINED
#define COMPSTEPS_H_DEFINED

#include "../types.h"
#include "../util/algo.h"   // for util::equal

inline bool compsteps(Steprange x, Steprange y)
{
    return std::lexicographical_compare
    (x.first, x.second, y.first, y.second, std::less<const char *>());
}

#endif
