#ifndef PARAM_H_INCLUDED
#define PARAM_H_INCLUDED

#include <iosfwd>
#include "MCTS/stageval.h"

struct Param
{
    double exploration;
    double weightfactor;
    bool staged;
    std::size_t maxsize;
    static Param const default;
private:
    bool read(const char * filename);
    bool save(const char * filename) const;
    template<class T>
    bool fillfield(T Param::* p, std::istream & in)
    { return in >> this->*p, true; }
    bool fill(std::istream & in);
public:
    void update(const char * filename);
    bool checkexploration() const { return exploration >= 0; }
    bool checkweightfactor() const { return weightfactor >= 0; }
    bool checkstaged() const { return true; }
    bool checkmaxsize() const{ return true; }
    bool good() const
    {
        return
            checkexploration() &&
            checkweightfactor() &&
            checkstaged() &&
            checkmaxsize() &&
        true;
    }
    bool bad() const;
};

std::ostream & operator<<(std::ostream & out, Param const & param);

#endif // PARAM_H_INCLUDED
