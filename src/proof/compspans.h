#ifndef COMPSPANS_H_DEFINED
#define COMPSPANS_H_DEFINED

#include "../types.h"
#include "../util/algo.h"   // for util::equal

// Compare spans of proof steps.
inline bool compspans (Steprange x, Steprange y)
{ return std::lexicographical_compare(x.first, x.second, y.first, y.second); }
inline bool operator==(Steprange x, Steprange y)
{ return util::equal(x.first, x.second, y.first, y.second); }
inline bool operator!=(Steprange x, Steprange y)
{ return !(x ==y); }

#endif // COMPSPANS_H_DEFINED
