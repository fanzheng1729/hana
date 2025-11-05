#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <algorithm>

namespace util
{
#if __cplusplus >= 201103L
    using std::none_of;
#else
    template<class II, class Pred>
    bool none_of(II first, II last, Pred pred)
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

template<class It, class T>
std::size_t count_not(It begin, It end, T const & value)
{
    std::size_t count = 0;
    while (begin != end) count += (*begin++ != value);
    return count;
}
} // namespace util

#endif // UTIL_H_INCLUDED
