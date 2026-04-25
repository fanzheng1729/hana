#ifndef INCL_H_INCLUDED
#define INCL_H_INCLUDED

#include <functional>
#include <map>
#include <vector>
#include "for.h"

namespace util
{
template<class Key, class T, class Comp = std::less<Key> >
class WeakIncl : public std::map<Key, std::vector<T>, Comp>
{
    typedef typename mapped_type Minvec;
    Minvec * findminvec(Key const & key, Minvec * p)
    {
        Minvec & v = (*this)[key];
        // Find shortest vector.
        if (!p || v.size() < p->size()) p = &v;
        return p;
    }
public:
    WeakIncl() {}
    WeakIncl(Comp const & comp) :
        std::map<Key, std::vector<T>, Comp>(comp) {}
    template<class C>
    void addkeys(C const & keys, T const & item)
    {
        Minvec * p = NULL;
        FOR (Key const & key, keys)
        {
            p = findminvec(key, p);
            if (p->empty()) break;
        }
        if (!p) p = &(*this)[Key()];
        p->push_back(item);
    }
    Minvec const * keyis(Key const & key) const
    {
        typename const_iterator iter = this->find(key);
        return iter == this->end() ? NULL : &iter->second;
    }
    template<class C>
    Minvec keyin(C const & keys) const
    {
        Minvec result;
        Minvec const * const p0 = keyis(Key());
        if (p0) result = *p0;
        FOR (Key const & key, keys)
        {
            Minvec const * const p = keyis(key);
            if (p)
                result.insert(result.end(), p->begin(), p->end());
        }
        return result;
    }
};

template<class Key, class T, class Comp = std::less<Key> >
class Incl : public std::map<Key, std::vector<T>, Comp>
{
    typedef typename mapped_type Minvec;
    Minvec const * findminvec(Key const & key, Minvec const * p) const
    {
        typename const_iterator const iter = find(key);
        if (iter == end()) return NULL;
        Minvec const & v = iter->second;
        if (!p || v.size() < p->size()) p = &v;
        return p;
    }
public:
    Incl() {}
    Incl(Comp const & comp) :
        std::map<Key, std::vector<T>, Comp>(comp) {}
    template<class C>
    void addkeys(C const & keys, T const & item)
    {
        FOR (Key const & key, keys)
            (*this)[key].push_back(item);
    }
    template<class C>
    Minvec const * hasall(C const & keys) const
    {
        Minvec const * p = NULL;
        FOR (Key const & key, keys)
        {
            p = findminvec(key, p);
            static const Minvec v0;
            if (!p) return &v0;
            if (p->empty()) return p;
        }
        return p;
    }
};
} // namespace util

#endif // INCL_H_INCLUDED
