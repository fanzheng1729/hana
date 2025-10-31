#ifndef TYPECODE_H_INCLUDED
#define TYPECODE_H_INCLUDED

#include <map>
#include <utility>
#include "util/strview.h"

// Map: type code -> (as type code, or "" if none, is bound type code) ($4.4.3)
struct Typecodes : std::map<std::string, std::pair<std::string, bool> >
{
    Typecodes() {}
    Typecodes(struct Commands const & syntax, Commands const & bound);
    // Return true if a type code is primitive. Return -1 if not found.
    int isprimitive(strview typecode) const
    {
        const_iterator const iter(find(typecode));
        return iter == end() ? -1 : iter->second.first.empty();
    }
    // Return true if a variable is bound. Return -1 if not found.
    int isbound(strview var) const
    {
        const_iterator const iter(find(var));
        return iter == end() ? -1 : iter->second.second;
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
