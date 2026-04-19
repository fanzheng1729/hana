#include <iostream>
#include "../ass.h"
#include "../util/for.h"

static const char steperr[] = "Invalid proof step ";

strview RPNstep::typecode() const
{
    switch (type)
    {
    case HYP:
        if (!phyp) return "";
        if (phyp->second.expression.empty()) return "";
        return phyp->second.expression[0];
    case THM:
        return pass ? pass->second.exptypecode() : "";
    default:
        return "";
    }
}

// Return the name of the proof step.
RPNstep::operator const char*() const
{
    switch (type)
    {
    case HYP:
        if (!phyp) return NULL;
        return phyp->first.c_str;
    case THM:
        if (!pass) return NULL;
        return pass->first.c_str;
    case LOAD: case SAVE:
        std::cerr << steperr << "of type " << type << std::endl;
    default:
        return NULL;
    }
}

// label -> proof step, using hypotheses and assertions.
RPNstep RPNstep::Builder::operator()(strview label) const
{
    Hypiter const hypiter = m_hyps.find(label);
    if (hypiter != m_hyps.end())
        return hypiter;

    Assiter const assiter = m_assertions.find(label);
    if (assiter != m_assertions.end())
        return &*assiter;

    std::cerr << steperr << label.c_str << std::endl;
    return RPNstep::NONE;
}

static void writeprooferr(const char * thmlabel)
{
    std::cerr << "When writing proof using " << thmlabel;
}

static void hypothesiserr(const char * hyplabel)
{
    std::cerr << ", hypothesis " << hyplabel;
}

// Write the proof from pointers to proof of hypotheses. Return true if Okay.
bool writeproof(RPN & dest, pAss pthm, pProofs const & hyps)
{
    dest.clear();
    if (hyps.size() != pthm->second.nhyps())
    {
        writeprooferr(pthm->first.c_str);
        std::cerr << ", expected " << pthm->second.nhyps() << " hypotheses";
        std::cerr << ", but found " << hyps.size() << std::endl;
        return false;
    }
    // Total length ( +1 for label)
    RPNsize length = 1;
    for (Hypsize i = 0; i < hyps.size(); ++i)
    {
        pProof const phyp = hyps[i];
        if (!phyp || phyp->empty())
        {
            writeprooferr(pthm->first.c_str);
            hypothesiserr(pthm->second.hyplabel(i).c_str);
            std::cerr << " has no proof" << std::endl;
            return false;
        }
        if (phyp == &dest)
        {
            writeprooferr(pthm->first.c_str);
            hypothesiserr(pthm->second.hyplabel(i).c_str);
            std::cerr << " is the same as the conclusion" << std::endl;
            return false;
        }
        length += phyp->size();
    }
    // Preallocate for efficiency
    dest.reserve(length);
    FOR (pProof const phyp, hyps)
        dest += *phyp;
    // Label of the assertion used
    dest.push_back(pthm);
    //std::cout << "Built proof: " << proof;
    return true;
}

std::ostream & operator<<(std::ostream & out, RPN const & rpn)
{
    FOR (RPNstep const step, rpn)
        out << step << ' ';
    return out << std::endl;
}
