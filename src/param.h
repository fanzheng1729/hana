#ifndef PARAM_H_INCLUDED
#define PARAM_H_INCLUDED

#include <cstddef>  // for std::size_t

struct Param
{
    double exploration, freqbias;
    bool staged;
    std::size_t treesize;
};

#endif // PARAM_H_INCLUDED
