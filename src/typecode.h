#ifndef TYPECODE_H_INCLUDED
#define TYPECODE_H_INCLUDED

#include <map>
#include <utility>
#include "util/strview.h"
#include "util/tribool.h"

// Map: type code -> (as type code, or "" if none, is bound type code) ($4.4.3)
struct Typecodes : std::map<std::string, std::pair<std::string, bool> >
{
    Typecodes() {}
    Typecodes(struct Commands const & syntax, Commands const & bound);
    // Return 1 if a type code is primitive. Return -1 if not found.
    Tribool isprimitive(strview typecode) const
    {
        const_iterator const iter(find(typecode));
        if (iter == end()) return UNKNOWN;
        return iter->second.first.empty() ? TRUE : FALSE;
    }
    // Return 1 if a type code can be bound. Return -1 if not found.
    Tribool isbound(strview typecode) const
    {
        const_iterator const iter(find(typecode));
        if (iter == end()) return UNKNOWN;
        return iter->second.second ? TRUE : FALSE;
    }
    // Return the normalized type code. Return "" if not found.
    strview normalize(strview typecode) const
    {
        const_iterator iter(find(typecode));
        if (iter == end()) return "";
        while (!iter->second.first.empty())
            iter = find(iter->second.first);
        return iter->first;
    }
};

#endif // TYPECODE_H_INCLUDED
