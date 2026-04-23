#include <fstream>
#include <ostream>
#include <sstream>
#include "io.h"
#include "param.h"

bool Param::read(const char * filename)
{
    std::ifstream in(filename, std::ios::binary);
    if (!in.good()) return false;
    in.read(reinterpret_cast<char *>(this), sizeof Param);
    return true;
}

bool Param::save(const char * filename) const
{
    std::ofstream out(filename, std::ios::binary);
    if (!out) return false;
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

void Param::fill(std::istream & in)
{
    std::cout << "Format: field value, ended with anything invalid\n";
    Param newparam = *this;
    while (true)
    {
        std::string field, value;
        in >> field >> value;
        if (!in.good()) return;
        bool okay = false;
#define FILLFIELD(FIELD) \
        { \
            std::istringstream s(value); \
            okay |= field == #FIELD && \
                    (s >> newparam.FIELD, true); \
            if (okay) continue; \
        }
        FILLFIELD(exploration);
        FILLFIELD(freqbias);
        FILLFIELD(staged);
        FILLFIELD(maxsize);
#undef FILLFIELD
        break;
    }
    std::cout << "New parameters\n" << newparam;
    if (askyn("Save new parameters y/n?"))
        *this = newparam;
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
