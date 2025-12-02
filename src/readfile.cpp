#include <algorithm>
#include <cctype>       // for std::isprint
#include <fstream>
#include <iostream>
#include <locale>
#include <set>
#include <vector>
#include "comment.h"
#include "util/msg.h"
#include "token.h"
#include "util/find.h"
#include "util/for.h"   // for util::end

// Return true iff ch is not printable ($4.1.1).
static bool notprint(unsigned char ch)
{
    return !std::isprint(ch);
}

// Return the next token ($4.1.1).
static Token nexttoken(std::istream & in)
{
    if (!in.good())
        return Token();

    Token token;
    in >> token;

    // Iterator to first invalid character ($4.1.1)
    Token::const_iterator const iter
        (std::find_if(token.begin(), token.end(), notprint));
    if (iter != token.end())
    {
        std::cerr << "Invalid character read with code 0x";
        std::cerr << std::hex << (unsigned int)(unsigned char)*iter;
        std::cerr << std::dec << std::endl;
        return Token();
    }
    return token;
}

// Read file name in file inclusion commands ($4.1.2).
// Return the file name if okay; otherwise return the empty string.
static std::string readfilename(std::ifstream & in)
{
    std::string const filename(nexttoken(in));
    if (filename.find('$') < std::string::npos)
    {
        std::cerr << "Filename " << filename << " contains a $"
                  << std::endl;
        return std::string();
    }

    std::string const token(nexttoken(in));
    if (token != "$]")
    {
        std::cerr << "Didn't find closing file inclusion delimiter"
                  << std::endl;
        return std::string();
    }

    return filename;
}

// Prepare stream for I/O, switching the whitespace states of some chars.
static void preparestream(std::ios_base & ios, const char * chars)
{
    // Get the classic character classifying table
    typedef std::ctype<char> ctype;
    static const ctype::mask * const table(ctype::classic_table());

    // Duplicate it
    static const std::size_t size(ctype::table_size);
    static std::vector<ctype::mask> masks(table, table + size);
    static ctype::mask * const data(masks.data());

    std::size_t c;
    for (c = *chars; c; c = *++chars)
        masks[c] = table[c] ^ ctype::space;

    // Set the new character classifying table
    ios.imbue(std::locale(std::locale::classic(), new ctype(data)));
}

// Show error message for reading the file.
static void readfileerr(const char * msg, const char * name)
{
    std::cerr << msg << ' ' << name << std::endl;
}

// Associate the file stream with the file. Returns true if okay.
static bool openfile(std::ifstream & in, std::string const & name)
{
    // '\v' (vertical tab) is not a white space per spec ($4.1.1)
    preparestream(in, "\v");
    in.open(name.c_str());
    if (!in) readfileerr("Could not open", name.c_str());

    return bool(in);
}

// Read tokens. Returns true if okay.
static bool readtokens
    (const char * const filename, std::set<std::string> & names,
     Tokens & tokens, Comments & comments)
{
    if (!names.insert(filename).second)
        return true; // file already read

    std::ifstream in;
    if (!openfile(in, filename))
    {
        readfileerr("Could not open", filename);
        return false;
    }

    bool instatement = false;
    std::size_t scopecount = 0;

    Token token;
    while (!(token = nexttoken(in)).empty())
    {
//std::cout << token << ' ';

        if (token == "$(")
        {
            // Read and return a comment. Return "" on failure ($4.1.2).
            std::string comment(std::ifstream & in);
            Comment const newcomment = {comment(in), tokens.size()};
            if (newcomment.text.empty())
            {
                std::cerr << "Bad comment" << std::endl;
                return false;
            }

            comments.push_back(newcomment);
            continue;
        }

        if (token == "$[")
        {
            // File inclusion command only allowed in outermost scope ($4.1.2)
            if (scopecount > 0)
            {
                std::cerr << "File inclusion command not in outermost scope\n";
                return false;
            }
            // File inclusion command cannot be in a statement ($4.1.2)
            if (instatement)
            {
                std::cerr << "File inclusion command in a statement\n";
                return false;
            }

            std::string const & newfilename(readfilename(in));
            if (newfilename.empty())
            {
                std::cerr << "Unfinished file inclusion command" << std::endl;
                return false;
            }

            if (!readtokens(newfilename.c_str(), names, tokens, comments))
            {
                readfileerr("Error reading from included", newfilename.c_str());
                return false;
            }

            continue;
        }

        // types of statements
        const char * const statements[] = {"$c", "$v", "$f", "$e", "$d", "$a", "$p"};

        // Count scopes
        if (token == "${")
            ++scopecount;
        else if (token == "$}")
        {
            if (scopecount == 0)
            {
                std::cerr << extraendscope << std::endl;
                return false;
            }
            --scopecount;
        }

        // Detect statements
        else if (util::find(statements, token) != util::end(statements))
            instatement = true;
        else if (token == "$.")
            instatement = false;

        tokens.push_back(token);
    }

    if (!in.eof())
    {
        if (in.fail())
            readfileerr("Error reading from", filename);
        return false;
    }

    return true;
}

// Read tokens. Returns true if okay.
bool doread(const char * filename,
            struct Tokens & tokens, struct Comments & comments)
{
    std::set<std::string> names;
    return readtokens(filename, names, tokens, comments);
}
