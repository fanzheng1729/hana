#ifndef INFO_H_INCLUDED
#define INFO_H_INCLUDED

#include <map>
#include <string>

// Generic information collection
struct Info : std::map<std::string, void *>
{
    template<class T>
    T * get(std::string const & key) const
    {
        const_iterator iter = find(key);
        if (iter == end()) return NULL;
        return static_cast<T*>(iter->second);
    }
    template<class T>
    void set(std::string const & key, T const & value)
    {
        (*this)[key] = const_cast<void *>(static_cast<const void *>(&value));
    }
};

#endif // INFO_H_INCLUDED
