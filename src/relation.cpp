#include <algorithm>// for std::max_element
#include "ass.h"
#include "relation.h"
#include "util/filter.h"
#include "util/for.h"

// Return true if RPN matches pattern.
static bool matchline(Proofsteps const & steps, int const * & cur,
                      int const * end, Proofsteps & substitutions)
{
    for (Proofsize i = 0; i < steps.size(); ++i, ++cur)
    {
        if (cur >= end || *cur < 0)
            return false;
        // 0 corresponds to the syntax axiom.
        if (*cur == 0 && steps[i].type != Proofstep::ASS)
            return false;
        // !0 corresponds to a variable.
        if (*cur != 0 && steps[i].type != Proofstep::HYP)
            return false;

        if (substitutions[*cur].type == Proofstep::NONE)
        {
            // A new step
            if (util::filter(substitutions)(steps[i]))
                return false; // seen before.
            substitutions[*cur] = steps[i]; // record it.
        }
        // Otherwise check if it matches the recod.
        else if (substitutions[*cur] != steps[i])
            return false; // mismatch
    }
    return true;
}

// If RPN of the expression matches a given pattern,
// return the proof step correponding to 0. Otherwise return NONE.
static Proofstep match(Assertion const & ass, const int pattern[])
{
    // End of pattern
    const int * end = pattern;
    while (*end != Relations::PATTERN_END) ++end;

    // Max # variables to substitute
    const int argc = *std::max_element(pattern, end);
    // Substitution vector
    Proofsteps substitutions(argc + 1, Proofstep::NONE);

    // Match hypotheses.
    const int * cur = pattern;
    for (Hypsize i = 0; i < ass.hypcount(); ++i)
    {
        if (ass.hypfloats(i)) continue;

        if (!matchline(ass.hypRPN(i), cur, end, substitutions))
            return Proofstep();
        if (*cur++ != Relations::LINE_END) // Match end of line.
            return Proofstep();
    }
    // Match conclusion.
    if (!matchline(ass.expRPN, cur, end, substitutions))
        return Proofstep();
    if (cur != end) // Match end of pattern.
        return Proofstep();

    // Return the proof string correponding to 0.
    return substitutions[0];
}

// Find relations and their justifications among syntax axioms.
Relations::Relations(Assertions const & assertions)
{
    FOR (Assertions::const_reference ass, assertions)
        for (unsigned i = 0; i < NJUSTIFICATION; ++i)
            if (Proofstep const step
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
