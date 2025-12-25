#include <cstddef>  // for std::size_t
#include <iostream>
#include "util/strview.h"

std::ostream & operator<<(std::ostream & out, strview str)
{
    return out << str.c_str;
}

// Return true if n <= lim. Otherwise print a message and return false.
bool is1stle2nd(std::size_t const n, std::size_t const lim,
                const char * const s1, const char * const s2)
{
    if (n <= lim)
        return true;
    std::cout << n << s1 << lim << s2 << std::endl;
    return false;
}

// Print the number and the label of an assertion.
void printass(std::size_t number, strview label)
{
    std::cout << "\t#" << number << '\t' << label << '\t';
}

