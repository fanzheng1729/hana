#include <algorithm>// for std::max_element
#include "ass.h"
#include "relation.h"
#include "util/filter.h"
#include "util/for.h"

// Return true if a line matches pattern.
static bool matchline
    (RPN const & line, int const * & cur, int const * end, RPN & substs)
{
    FOR (RPNstep step, line)
    {
        if (cur >= end || *cur < 0)
            return false;
        // 0 corresponds to the syntax axiom.
        if (*cur == 0 && !step.isthm())
            return false;
        // !0 corresponds to a variable.
        if (*cur != 0 && !step.id())
            return false;

        if (substs[*cur].empty())
        {
            // A new step
            if (util::filter(substs)(step))
                return false; // seen before.
            substs[*cur] = step; // record it.
        }
        // Otherwise check if it matches the recod.
        else if (substs[*cur] != step)
            return false; // mismatch
        ++cur;
    }
    return true;
}

// If the assertion matches a given pattern,
// return the proof step correponding to 0. Otherwise return NONE.
static RPNstep match(Assertion const & ass, const int pattern[])
{
    // End of pattern
    const int * end = pattern;
    while (*end != Relations::PATTERN_END) ++end;

    // Substitution vector
    RPN substs(*std::max_element(pattern, end) + 1);

    // Match hypotheses.
    const int * cur = pattern;
    for (Hypsize i = 0; i < ass.nhyps(); ++i)
    {
        if (ass.hypfloats(i)) continue;

        if (!matchline(ass.hypRPN(i), cur, end, substs))
            return RPNstep();
        if (*cur++ != Relations::LINE_END) // Match end of line.
            return RPNstep();
    }
    // Match conclusion.
    if (!matchline(ass.expRPN, cur, end, substs))
        return RPNstep();
    if (cur != end) // Match end of pattern.
        return RPNstep();

    // Return the proof string correponding to 0.
    return substs[0];
}

// Find relations and their justifications among syntax axioms.
Relations::Relations(Assertions const & assertions)
{
    FOR (Assertions::const_reference ass, assertions)
        for (unsigned i = 0; i < NJUSTIFICATION; ++i)
            if (RPNstep const step
                = match(ass.second, Relations::patterns()[i]))
            {
                Justifications & just((*this)[strview(step)]);
                just.pass = step.pass;
                if (!just[i] ||
                    just[i]->second.number > ass.second.number)
                    just[i] = &ass;
                break;
            }
}
