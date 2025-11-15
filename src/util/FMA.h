#ifndef FMA_H_INCLUDED
#define FMA_H_INCLUDED

#include <limits>

template<class T, class U>
static bool FMA(T & num, U mul, int add)
{
    static const int ok[std::numeric_limits<T>::is_signed ||
                        std::numeric_limits<U>::is_signed ? -1 : 1];
    static const T max = -1;
    if (num > max / mul || mul * num > max - add)
        return false;
    num = mul * num + add;
    return true;
}

#endif // FMA_H_INCLUDED
