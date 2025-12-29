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
    Tribool isprimitive(strview typecode) const
    {
        const_iterator const iter = find(typecode);
        if (iter == end()) return UNKNOWN;
        return iter->second.first.empty() ? TRUE : FALSE;
    }
    Tribool isbound(strview typecode) const
    {
        const_iterator const iter = find(typecode);
        if (iter == end()) return UNKNOWN;
        return iter->second.second ? TRUE : FALSE;
    }
    strview normalize(strview typecode) const
    {
        for (const_iterator iter = find(typecode);
             iter != end() && !iter->second.first.empty();
             iter = find(typecode))
            typecode = iter->second.first;
        return typecode;
    }
};

#endif // TYPECODE_H_INCLUDED
