#include <algorithm>// for std::lower_bound
#include <numeric>  // for std::accumulate
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include "comment.h"
#include "io.h"
#include "types.h"
#include "util/for.h"

// Return the first token in a string, or "" if there is not any.
static Token firsttoken(std::string const & str)
{
    std::string::size_type const begin = str.find_first_not_of(mmws);
    return begin == std::string::npos ? "" :
        str.substr(begin, str.find_first_of(mmws, begin) - begin);
}


// Classify comments ($4.4.2 and 4.4.3).
std::vector<strview> Comments::operator[](strview type) const
{
    std::vector<strview> result;
    FOR (const_reference comment, *this)
        if (firsttoken(comment.text) == type)
            result.push_back(comment.text);
    return result;
}

static void commenterr(const char * badstring, std::string const & comment)
{
    std::cerr << "Characters " << badstring << " in a comment:\n";
    std::cerr << comment << std::endl;
}

// Read and return a comment. Return "" on failure ($4.1.2).
std::string comment(std::ifstream & in)
{
    std::stringstream result;//("", std::ios_base::ate);

    while (in.good())
    {
        // Read up to '$'. Skip if the next char is '$'. Otherwise in fails.
        in.get(*result.rdbuf(), '$');

        // Eat '$' in the stream.
        if (in.eof() || in.bad() || (in.clear(), in.get(), !in.good()))
        {
            std::cerr << "Unclosed comment" << result.str() << std::endl;
            return "";
        }

        // Check if the token is legal.
        char c(in.get());
        if (c == '(')
        {
            commenterr("$(", result.str());
            return "";
        }

        if (c == ')')
        {
            // the last char read
            char last = 0;
            result.seekg(-1, std::ios::end);
            result >> last;

            if (std::strchr(mmws, last) == NULL)
            {
                commenterr("...$)", result.str());
                return "";
            }

            // The token begins with "$)". Check if it ends here.
            if (in.eof())
                return result.str();
            if (!in.good())
                return "";

            // the next char to read
            if (std::strchr(mmws, in.peek()) == NULL)
            {
                commenterr("$)...", result.str());
                return "";
            }

            // "$)" is legal.
            return result.str();
        }

        // "$" followed by other chars
        result.seekg(0, std::ios::end);
        result << '$' << c;
    }

    return "";
}

// Read commands from comments ($4.4.2 and 4.4.3). C-comment unsupported.
static Commands * addcmd(Commands * cmds, std::string str)
{
    char * token(std::strtok(&str[0], mmws));
    if (!token || std::strlen(token) != 2 || token[0] != '$')
        return cmds;
    // metamath comment types ($4.4.2 and $4.4.3)
    if (std::strchr("jt", token[1]) == NULL)
        return cmds;
    // Move to next token.
    char * tokenend(token + std::strlen(token));
    if (tokenend < str.data() + str.size())
        ++tokenend;

    cmds->push_back(Command());
    while ((token = std::strtok(tokenend, mmws)))
    {
        // end of token.
        tokenend = token + std::strlen(token);
        // Break token at ';'.
        char * s(std::strtok(token, ";"));
        while (s)
        {
            // Add token.
            cmds->back().push_back(s);
            // Check if there is a semicolon.
            if (s + std::strlen(s) < tokenend)
                cmds->push_back(Command());
            // Find new token.
            s = std::strtok(NULL, ";");
        }
        // Move to next token.
        if (tokenend < str.data() + str.size())
            ++tokenend;
    }
    // Remove possible empty command.
    if (cmds->back().empty()) cmds->pop_back();
    return cmds;
}

template<class It, class T, class Op>
static void accumulate(It first, It last, T init, Op op)
{
    (void)std::accumulate(first, last, init, op);
}

Commands::Commands(std::vector<strview> const & comments)
{
    ::accumulate(comments.begin(), comments.end(), this, addcmd);
}

// Classify comments ($4.4.3).
Commands Commands::operator[](strview type) const
{
    Commands result;

    FOR (const_reference command, *this)
        if (!command.empty() && command[0] == type)
        {
            result.resize(result.size() + 1);
            result.back().assign(command.begin() + 1, command.end());
        }

    return result;
}

// Return the unquoted word. Return "" if it is not quoted.
static std::string unquote(std::string const & str)
{
    return (str.size()<3 || str[0] != '\'' || str[str.size()-1] != '\'') ? "" :
        str.substr(1, str.size() - 2);
}

std::ostream & operator<<(std::ostream & out, Commands commands)
{
    FOR (Command const & command, commands)
        out << command;

    return out;
}

static Typecodes * addsyntax(Typecodes * p, Command const & command)
{
    if (unexpected(command.size() != 1 &&
                   !(command.size() == 3 && command[1] == "as"),
                   "syntax command", command))
        return p;
    // Get the type.
    std::string const & type(unquote(command[0]));
    if (unexpected(type.empty(), "type code", command[0]))
        return p;
    // Add the type.
    std::pair<Typecodes::iterator, bool> result
        (p->insert(std::make_pair(type, Typecodes::mapped_type())));
    if (result.second == false)
    {
        std::cerr << "Type code " << type << " already exists\n";
        return p;
    }
    // No as type.
    if (command.size() == 1)
        return p;
    // Add the as type.
    std::string const & astype(unquote(command.back()));
    if (unexpected(astype.empty(), "type code", command.back()))
        return p;
    if (p->count(astype) == 0)
    {
        std::cerr << "Type code " << astype << " does not exist\n";
        return p;
    }
    result.first->second.first = astype;
    return p;
}

static Typecodes * addbound(Typecodes * p, Command const & command)
{
    if (unexpected(command.size() != 1, "bound syntax command", command))
        return p;
    // Get the type.
    Typecodes::iterator const iter(p->find(unquote(command[0])));
    if (unexpected(iter == p->end(), "type code", command[0]))
        return p;
    // Add it as a bound type.
    iter->second.second = true;
    return p;
}

Typecodes::Typecodes(struct Commands const & syntax, Commands const & bound)
{
    ::accumulate(syntax.begin(), syntax.end(), this, addsyntax);
    ::accumulate(bound.begin(), bound.end(), this, addbound);
}

static Ctordefns * adddefinition(Ctordefns * p, Command const & command)
{
    if (unexpected(command.size() != 3
                   || command[1] != "for", "command", command))
        return p;
    // Get the constructor.
    std::string const & ctor(unquote(command[2]));
    if (unexpected(ctor.empty(), "constructor", command[2]))
        return p;
    // Get its definition.
    std::string const & defn(unquote(command[0]));
    if (unexpected(defn.empty(), "definition", command[0]))
        return p;
    // Add the constructor.
    if (p->insert(std::make_pair(ctor, defn)).second == false)
        std::cerr << "Constructor " << ctor << " already exists\n";
    return p;
}

static Ctordefns * addprimitives(Ctordefns * p, Command const & command)
{
    FOR (std::string const & token, command)
    {
        // Get the constructor.
        std::string const & ctor(unquote(token));
        if (unexpected(ctor.empty(), "constructor", token))
            return p;
        // Add the constructor.
        if (p->insert(std::make_pair(ctor, "")).second == false)
            std::cerr << "Constructor " << ctor << " already exists\n";
    }
    return p;
}

Ctordefns::Ctordefns(Commands const & definitions, Commands const & primitives)
{
    ::accumulate(definitions.begin(), definitions.end(), this, adddefinition);
    ::accumulate(primitives.begin(), primitives.end(), this, addprimitives);
}
