#ifndef INCL_H_INCLUDED
#define INCL_H_INCLUDED

#include <functional>
#include <map>
#include <vector>
#include "for.h"

template<class Key, class T, class Comp = std::less<Key> >
class WeakInc : std::map<Key, std::vector<T>, Comp>
{
    typedef typename mapped_type Minvec;
    Minvec * addkey(Key const & key, Minvec * p)
    {
        Minvec & v = (*this)[key];
        // Update shortest vector and its size.
        if (!p || v.size() < p->size())
        {
            p = &v;
            if (v.empty()) break; // Cannot be shoter.
        }
        return p;
    }
public:
    template<class C>
    void addkeys(C const & keys, T const & item)
    {
        Minvec * p = NULL;
        FOR (Key const & key, keys) p = addkey(key, p);
        if (!p) p = (*this)[Key()];
        p->push_back(item);
    }
    Minvec const * keyis(Key const & key) const
    {
        typename const_iterator iter = this->find(key);
        return iter == this->end() ? NULL : &iter->second;
    }
    template<class C>
    Minvec keyis(C const & keys) const
    {
        Minvec result;
        FOR (Key const & key, keys)
        {
            Minvec const * p = keyis(key);
            if (p)
                result.insert(result.end(), p->begin(), p->end());
        }
        return result;
    }
};

#endif // INCL_H_INCLUDED
