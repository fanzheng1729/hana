#ifndef PARAM_H_INCLUDED
#define PARAM_H_INCLUDED

#include <iosfwd>
#include "MCTS/stageval.h"

struct Param
{
    double exploration;
    double freqbias;
    bool staged;
    std::size_t maxsize;
    Param()
        : exploration(1e-3)
        , freqbias(0)
        , staged(false)
        , maxsize(1u << 14)
    {}
    bool read(const char * filename);
    bool save(const char * filename) const;
    template<class T>
    bool fillfield(T Param::* p, std::istream & in)
    { return in >> this->*p, true; }
    void fill(std::istream & in);
    void update(const char * filename);
    bool checkexploration() const { return exploration >= 0; }
    bool checkfreqbias() const { return freqbias >= 0; }
    bool checkstaged() const { return true; }
    bool checkmaxsize() const{ return true; }
    bool good() const
    {
        return
            checkexploration() &&
            checkfreqbias() &&
            checkstaged() &&
            checkmaxsize() &&
        true;
    }
    bool bad() const;
};

std::ostream & operator<<(std::ostream & out, Param const & param);

#endif // PARAM_H_INCLUDED
