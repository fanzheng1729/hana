#include "ass.h"
#include "varbank.h"
#include "util/hex.h"

Symbol3 Varbank::addRPN(Proofsteps const & RPN)
{
    // First call, RPN is empty
    if (RPN.empty())
        return m_RPNSymbols[RPN];
    // Later calls, RPN is !empty
    // Find type code from the last step of the RPN.
    strview typecode = RPN.back().typecode();
    if (typecode == "")
        return ""; // Bad step
    // id of the variable, starting from 1
    Symbol2::ID const n = rPNSymbols().size();
    Symbol3 & var = m_RPNSymbols[RPN];
    if (var.id != 0) // old RPN
        return var;
    // new RPN, to which variable #n is assigned
    // Name of variable = hex(n) = 0x########
    m_varvec.push_back(util::hex(n));
    strview label(m_varvec[n]);
    Hypotheses::value_type const value(label, Hypothesis());
    // Iterator to floating hypothesis associated to the variable
    Hypotheses::iterator const hypiter = m_hypotheses.insert(value).first;
    hypiter->second.floats = true;
    // Expression of the hypothesis = {type code, variable}
    Expression & exp = hypiter->second.expression;
    exp.resize(2);
    exp[0] = typecode;
    // Symbol for the variable
    return exp[1] = var = Symbol3(label, n, &*hypiter);
}
