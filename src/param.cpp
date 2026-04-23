#include <fstream>
#include <ostream>
#include <sstream>
#include "io.h"
#include "param.h"

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

void Param::fill(std::istream & in)
{
    std::cout << "Format: field value, ended with anything invalid\n";
    Param param = *this;
    while (true)
    {
        std::string field, value;
        in >> field >> value;
        if (!in.good()) return;
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
    if (askyn("Save new parameters y/n?"))
        *this = param;
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
    if (askyn("Edit parameters y/n?"))
        fill(std::cin);
    std::cout << p << &s[4 * save(filename)];
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
