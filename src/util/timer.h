#ifndef TIMER_H_INCLUDED
#define TIMER_H_INCLUDED

#include <ctime>

typedef double Time;

struct Timer
{
    std::clock_t start;
    void reset() { start = std::clock(); }
    Timer() { reset(); }
    operator Time() const { return resolution() * (std::clock() - start); }
    static Time resolution() { return 1./CLOCKS_PER_SEC; }
};

#endif // TIMER_H_INCLUDED
