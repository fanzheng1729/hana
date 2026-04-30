#ifndef INFO_H_INCLUDED
#define INFO_H_INCLUDED

#include <map>
#include <string>
#include <utility>
#include "util/for.h"

// Generic information collection
class Info : std::map<std::string, std::pair<void *, bool> >
{
public:
    template<class T>
    T * get(std::string const & key) const
    {
        const_iterator iter = find(key);
        if (iter == end()) return NULL;
        return static_cast<T*>(iter->second->first);
    }
#if __cpluscplus >= 201103L
    // Non-owning
    template<class T>
    void set(std::string const & key, T const & value)
    {
        void * p = const_cast<void *>(static_cast<const void *>(&value));
        (*this)[key] = std::make_pair(p, false);
    }
    // Owning
    template<class T>
    void set(std::string const & key, T && value)
    {
        void * p = new T(std::move(value));
        (*this)[key] = std::make_pair(p, true);
        // (*this)[key] = const_cast<void *>(static_cast<const void *>(&value));
    }
#else
    // Owning
    template<class T>
    void set(std::string const & key, T const & value)
    {
        void * p = new T(value);
        (*this)[key] = std::make_pair(p, true);
        // (*this)[key] = const_cast<void *>(static_cast<const void *>(&value));
    }
#endif // __cpluscplus >= 201103L
    ~Info()
    {
        typedef std::map<std::string, std::pair<void *, bool> > Base;
        FOR (Base::const_reference item, *this)
            if (item.second.second)
                delete item.second.first;
    }
};

#endif // INFO_H_INCLUDED
