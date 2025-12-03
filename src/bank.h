#ifndef BANK_H_INCLUDED
#define BANK_H_INCLUDED

#include <deque>
#include <map>
#include "types.h"

// Storage for temporary variables
class Bank
{
    // Map: RPN -> Symbol
    typedef std::map<Proofsteps, Symbol3> RPNSymbols;
    RPNSymbols m_RPNSymbols;
    typedef std::deque<std::string> Tokens;
    Tokens m_hyplabels;
    Tokens m_varlabels;
    Hypotheses m_hypotheses;
public:
    Bank() : m_varlabels(1, ""), m_hyplabels(1, "")
        { m_RPNSymbols[Proofsteps()]; }
    Bank(class Database const & database);
    void clear() { *this = Bank(); }
    RPNSymbols const & rPNSymbols() const { return m_RPNSymbols; }
    Tokens::size_type varcount() const { return m_varlabels.size() - 1; }
    Hypotheses const & hypotheses() const { return m_hypotheses; }
    Symbol3 addRPN(Proofsteps const & RPN = Proofsteps());
};

#endif // BANK_h_INCLUDED
