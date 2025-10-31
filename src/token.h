#ifndef TOKEN_H_INCLUDED
#define TOKEN_H_INCLUDED

#include <cctype>
#include <deque>
#include "util/strview.h"
#include "util/util.h"      // for util::none_of

typedef std::string Token;
// A deque of tokens for input. Tokens are not destroyed after popping.
struct Tokens : private std::deque<Token>
{
    size_type position;
    Tokens(): position(0) {}
    using deque::size_type;
    using deque::size;
    using deque::push_back;
    bool empty() const { return position >= size(); }
    strview front() const { return (*this)[position]; }
    void pop() { ++position; }
};

// Determine if a char cannot appear in a label ($4.1.1).
inline bool badlabelchar(unsigned char c)
{
    return !std::isalnum(c) && !std::strchr(".-_", c);
}

// Determine if a token is a label token ($4.1.1).
inline bool islabeltoken(Token const & token)
{
    return util::none_of(token.begin(), token.end(), badlabelchar);
}

// Determine if a token is a math symbol token ($4.1.1).
inline bool ismathsymboltoken(strview token)
{
    return !std::strchr(token.c_str, '$');
}

#endif // TOKEN_H_INCLUDED
