#include "../ass.h"
#include "../io.h"
#include "../typecode.h"
#include "../util/hex.h"
#include "verify.h"

bool Printer::addstep(Expression const & stacktop, Proofsize index,
                      strview label, Symbol2 save)
{
    if (stacktop.empty())
        return false;

    if (ptypes->isprimitive(stacktop.front()))
        return true;

    stack.push_back(steps.size());

    steps.push_back(Expression(stacktop.size() + 3));

    Expression & dest(steps.back());
    dest[0] = label;
    dest[1] = save;
    dest[2].id = index;
    std::copy(stacktop.begin(), stacktop.end(), dest.begin() + 3);

    return true;
}

bool Printer::doaddstep(Proofstep step, Proofsize index, Expression const & stacktop)
{
//std::cout << "Step " << step << ", top of stack: " << stacktop;
    switch (step.type)
    {
    case Proofstep::HYP:
        return addstep(stacktop, index, step.phyp->first);
    case Proofstep::ASS:
        {
            Assertion const & ass(step.pass->second);
            Hypsize const esscount(ass.esshypcount());
            if (!enoughitemonstack(esscount, stack.size(), ""))
                return false;
            // Pop all the essential hypotheses.
            stack.resize(stack.size() - esscount);
            return addstep(stacktop, index, step.pass->first);
        }
    case Proofstep::LOAD:
        return addstep(stacktop, index, "", Symbol2("<=", step.index + 1));
    case Proofstep::SAVE:
        if (stacktop.empty())
            return false;
        if (!ptypes->isprimitive(stacktop[0]))
            steps.back()[2] = Symbol2(">=", ++savecount);
        return true;
    default:
        return false;
    }
    return false;
}

std::string Printer::str(std::vector<Proofsize> const & indentation) const
{
    std::string result;
    FOR (Expression const & step, steps)
    {
        // Justification + indentation + expression + tag #
        (result += strview(step[0])) += '\t';
        result += std::string(indentation[step[2]], ' ');
        for (Expression::size_type i(3); i < step.size(); ++i)
            (result += strview(step[i])) += ' ';
        if (!step[1].empty())
            ((result += strview(step[1])) += ' ') += util::hex(step[2].id);
        result += '\n';
    }

    return result;
}
