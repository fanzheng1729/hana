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
    Tokens m_varvec;
    Hypotheses m_hypotheses;
public:
    Bank() : m_varvec(1, "") { addRPN(); }
    void clear() { *this = Bank(); }
    RPNSymbols const & rPNSymbols() const { return m_RPNSymbols; }
    Tokens const & varvec() const { return m_varvec; }
    Tokens::size_type varcount() const { return varvec().size() - 1; }
    Hypotheses const & hypotheses() const { return m_hypotheses; }
    Symbol3 addRPN(Proofsteps const & RPN = Proofsteps());
};

#endif // BANK_h_INCLUDED
