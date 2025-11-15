#ifndef FMA_H_INCLUDED
#define FMA_H_INCLUDED

#include <limits>

template<unsigned MUL, class T>
static bool FMA(T & num, int add)
{
    static const int okay[std::numeric_limits<T>::is_signed ? -1 : 1];
    static const T max = -1;
    if (num > max / MUL || MUL * num > max - add)
        return false;
    num = MUL * num + add;
    return true;
}

#endif // FMA_H_INCLUDED
