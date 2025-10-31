#ifndef FOR_H_INCLUDED
#define FOR_H_INCLUDED

namespace util
{
    // Begin iterator of a container
    template<class Container>
    typename Container::iterator begin(Container & c) {return c.begin();}
    template<class Container>
    typename Container::const_iterator begin(Container const & c) {return c.begin();}
    template<class T, std::size_t N>
    T * begin(T (&a)[N]) { return a; }
    // end iterator of a container
    template<class Container>
    typename Container::iterator end(Container & c) {return c.end();}
    template<class Container>
    typename Container::const_iterator end(Container const & c) {return c.end();}
    template<class T, std::size_t N>
    T * end(T (&a)[N]) { return a + N; }
} // namespace util

#ifdef __cpp_range_based_for
#define FOR(x, c) for (x : c)
#else
#include <cstddef>  // for std::size_t
#include <iterator> // for std::iterator_traits

namespace util
{
// Base class for generic iterators
struct _IterBase { operator bool() const { return false; } };
// Wrapper for iterators
template<class It>
struct _Wrap : _IterBase
{
    It _it;
    _Wrap(It const & it) : _it(it) {}
    int operator++() { ++_it; return 0; }
    typename std::iterator_traits<It>::reference operator*() const
    { return *_it; }
};
// Wrapping iterators
template<class It> _Wrap<It> _wrap(It const & it) { return it; }

// Down cast to wrapper
template<class T> struct _Cast {};
template<class Container> struct _Cast<Container &>
{ typedef _Wrap<typename Container::iterator> & type; };
template<class Container> struct _Cast<Container const &>
{ typedef _Wrap<typename Container::const_iterator> & type; };
template<class T, std::size_t N> struct _Cast<T (&)[N]>
{ typedef _Wrap<T *> & type; };
template<class T, std::size_t N> struct _Cast<T const (&)[N]>
{ typedef _Wrap<T const *> & type; };
template<class Container>
typename _Cast<Container &>::type _cast(_IterBase const & it, Container &)
{
    return (typename _Cast<Container &>::type)it;
}

// Return true if two iterator wrappers are equal.
template<class Container>
bool _eq(_IterBase const & it1, _IterBase const & it2, Container & p)
{
    return _cast(it1, p)._it == _cast(it2, p)._it;
}

// Class which sets a break.
struct _Done
{
    bool _done;
    bool & _break;
    _Done(bool & bbreak) : _done(false), _break(bbreak) {}
    void done() { _done = true; }
    operator bool() const { return _done; }
    ~_Done() { _break |= !_done; }
};
} // namespace util

#define FOR(x, c) \
    if (bool _break = false) {} else \
    if (util::_IterBase const & _end = util::_wrap(util::end(c))) {} else \
    for (util::_IterBase const & _base(util::_wrap(util::begin(c))); \
        !_break && !_eq(_base, _end, c); \
        _break ? 0 : ++util::_cast(_base, c)) \
    if (util::_Done _done = _break) {} else \
    for (x = *util::_cast(_base, c); !_done; _done.done())

#endif // __cpp_range_based_for

#endif // FOR_H_INCLUDED
