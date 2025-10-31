#include <cctype>       // for std::isupper
#include <iostream>
#include "getproof.h"   // for Readretval
#include "token.h"
#include "util/for.h"

// Determine if there is no more token before finishing a statement.
bool unfinishedstat(Tokens const & tokens, strview stattype, strview label)
{
    if (tokens.empty())
        std::cerr << "Unfinished " << stattype.c_str << " statement " << label.c_str
                  << std::endl;
    return tokens.empty();
}

// Print error message (if any) in the proof of label.
Readretval printprooferr(strview label, Readretval err)
{
    static const char * const prefix[] = {"Warning: incomplete", "Error: no"};
    if (err != 1)
        std::cerr << prefix[err + 1] << " proof for theorem "
                  << label.c_str << std::endl;
    return err;
}

// Determine if a char is invalid in a compressed proof (Appendix B).
static bool isbad(unsigned char c)
{
    return !std::isupper(c) && c != '?';
}

// If a token is valid in a compressed proof (Appendix B), return 0.
// Otherwise return the bad letter.
static char badletter(strview token)
{
    const char * s = token.c_str;
    while (!isbad(*s)) ++s;
    return *s;
}

// Get sequence of letters in a compressed proof (Appendix B).
// Returns 1 if Okay, 0 on error, -1 if the proof is incomplete.
Readretval getproofletters(strview label, Tokens & tokens, std::string & proof)
{
    strview token;

    while (!tokens.empty() && (token = tokens.front()) != "$.")
    {
        const char c = badletter(token);
        if (c)
        {
            std::cerr << "Bogus character " << c
                      << " in compressed proof of " << label.c_str << std::endl;
            return Readretval::PROOF_BAD;
        }

        proof += token.c_str;
        tokens.pop();
    }

    if (unfinishedstat(tokens, "$p", label))
        return Readretval::PROOF_BAD;

    tokens.pop(); // Discard $. token

    Readretval err = proof.empty() ? Readretval::PROOF_BAD :
                    proof.find('?') != Token::npos ? INCOMPLETE : PROOF_OKAY;
    return printprooferr(label, err);
}

// Subroutine for calculating proof number. Returns true iff okay.
template<Proofnumber MUL>
static bool FMA(Proofnumber & num, int add, strview label)
{
    static const Proofnumber size_max = -1;

    if (num > size_max / MUL || MUL * num > size_max - add)
    {
        std::cerr << "Overflow computing numbers in compressed proof of "
                  << label.c_str << std::endl;
        return false;
    }

    num = MUL * num + add;
    return true;
}

// Get the raw numbers from compressed proof format.
// The letter Z is translated as 0 (Appendix B).
Proofnumbers getproofnumbers(strview label, std::string const & proof)
{
    Proofnumbers result;
    result.reserve(proof.size()); // Preallocate for efficiency

    std::size_t num = 0u;
    bool justgotnum(false);
    FOR (char const c, proof)
    {
        if (c <= 'T')
        {
            if (!FMA<20>(num, c - ('A' - 1), label))
                return Proofnumbers();

            result.push_back(num);
            num = 0u;
            justgotnum = true;
        }
        else if (c <= 'Y')
        {
            if (!FMA<5>(num, c - 'T', label))
                return Proofnumbers();

            justgotnum = false;
        }
        else // It must be Z
        {
            if (!justgotnum)
            {
                std::cerr << "Stray Z found in compressed proof of "
                          << label.c_str << std::endl;
                return Proofnumbers();
            }

            result.push_back(0u);
            justgotnum = false;
        }
    }

    if (num != 0u)
    {
        std::cerr << "Compressed proof of theorem " << label.c_str
                  << " ends in unfinished number" << std::endl;
        return Proofnumbers();
    }

    return result;
}
