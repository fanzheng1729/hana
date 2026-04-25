#ifndef COMPSPAN_H_DEFINED
#define COMPSPAN_H_DEFINED

#include "../types.h"
#include "../util/algo.h"   // for util::equal

// Compare spans of proof steps.
inline bool compspan(RPNspan x, RPNspan y)
{ return std::lexicographical_compare(x.first, x.second, y.first, y.second); }
inline bool operator==(RPNspan x, RPNspan y)
{ return util::equal(x.first, x.second, y.first, y.second); }
inline bool operator!=(RPNspan x, RPNspan y)
{ return !(x == y); }

#endif // COMPSPAN_H_DEFINED
