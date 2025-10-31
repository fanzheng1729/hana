#ifndef FILTER_H_INCLUDED
#define FILTER_H_INCLUDED

#include <algorithm>// for std::find and std::set_intersection
#include <cstddef>  // for std::size_t
#include <iterator> // for std::inserter
#include <set>

namespace util
{
// Filter checking if an obj is in a container using @std::find() function
// Container could be std::vector, std::deque, etc.
template<bool B, class Container>
struct Filter
{
    typedef typename Container::value_type argument_type;
    typedef bool result_type;
    Filter(Container const & container) :
        container(container), begin(container.begin()), end(container.end()) {}
    template<class T> bool operator()(T const & value) const
    {
        return (std::find(begin, end, value) != end) == B;
    }
    Container const & container;
    typename Container::const_iterator begin, end;
};

template<bool B, class T, std::size_t N>
struct Filter<B, T[N]>
{
    typedef T argument_type;
    typedef bool result_type;
    Filter(const T(&array)[N]) : container(array), begin(array), end(array + N) {}
    template<class U> bool operator()(U const & value) const
    {
        return (std::find(begin, end, value) != end) == B;
    }
    const T(&container)[N];
    const T * begin, * end;
};

// Filter checking if an obj is in a container using @std::find() function
// Container could be std::vector, std::deque, etc.
template<class Container>
Filter<true, Container> filter(Container const & container)
{
    return Filter<true, Container>(container);
}
} // namespace util

// Find the intersection of two sets.
template<class A, class B>
std::set<A> operator&(std::set<A> const & a, B const & b)
{
    std::set<A> c;
    std::set_intersection(a.begin(), a.end(), b.begin(), b.end(),
                          std::inserter(c, c.end()));
    return c;
}

// Return true if two sets are disjoint.
template<class IIter1, class IIter2>
bool is_disjoint(IIter1 first1, IIter1 last1, IIter2 first2, IIter2 last2)
{
    while (first1 != last1 && first2 != last2)
    {
        if (*first1 == *first2) return false;
        if (*first1 < *first2) ++first1;
        else ++first2;
    }
    return true;
}

#endif // FILTER_H_INCLUDED
