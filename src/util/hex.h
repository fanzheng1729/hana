#ifndef HEX_H_INCLUDED
#define HEX_H_INCLUDED

#include <algorithm>    // for std::reverse
#include <limits>       // for is_signed and digits

namespace util
{
// Return hex string of n, in the form of 0x########\0.
// Result must be assigned to std::string, to avoid dangling pointer.
template<class T> const char * hex(T n)
{
    static bool const okay = !std::numeric_limits<T>::is_signed;
    static int  const padding = sizeof(T[okay ? 3 : -1])/sizeof(T);
    static int  const size = std::numeric_limits<T>::digits/4
                            + (std::numeric_limits<T>::digits%4 > 0)
                            + padding;
    #if __cplusplus >= 201103L
    thread_local
    #else
    static
    #endif // __cplusplus >= 201103L
    // buffer
    char s[size];
    // Clear buffer.
    std::fill(s, s + size, 0);
    // Fill digits ########.
    char * p = s;
    for ( ; n > 0; n /= 16) *p++ = "0123456789ABCDEF"[n % 16];
    if (p == s) *p++ = '0';
    // Add "x0".
    *p++ = 'x', *p++ = '0';
    // Reverse the string.
    std::reverse(s, p);

    return s;
}
} // namespace util

#endif // HEX_H_INCLUDED
