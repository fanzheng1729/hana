#ifndef COMMENT_H_INCLUDED
#define COMMENT_H_INCLUDED

#include <map>
#include <vector>
#include "token.h"
#include "typecode.h"

const char mmws[] = "\t\n\f\r "; // metamath whitespace chars ($4.1.1)

struct Comment
{
    std::string text;
    Tokens::size_type tokenpos;
    operator Tokens::size_type() const { return tokenpos; }
};
struct Comments : std::vector<Comment>
{
// Classify comments ($4.4.2 and 4.4.3).
    std::vector<strview> operator[](strview type) const;
};

typedef std::vector<std::string> Command;
struct Commands : std::vector<Command>
{
    Commands(std::vector<strview> const & comments = std::vector<strview>());
// Classify comments ($4.4.3).
    Commands operator[](strview type) const;
};

// Map: constructor -> definition, or "" if none ($4.4.3)
struct Ctordefns : std::map<std::string, std::string>
{
    Ctordefns() {}
    Ctordefns(Commands const & definitions, Commands const & primitives);
};

struct Commentinfo
{
    Typecodes typecodes;
    Ctordefns ctordefns;
};

#endif // COMMENT_H_INCLUDED
