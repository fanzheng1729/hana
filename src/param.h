#ifndef PARAM_H_INCLUDED
#define PARAM_H_INCLUDED

#include <cstddef>  // for std::size_t
#include "MCTS/stageval.h"

struct Param
{
    double exploration, freqbias;
    bool staged;
    std::size_t maxsize;
};

#endif // PARAM_H_INCLUDED
