#ifndef ARITH_H_INCLUDED
#define ARITH_H_INCLUDED

#ifdef __cpp_lib_int_pow2
#include <bit>          // for std::bit_width
#endif // __cpp_lib_int_pow2
// #include <iostream>
#include <algorithm>    // for std::mismatch
#include <limits>       // for is_signed and digits

namespace util
{
// Return n > 0 ? [log2(n)] : 0.
template<class T> static T log2(T n)
{
#ifdef __cpp_lib_int_pow2
    return !!n * (std::bit_width(n) - 1);
#endif // __cpp_lib_int_pow2
    static const unsigned char char_bit = 8;
    static const struct SmallLog2
    {
        unsigned char smalllog2[1U << char_bit];
        SmallLog2()
        {
            smalllog2[0] = smalllog2[1] = 0;
            for (unsigned i = 2; i < sizeof(smalllog2); ++i)
                smalllog2[i] = smalllog2[i / 2] + 1;
        }
    } init;

    T result = 0 * sizeof(char[std::numeric_limits<T>::is_signed ? -1 : 1]);
    // n < 2^digits
    T digits = std::numeric_limits<T>::digits;
    while (digits > char_bit)
    {
        digits = digits / 2 + digits % 2;
        if (T m = n >> digits) n = m, result += digits;
    }
    // while (T m = n >> char_bit) n = m, result += char_bit;
    // std::cout << n << ' ' << static_cast<int>(init.smalllog2[n]) << std::endl;
    return result + init.smalllog2[n];
}

// Return 1 if periodic, 0 if not, -1 if empty
template<class IT, class T>
int isperiodic(IT begin, IT end, T period)
{
    if (end <= begin) return -1;
    if ((end - begin) % period != 0) return 0;
    return std::mismatch(begin, end - period, begin + period).second == end;
}
} // namespace util

#endif // ARITH_H_INCLUDED
