#include <fstream>
#include <ostream>
#include <sstream>
#include "io.h"
#include "param.h"

Param const Param::default = {3e-3, 0.5, false, 1u << 15};

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
        if (!in.good())
            break;
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
        FILLFIELD(weightfactor);
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
    if (bad())
        return false;

    std::ofstream out(filename, std::ios::binary);
    if (!out.good())
        return !unexpected(true, "file save error", filename);

    out.write(reinterpret_cast<const char *>(this), sizeof Param);
    return out.good();
}

void Param::update(const char * filename)
{
    if (!read(filename))
    {
        std::cout << "Using current parameters\n" << *this;
        return;
    }

    static const char p[] = "Parameters ";
    std::cout << p << std::endl << *this;
    if (!askyn("Edit parameters y/n"))
        return;

    const bool saved = fill(std::cin) && save(filename);
    std::cout << p << &"not saved\n"[4 * saved];
}

std::ostream & operator<<(std::ostream & out, Param const & param)
{
#define SHOWFIELD(FIELD) (out << #FIELD << '\t' << param.FIELD << std::endl)
    SHOWFIELD(exploration);
    SHOWFIELD(weightfactor);
    SHOWFIELD(staged);
    SHOWFIELD(maxsize);
#undef SHOWFIELD
    return out;
}
