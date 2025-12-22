#ifndef ALGO_H_INCLUDED
#define ALGO_H_INCLUDED

#include <algorithm>
#include <functional>   // for std::less

namespace util
{
#if __cplusplus >= 201103L
    using std::none_of;
#else
    template<class II, class P>
    bool none_of(II first, II last, P pred)
    {
        return std::find_if(first, last, pred) == last;
    }
#endif // __cplusplus >= 201103L

#ifdef __cpp_lib_robust_nonmodifying_seq_ops
    using std::equal;
    using std::mismatch;
#else
    template<class II1, class II2>
    bool equal(II1 first1, II1 last1, II2 first2, II2 last2)
    {
        return last1 - first1 == last2 - first2 &&
                std::equal(first1, last1, first2);
    }
    template<class II1, class II2>
    std::pair<II1, II2> mismatch(II1 first1, II1 last1, II2 first2, II2 last2)
    {
        last1 = std::min(last1, first1 + (last2 - first2));
        return std::mismatch(first1, last1, first2);
    }
#endif // __cpp_lib_robust_nonmodifying_seq_ops
    template<class II1, class II2>
    int compare(II1 first1, II1 last1, II2 first2, II2 last2)
    {
        std::pair<II1, II2> last = mismatch(first1, last1, first2, last2);
        if (last.first == last1)
            return last.second == last2 ? 0 : -1;
        else if (last.second == last2)
            return 1;
        else if (*last.first > *last.second)
            return 1;
        else if (*last.first == *last.second)
            return 0;
        return -1;
    }

    // Add an item to an ordered container if not already present.
    template <class C, class T>
    void addordered(C & c, T const & x)
    {
        typename C::iterator const iter =
            std::lower_bound(c.begin(), c.end(), x, std::less<T>());
        if (iter == c.end() || *iter != x)
            c.insert(iter, x);
    }

} // namespace util

#endif // ALGO_H_INCLUDED
