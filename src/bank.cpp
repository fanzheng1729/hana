#include "bank.h"
#include "database.h"
#include "io.h"
#include "util/hex.h"
#include "util/for.h"

Bank::Bank(class Database const & database)
{
    //
}

// Type label delimiter
static const std::string typedelim = "~";

Symbol3 Bank::addRPN(Proofsteps const & RPN)
{
    if (RPN.empty())
        return "";
    // Find type code from the last step of the RPN.
    strview typecode = RPN.back().typecode();
    if (!typecode.c_str)
        return ""; // Bad step

    Symbol2::ID const id = rPNSymbols().size();
    Symbol3 & var = m_RPNSymbols[RPN];
    if (var.id != 0) // old RPN
        return var;
    // new RPN, to which variable #n is assigned
    // Name of variable = typecode~hex(n)
    m_varlabels.push_back(typecode.c_str + typedelim + util::hex(id));
    // Name of hypothesis = f~typecode~hex(n)
    m_hyplabels.push_back('f' + typedelim + m_varlabels[id]);
    strview hyplabel(m_hyplabels[id]);
    strview varlabel(m_varlabels[id]);
    Hypotheses::value_type const value(hyplabel, Hypothesis());
    // Iterator to floating hypothesis associated to the variable
    Hypotheses::iterator const hypiter = m_hypotheses.insert(value).first;
    hypiter->second.floats = true;
    // Expression of the hypothesis = {type code, variable}
    Expression & exp = hypiter->second.expression;
    exp.resize(2);
    exp[0] = typecode;
    // Symbol for the variable
    return exp[1] = var = Symbol3(varlabel, id, &*hypiter);
}

std::ostream & operator<<(std::ostream & out, const Bank & bank)
{
    FOR (Hypotheses::const_reference rhyp, bank.hypotheses())
        out << rhyp.first << ' ' << rhyp.second.expression;
    return out;
}
