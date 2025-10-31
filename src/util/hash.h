#ifndef HASH_H_INCLUDED
#define HASH_H_INCLUDED

#include <string>

namespace util
{
template<unsigned long MUL>
#if __cplusplus >= 201103L
using hash = std::hash<std::string>;
#else
struct hash
{
    unsigned long operator()(std::string const & str) const
    {
        unsigned long result = 0;
        for (std::string::size_type i = 0; i < str.size(); ++i)
            result = result * MUL + str[i];
        return result;
    }
};
#endif
}

#endif // __cplusplus >= 201103L
