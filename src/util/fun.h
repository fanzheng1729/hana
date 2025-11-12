#ifndef FUN_H_INCLUDED
#define FUN_H_INCLUDED

#include <cstddef>      // for NULL
#include <functional>
#include <utility>      // for std::forward

namespace util
{
#if __cplusplus >= 201103L
    using std::mem_fn;
#else
    template<class R, class T>
    class mem_fn_t
    {
        typedef R T::* PM;
        typedef R (T::*PF)();
        PM pm;
        PF pf;
    public:
        mem_fn_t(PM mem) : pm(mem), pf(NULL) {}
        mem_fn_t(PF mem) : pm(NULL), pf(mem) {}
        R operator()(T * p) const { return pm ? p->*pm : (p->*pf)(); }
        R operator()(T const * p) const { return pm ? p->*pm : (p->*pf)(); }
        R operator()(T & t) const { return pm ? t.*pm : (t.*pf)(); }
        R operator()(T const & t) const { return pm ? t.*pm : (t.*pf)(); }
    }; // struct mem_fn_t

    template<class R, class T>
    class const_mem_fn_t
    {
        typedef R (T::*PF)() const;
        PF pf;
    public:
        const_mem_fn_t(PF mem) : pf(mem) {}
        R operator()(T const * p) const { return (p->*pf)(); }
        R operator()(T const & t) const { return (t.*pf)(); }
    }; // struct const_mem_fn_t

    // mem_fn for pointers to a member
    template<class R, class T>
    mem_fn_t<R, T> mem_fn(R T::* mem) {return mem;}
    // mem_fn for pointers to a member function
    template<class R, class T>
    mem_fn_t<R, T> mem_fn(R (T::*mem)()) {return mem;}
    // mem_fn for pointers to a constant member function
    template<class R, class T>
    const_mem_fn_t<R, T> mem_fn(R (T::*mem)() const) {return mem;}
#endif // __cplusplus >= 201103L

#ifdef __cpp_lib_not_fn
    template<class P>
    auto not1(P && pred) { return std::not_fn(std::forward(pred)); }
#else
    template<class P>
    struct not_fn_t
    {
        not_fn_t(P p) : pred(p) {}
        template<class Arg> bool operator()(Arg const & x) { return !pred(x); }
    private:
        P pred;
    }; // struct not_fn_t
    template<class P>
    not_fn_t<P> not1(P pred) { return not_fn_t<P>(pred); }
#endif // __cpp_lib_not_fn

    struct identity { template<class T> T operator()(T x) const { return x; }};
} // namespace util

#endif // FUN_H_INCLUDED
