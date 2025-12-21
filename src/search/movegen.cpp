#include <algorithm>    // for std::min
#include "../proof/analyze.h"
#include "../proof/skeleton.h"
#include "environ.h"
#include "problem.h"

// Moves generated at a given stage
Moves Environ::ourmoves(Game const & game, stage_t stage) const
{
    Goal const & goal = game.goal();
    if (goal.ast.empty())
        goal.ast = ast(goal.RPN);
// if (stage >= 5)
// std::cout << "Finding moves at stage " << stage << " for " << game;
    Moves moves;

    Assiters const & assvec = database.assiters();
    Assiters::size_type limit = std::min(assertion.number, prob().numberlimit);
    for (Assiters::size_type i = 1; i < limit; ++i)
    {
        Assertion const & ass = assvec[i]->second;
        if ((ass.type & Asstype::USELESS) || !ontopic(ass))
            continue; // Skip non propositional theorems.
        if (stage == 0 || (ass.nfreevar() > 0 && stage >= ass.nfreevar()))
            if (trythm(game, assvec[i], stage, moves))
                break; // Move closes the goal.
    }
// if (stage >= 5)
// std::cout << moves.size() << " moves found" << std::endl;
    return moves;
}

// Try applying the theorem, and add moves if successful.
// Return true if it has no open hypotheses.
bool Environ::trythm
    (Game const & game, Assiter iter, Proofsize size, Moves & moves) const
{
// std::cout << "Trying " << iter->first << " with " << game.goal().expression();
    Assertion const & thm = iter->second;
    Goal const & goal = game.goal();
    if (thm.expression.empty() || thm.exptypecode() != goal.typecode)
        return false; // Type code mismatch
// std::cout << "Trying " << iter->first << " with " << game.goal().expression();
    Stepranges subst(thm.maxvarid() + 1);
    if (!findsubstitutions(goal, thm.expRPNAST(), subst))
        return size == 0 && thm.esshypcount() == 0 && false
                && addabsmoves(goal, &*iter, moves);

    // Move with all bound substitutions
    Move move(&*iter, subst);
    if (size > 0)
        return thm.nfreevar() > 0 && addhardmoves(move.pthm, size, move, moves);
    else if (thm.nfreevar() > 0)
        return assertion.esshypcount() && addhypmoves(move.pthm, moves, subst);
    else
        return addboundmove(move, moves);
}

// Add a move with only bound substitutions.
// Return true if it has no open hypotheses.
bool Environ::addboundmove(Move const & move, Moves & moves) const
{
    switch (valid(move))
    {
    case MoveCLOSED:
        moves.clear();
        moves.push_back(move);
        return true;
    case MoveVALID:
        // std::cout << move.label() << std::endl;
        // std::cout << move.substitutions;
        moves.push_back(move);
        // std::cin.get();
        return false;
    default:
        return false;
    }
}

// Add abstraction moves. Return true if it has no open hypotheses.
bool Environ::addabsmoves(Goal const & goal, pAss pthm, Moves & moves) const
{
    Assertion const & thm = pthm->second;
    if (thm.expmaxabs.empty())
        return false;

    if (!goal.maxabscomputed)
    {
        goal.maxabs = maxabs(goal.RPN, goal.ast);
        goal.maxabscomputed = true;
    }
    if (goal.maxabs.empty())
        return false;

    Stepranges subst(thm.maxvarid() + 1);

    FOR (GovernedSteprangesbystep::const_reference rstep, thm.expmaxabs)
    {
        GovernedSteprangesbystep::const_iterator const iter
        = goal.maxabs.find(rstep.first);
        if (iter == goal.maxabs.end())
            continue;

        FOR (GovernedStepranges::const_reference thmrange, rstep.second)
        {
            SteprangeAST thmsubexp(thmrange.first, thmrange.second);

            FOR (GovernedStepranges::const_reference goalrange, iter->second)
            {
                SteprangeAST goalsubexp(goalrange.first, goalrange.second);

                subst.assign(thm.maxvarid() + 1, Steprange());
                if (findsubstitutions(goalsubexp, thmsubexp, subst) &&
                    addabsmove(goal, goalrange.first, Move(pthm,subst), moves))
                    return true;
            }
        }
    }
    
    return false;
}

