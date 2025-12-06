#ifndef BANK_H_INCLUDED
#define BANK_H_INCLUDED

#include <deque>
#include "types.h"

// Storage for temporary variables
class Bank
{
    // Map: revPolish notation -> Symbol
    typedef std::map<Proofsteps, Symbol3> RPNSymbols;
    RPNSymbols m_RPNSymbols;
    // Vector of substitutions
    typedef std::vector<RPNSymbols::const_iterator> RPNSymbolsbyid;
    RPNSymbolsbyid m_RPNSymbolsbyid;
    typedef std::deque<std::string> Tokens;
    Tokens m_varlabels;
    Tokens m_ehyplabels;
    Tokens m_fhyplabels;
    Hypotheses m_hypotheses;
public:
    // Variable id starts from reserve + 1.
    Bank(Symbol2::ID reserve = 0) :
        m_varlabels(1 + reserve, ""),
        m_fhyplabels(1 + reserve, ""), m_ehyplabels(1, "")
    {
        m_RPNSymbols[Proofsteps()];
        m_RPNSymbolsbyid.assign(1 + reserve, m_RPNSymbols.begin());
    }
    RPNSymbols const & rPNSymbols() const { return m_RPNSymbols; }
    Proofsteps const & substitution(Symbol2::ID id) const
    { return m_RPNSymbolsbyid[id]->first; }
    Symbol3 var(Symbol2::ID id) const { return m_RPNSymbolsbyid[id]->second; }
    Tokens::size_type varcount() const { return m_varlabels.size() - 1; }
    Symbol3 addRPN(Proofsteps const & RPN);
    Hypotheses const & hypotheses() const { return m_hypotheses; }
    Hypiter addhyp(Proofsteps const & RPN, strview typecode);
};

#endif // BANK_h_INCLUDED
