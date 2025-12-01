#ifndef FIND_H_INCLUDED
#define FIND_H_INCLUDED

#include <algorithm>

namespace util
{
    template<class C, class T>
    typename C::const_iterator find(C const & c, T const & val)
    {
        return std::find(c.begin(), c.end(), val);
    }
    template<class T, std::size_t N, class V>
    T const * find(T const (&c)[N], V const & val)
    {
        return std::find(c, c + N, val);
    }
    template<class C, class T>
    typename C::const_iterator findsorted(C const & c, T const & val)
    {
        typename C::const_iterator const iter
        = std::lower_bound(c.begin(), c.end(), val);
        return *iter == val ? iter : c.end();
    }
    template<class T, std::size_t N, class V>
    T const * findsorted(T const (&c)[N], V const & val)
    {
        T const * const p = std::lower_bound(c, c + N, val);
        return *p == val ? p : c + N;
    }
} // namespace util

#endif // FIND_H_INCLUDED
