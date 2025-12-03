#ifndef COMPRANGES_H_DEFINED
#define COMPRANGES_H_DEFINED

#include <algorithm>    // for std::lexicographical_compare
#include "../types.h"

// Compare steps by content.
inline bool compranges(Steprange x, Steprange y)
{
    return std::lexicographical_compare
    (x.first, x.second, y.first, y.second, std::less<const char *>());
}

#endif // COMPRANGES_H_DEFINED
