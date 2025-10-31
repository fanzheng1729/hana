#include <iostream>
#include "../ass.h"
#include "../util/for.h"

static const char proofsteperr[] = "Invalid proof step ";

// Return symbol of the variable.
Symbol3 Proofstep::symbol() const
{
    if (type != HYP) return Symbol3();
    Hypothesis const & hyp = phyp->second;
    return (hyp.floats && hyp.expression.size() == 2) ? hyp.expression[1] : Symbol3();
}

// Return the name of the proof step.
Proofstep::operator const char*() const
{
    switch (type)
    {
    case HYP:
        return phyp->first.c_str;
    case ASS:
        return pass->first.c_str;
    case LOAD: case SAVE:
        std::cerr << proofsteperr << "of type " << type << std::endl;
    default:
        return NULL;
    }
    return NULL;
}

// label -> proof step, using hypotheses and assertions.
Proofstep Proofstep::Builder::operator()(strview label) const
{
    Hypiter const hypiter = m_hyps.find(label);
    if (hypiter != m_hyps.end())
        return hypiter; // hypothesis

    Assiter const assiter = m_assertions.find(label);
    if (assiter != m_assertions.end())
        return &*assiter; // assertion

    std::cerr << proofsteperr << label.c_str << std::endl;
    return Proofstep::NONE;
}

// Write the proof from pointers to proof of hypotheses. Return true if Okay.
bool writeproof(Proofsteps & dest, Assptr pass, pProofs const & hyps)
{
    // Hypotheses
    dest.clear();
    if (hyps.size() != pass->second.hypcount())
    {
        std::cerr << "When writing proof using " << pass->first.c_str;
        std::cerr << ", expected " << pass->second.hypcount() << " hypotheses";
        std::cerr << ", but found " << hyps.size() << std::endl;
        return false;
    }
    // Total length ( +1 for label)
    Proofsize length = 1;
    for (Hypsize i = 0; i < hyps.size(); ++i)
    {
        Proofsteps const * const p = hyps[i];
        if (!p || p->empty())
        {
            std::cerr << "When writing proof using " << pass->first.c_str;
            std::cerr << ", hypothesis " << pass->second.hyplabel(i).c_str;
            std::cerr << " has no proof" << std::endl;
            return false;
        }
        if (p == &dest)
        {
            std::cerr << "When writing proof using " << pass->first.c_str;
            std::cerr << ", hypothesis " << pass->second.hyplabel(i).c_str;
            std::cerr << " is the same as the conclusion" << std::endl;
            return false;
        }
        length += p->size();
    }
    // Preallocate for efficiency
    dest.reserve(length);
    FOR (Proofsteps const * p, hyps)
        dest += *p;
    // Label of the assertion used
    dest.push_back(pass);
    //std::cout << "Built proof: " << proof;
    return true;
}

std::ostream & operator<<(std::ostream & out, Proofsteps const & steps)
{
    FOR (Proofstep step, steps)
        out << step << ' ';
    return out << std::endl;
}
