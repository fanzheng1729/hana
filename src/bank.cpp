#include "bank.h"
#include "io.h"
#include "proof/analyze.h"
#include "proof/verify.h"
#include "util/hex.h"
#include "util/for.h"

// Type label delimiter
static const std::string typedelim = "~";
static const std::string essentialhypheader = "e" + typedelim;
static const std::string floatinghypheader = "f" + typedelim;

// Add an abstraction variable.
Symbol3 Bank::addabsvar(Steprange absRPN)
{
    if (absRPN.empty())
        return "";

    RPNSymbols::value_type value(Proofsteps(absRPN.first, absRPN.second), "");
    RPNSymbols::iterator const RPNiter = m_RPNSymbols.insert(value).first;
    // New variable
    Symbol3 & var = RPNiter->second;
    if (var.id > 0) // old RPN
        return var;
    // New RPN, to which variable #id is assigned
    Symbol2::ID const id = m_varlabels.size();

    strview typecode = absRPN.root().typecode();
    m_varlabels.push_back(typecode.c_str + typedelim + util::hex(id));
    m_fhyplabels.push_back(floatinghypheader + m_varlabels[id]);
    m_RPNSymbolsbyid.push_back(RPNiter);

    strview hyplabel(m_fhyplabels[id]);
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
    exp[1] = var = Symbol3(varlabel, id, hypiter);
    // rev-Polish notation of the hypothesis
    hypiter->second.RPN.assign(1, hypiter);
    // Abstract syntax tree of the hypothesis
    hypiter->second.ast.resize(1);

    return var;
}

Hypiter Bank::addhyp(Proofsteps const & RPN, strview typecode)
{
    Hypothesis hyp;
    // Expression of the hypothesis
    Expression & exp = hyp.expression = verify(RPN);
    if (exp.empty())
        return Hypiter();
    // Abstract syntax tree of the hypothesis
    hyp.ast = ast(RPN);
    if (hyp.ast.empty())
        return Hypiter();

    exp[0] = typecode;
    Hypiter & hypiter = m_hypitersbyexp[exp];
    if (hypiter != Hypiter())
        return hypiter;

    hyp.floats = false;
    hyp.RPN = RPN;
    // # of the hypothesis
    Hypsize const n = m_ehyplabels.size();
    // label of the hypothesis
    m_ehyplabels.push_back(essentialhypheader + util::hex(n));
    // Add the hypothesis.
    std::pair<strview, Hypothesis const &> value(m_ehyplabels[n], hyp);
    return hypiter = m_hypotheses.insert(value).first;
}

std::ostream & operator<<(std::ostream & out, const Bank & bank)
{
    FOR (Hypotheses::const_reference rhyp, bank.hypotheses())
        out << rhyp.first << ' ' << rhyp.second.expression;
    return out;
}
