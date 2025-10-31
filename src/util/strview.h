#ifndef STRVIEW_H_INCLUDED
#define STRVIEW_H_INCLUDED

#include <cstddef>  // for NULL
#include <cstring>
#include <string>

// Lightweight temporary string view, not to outlive viewed string.
struct strview
{
    const char * c_str;
    strview(const char * str = "") : c_str(str) {}
    strview(std::string const & s) : c_str(s.c_str()) {}
    operator std::string() const { return c_str; }
    bool empty() const { return *c_str == 0; }
    const char * remove_prefix(strview prefix) const
    {
        std::size_t const len = std::strlen(prefix.c_str);
        return std::memcmp(c_str, prefix.c_str, len) ? NULL : c_str + len;
    }
};
inline bool operator==(strview x, strview y)
    { return std::strcmp(x.c_str, y.c_str) == 0; }
inline bool operator!=(strview x, strview y)
    { return !(x == y); }
inline bool operator<(strview x, strview y)
    { return std::strcmp(x.c_str, y.c_str) < 0; }

#endif // STRVIEW_H_INCLUDED
