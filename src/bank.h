#ifndef BANK_H_INCLUDED
#define BANK_H_INCLUDED

#include <deque>
#include "types.h"

// Storage for temporary variables
class Bank
{
    // Map: RPN -> Symbol
    typedef std::map<Proofsteps, Symbol3> RPNSymbols;
    RPNSymbols m_RPNSymbols;
    // Vector of substitutions
    typedef std::vector<Proofsteps const *> Substitutions;
    Substitutions m_substitutions;
    typedef std::deque<std::string> Tokens;
    Tokens m_hyplabels;
    Tokens m_varlabels;
    Hypotheses m_hypotheses;
public:
    // # reserved variables
    Tokens::size_type const nReserve;
    // Variable id starts from reserve + 1.
    Bank(Tokens::size_type reserve = 0) :
        m_varlabels(1 + reserve, ""), m_hyplabels(1 + reserve, ""),
        nReserve(reserve)
    {
        m_RPNSymbols[Proofsteps()];
        m_substitutions.assign(1 + reserve, &m_RPNSymbols.begin()->first);
    }
    RPNSymbols const & rPNSymbols() const { return m_RPNSymbols; }
    Substitutions const & substitutions() const { return m_substitutions; }
    Proofsteps const & substitution(Symbol2::ID id) const
    { return *substitutions()[id]; }
    Tokens::size_type varcount() const { return m_varlabels.size() - 1; }
    Hypotheses const & hypotheses() const { return m_hypotheses; }
    Symbol3 addRPN(Proofsteps const & RPN = Proofsteps());
};

#endif // BANK_h_INCLUDED
