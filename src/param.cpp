#include <fstream>
#include <ostream>
#include <sstream>
#include "io.h"
#include "param.h"

Param const Param::default = {1e-3, 0, false, 1u << 14};
//Param const Param::default = {1e-4, 0, true, 1u << 14};
bool Param::bad() const
{
    return unexpected(!good(), "parameter\n", *this);
}

bool Param::read(const char * filename)
{
    Param param;
    std::ifstream in(filename, std::ios::binary);
    if (!in.good())
        return !unexpected(true, "file open error", filename);
    in.read(reinterpret_cast<char *>(&param), sizeof Param);
    return param.good() ? (*this = param, true) : !param.bad();
}

bool Param::fill(std::istream & in)
{
    std::cout << "Format: field value, ended with anything invalid\n";
    Param param = *this;
    while (true)
    {
        std::string field, value;
        in >> field >> value;
        if (!in.good()) return false;
#define FILLFIELD(FIELD) \
        { \
            std::istringstream s(value); \
            if (field == #FIELD) \
            { \
                Param newparam = param; \
                s >> newparam.FIELD; \
                if (newparam.check##FIELD()) \
                    param = newparam; \
                else \
                    unexpected(true, #FIELD, newparam.FIELD); \
                continue; \
            } \
        }
        FILLFIELD(exploration);
        FILLFIELD(freqbias);
        FILLFIELD(staged);
        FILLFIELD(maxsize);
#undef FILLFIELD
        break;
    }
    std::cout << "New parameters\n" << param;
    return
    askyn("Save new parameters y/n?") && !bad() ? (*this = param, true) : false;
}

bool Param::save(const char * filename) const
{
    if (bad()) return false;
    std::ofstream out(filename, std::ios::binary);
    if (!out.good())
        return !unexpected(true, "file save error", filename);
    out.write(reinterpret_cast<const char *>(this), sizeof Param);
    return out.good();
}

void Param::update(const char * filename)
{
    read(filename);
    const char p[] = "Parameters ", s[] = "not saved\n";
    std::cout << p << std::endl << *this;
    const bool saved = askyn("Edit parameters y/n") &&
        fill(std::cin) && save(filename);
    std::cout << p << &s[4 * saved];
}

std::ostream & operator<<(std::ostream & out, Param const & param)
{
#define SHOWFIELD(FIELD) (out << #FIELD << '\t' << param.FIELD << std::endl)
    SHOWFIELD(exploration);
    SHOWFIELD(freqbias);
    SHOWFIELD(staged);
    SHOWFIELD(maxsize);
#undef SHOWFIELD
    return out;
}
