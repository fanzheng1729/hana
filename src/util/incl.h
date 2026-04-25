#ifndef INCL_H_INCLUDED
#define INCL_H_INCLUDED

#include <functional>
#include <map>
#include <vector>
#include "for.h"

namespace util
{
template<class Key, class T, class Comp = std::less<Key> >
class WeakInc : std::map<Key, std::vector<T>, Comp>
{
    typedef typename mapped_type Minvec;
    Minvec * addkey(Key const & key, Minvec * p)
    {
        Minvec & v = (*this)[key];
        // Find shortest vector.
        if (!p || v.size() < p->size()) p = &v;
        return p;
    }
public:
    template<class C>
    void addkeys(C const & keys, T const & item)
    {
        Minvec * p = NULL;
        FOR (Key const & key, keys)
        {
            p = addkey(key, p);
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
} // namespace util

#endif // INCL_H_INCLUDED
