#include "bank.h"
#include "io.h"
#include "util/hex.h"
#include "util/for.h"

// Type label delimiter
static const std::string typedelim = "~";
static const std::string floatinghypheader = "f" + typedelim;

Symbol3 Bank::addRPN(Proofsteps const & RPN)
{
    if (RPN.empty())
        return "";

    strview typecode = RPN.back().typecode();
    if (!typecode.c_str)
        return "";

    std::pair<Proofsteps const &, Symbol3> const value(RPN, "");
    RPNSymbols::iterator const RPNiter = m_RPNSymbols.insert(value).first;
    Symbol3 & var = RPNiter->second;
    if (var.id != 0) // old RPN
        return var;
    // new RPN, to which variable #id is assigned
    Symbol2::ID const id = m_varlabels.size();
    // Name of variable = typecode~hex(n)
    m_varlabels.push_back(typecode.c_str + typedelim + util::hex(id));
    // Name of hypothesis = f~typecode~hex(n)
    m_hyplabels.push_back(floatinghypheader + m_varlabels[id]);
    // Substitution of the variable = RPN
    m_substitutions.push_back(&RPNiter->first);
    strview hyplabel(m_hyplabels[id]);
    strview varlabel(m_varlabels[id]);
    // Iterator to floating hypothesis associated to the variable
    Hypotheses::iterator const hypiter
    = m_hypotheses.insert(std::make_pair(hyplabel, Hypothesis())).first;
    hypiter->second.floats = true;
    // Expression of the hypothesis = {type code, variable}
    Expression & exp = hypiter->second.expression;
    exp.resize(2);
    exp[0] = typecode;
    // Symbol for the variable
    exp[1] = var = Symbol3(varlabel, id, &*hypiter);
    // rev-Polish notation of the hypothesis
    hypiter->second.RPN.assign(1, hypiter);

    return var;
}

std::ostream & operator<<(std::ostream & out, const Bank & bank)
{
    FOR (Hypotheses::const_reference rhyp, bank.hypotheses())
        out << rhyp.first << ' ' << rhyp.second.expression;
    return out;
}
