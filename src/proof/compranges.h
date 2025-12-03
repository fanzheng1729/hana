#ifndef COMPRANGES_H_DEFINED
#define COMPRANGES_H_DEFINED

#include "../types.h"
#include "../util/algo.h"   // for util::equal

// Compare ranges of proof steps.
inline bool compranges(Steprange x, Steprange y)
{
    return std::lexicographical_compare
    (x.first, x.second, y.first, y.second);
}
inline bool operator==(Steprange x, Steprange y)
{
    return util::equal(x.first, x.second, y.first, y.second);
}

#endif // COMPRANGES_H_DEFINED