// Add an abstraction move. Return true if it has no open hypotheses.
bool Environ::addabsmove
    (Goal const & goal, Steprange abstraction,
     Move const & move, Moves & moves) const
{
    Goal const & thmgoal(move.goal());
    AST  const & thmgoalast(ast(thmgoal.RPN));

    SteprangeAST thmexp(thmgoal.RPN, thmgoalast), goalexp(goal.RPN, goal.ast);
    Move::Conjectures conjs(2);
    Bank1var const var = pProb->bank.addabsvar(abstraction);

    if (skeleton(thmexp, Keeprange(abstraction), var, conjs[0].RPN) != TRUE)
        return false;
    if (skeleton(goalexp, Keeprange(abstraction), var, conjs[1].RPN) != TRUE)
        return false;

    conjs[0].typecode = thmgoal.typecode;
    conjs[1].typecode = goal.typecode;

    Stepranges subst(var.id + 1);
    subst.back() = abstraction;
    Move const conjmove(conjs, subst);

    switch (validconjmove(conjmove))
    {
    case MoveCLOSED:
        moves.clear();
        moves.push_back(conjmove);
        return true;
    case MoveVALID:
        moves.push_back(conjmove);
        return false;
    default:
        return false;
    }
}

// Return true if all variables in use have been substituted.
static bool allvarsfilled(Varusage const & varusage, Stepranges const & subst)
{
    FOR (Varusage::const_reference rvar, varusage)
        if (Symbol2::ID const id = rvar.first)
            if (subst[id].first == subst[id].second)
                return false;
    return true;
}

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
        if (asshyp < ass.hypcount())
            --delta; // 1 match removed
        for (++asshyp; asshyp <= ass.hypcount(); ++asshyp)
        {
            if (asshyp < ass.hypcount() &&
                (ass.hypfloats(asshyp) ||
                 ass.hyptypecode(asshyp) != thmhyptype))
                continue; // Skip floating hypothesis.
            // Copy the last substitution.
            substack[hypstack.size()] = substack[hypstack.size() - 1];
            if (asshyp == ass.hypcount())
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
    if (substack.empty() || assertion.hypcount() + 1 == 0)
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
        Stepranges const & newsubstitutions = substack[hypstack.size()];
        if (allvarsfilled(thm.varusage, newsubstitutions))
        {
            if (addboundmove(Move(pthm, newsubstitutions), moves))
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
bool Environ::addhypmoves(pAss pthm, Moves & moves,
                          Stepranges const & substitutions) const
{
    Assertion const & thm = pthm->second;
    FOR (Hypsize thmhyp, thm.hypsorder)
    {
        if (thm.nfreevars[thmhyp] < thm.nfreevar())
            return false;
// std::cout << move.label() << ' ' << thm.hyplabel(thmhyp) << ' ';
        strview thmhyptype = thm.hyptypecode(thmhyp);
        for (Hypsize asshyp = 0; asshyp < assertion.hypcount(); ++asshyp)
        {
            if (assertion.hypfloats(asshyp) ||
                assertion.hyptypecode(asshyp) != thmhyptype)
                continue;
            // Match hypothesis asshyp against key hypothesis thmhyp of the theorem.
            Stepranges newsubstitutions(substitutions);
            if (findsubstitutions
                (assertion.hypRPNAST(asshyp), thm.hypRPNAST(thmhyp),
                 newsubstitutions))
            {
// std::cout << assertion.hyplabel(asshyp) << ' ' << assertion.hypexp(asshyp);
                if (addboundmove(Move(pthm, newsubstitutions), moves))
                    return true;
            }
        }
    }
    return false;
}
