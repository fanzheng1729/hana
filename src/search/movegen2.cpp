#include "environ.h"

// Return true if all variables in use have been substituted.
static bool allvarsfilled(Varusage const & varusage, Stepranges const & subst)
{
    FOR (Varusage::const_reference rvar, varusage)
        if (rvar.first > 0 && subst[rvar.first].empty())
            return false;
    return true;
}

bool findsubstitutions(SteprangeAST exp, SteprangeAST tmp, Stepranges & subst);

static int const STACKEMPTY = -2;
// Advance the stack and return the difference in # matched hypotheses.
// Return STACKEMPTY if stack cannot be advanced.
static int next(Hypsizes & hypstack, std::vector<Stepranges> & substack,
                Assertion const & ass, Assertion const & thm)
{
    int delta = 0;
    while (!hypstack.empty())
    {
        Hypsize const thmhyp = thm.hypsorder[hypstack.size()-1];
        strview thmhyptype = thm.hyptypecode(thmhyp);
        // Advance the last hypothesis in the stack.
        Hypsize & asshyp = hypstack.back();
        if (asshyp < ass.nhyps())
            --delta; // 1 match removed
        for (++asshyp; asshyp <= ass.nhyps(); ++asshyp)
        {
            if (asshyp < ass.nhyps() &&
                (ass.hypfloats(asshyp) ||
                 ass.hyptypecode(asshyp) != thmhyptype))
                continue; // Skip floating hypothesis.
            // Copy the last substitution.
            substack[hypstack.size()] = substack[hypstack.size() - 1];
            if (asshyp == ass.nhyps())
                return delta; // No new match
            if (findsubstitutions
                (ass.hypRPNAST(asshyp), thm.hypRPNAST(thmhyp),
                 substack[hypstack.size()]))
                return ++delta; // New match
        }
        hypstack.pop_back();
    }
    return STACKEMPTY;
}

// Add Hypothesis-oriented moves.
// Return true if it has no open hypotheses.
bool Environ::addhypmoves(pAss pthm, Moves & moves,
                          Stepranges const & substitutions,
                          Hypsize maxfreehyps) const
{
    Assertion const & thm = pthm->second;
    // Hypothesis stack
    Hypsizes hypstack;
    // Substitution stack
    std::vector<Stepranges> substack(thm.nfreehyps() + 1);
    if (substack.empty() || assertion.nhyps() + 1 == 0)
        return false; // size overflow
    // Preallocate for efficiency.
    Hypsize const nfreehyps = thm.nfreehyps();
    hypstack.reserve(nfreehyps);
    // Bound substitutions
    substack[0] = substitutions;
    // substack.size() == hypstack.size() + 1
    // # matched hypotheses
    Hypsize matchedhyps = 0;
    int delta;
    do
    {
        // std::cout << hypstack;
        Stepranges const & newsubsts = substack[hypstack.size()];
        if (allvarsfilled(thm.varusage, newsubsts))
        {
            if (addboundmove(Move(pthm, newsubsts), moves))
                return true;
        }
        else
        if (hypstack.size() < nfreehyps && matchedhyps < maxfreehyps)
        {
            // Match new hypothesis.
            hypstack.push_back(static_cast<Hypsize>(-1));
        }
        delta = ::next(hypstack, substack, assertion, thm);
        matchedhyps += delta;
    } while (delta != STACKEMPTY);

    return false;
}
