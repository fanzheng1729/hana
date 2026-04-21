#include <algorithm>    // for std::min
#include "environ.h"
#include "problem.h"
#include "../proof/skeleton.h"

// Add a move with validation. Return true if it has no open hypotheses.
template<Environ::MoveValidity (Environ::*Validator)(Move const &) const>
bool Environ::addvalidmove(Move const & move, Moves & moves) const
{
    switch ((this->*Validator)(move))
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
bool Environ::addboundmove(Move const & move, Moves & moves) const
{
    return addvalidmove<&Environ::validthmmove>(move, moves);
}
bool Environ::addconjmove(Move const & move, Moves & moves) const
{
    return addvalidmove<&Environ::validconjmove>(move, moves);
}

// Moves generated at a given stage
Moves Environ::ourmoves(Game const & game, stage_t stage) const
{
    game.goal().fillast();
// if (stage >= 5)
// std::cout << "Finding moves at stage " << stage << " for " << game;
    Moves moves;

    Assiters const & assvec = database.assiters();
    Assiters::size_type & limit = pProb->numberlimit;
    if (limit > assnum()) limit = assnum();
    for (Assiters::size_type i = 1; i < limit; ++i)
    {
        Assertion const & thm = assvec[i]->second;
        if (!thm.testtype(Asstype::USELESS) && ontopic(thm))
            if (stage == 0 ||
                (thm.nfreevar() > 0 && stage >= thm.nfreevar()))
                if (trythm(game, assvec[i], stage, moves))
                    return moves; // Move closes the goal.
    }
// if (stage >= 5)
// std::cout << moves.size() << " moves found" << std::endl;
    if (stage == 0)
        addabsmoves(game.goal(), moves);
    return moves;
}

// Try applying the theorem, and add moves if successful.
// Return true if it has no open hypotheses.
bool Environ::trythm
    (Game const & game, Assiter iter, RPNsize size, Moves & moves) const
{
// std::cout << "Trying " << iter->first << " with " << game.goal().expression();
    Assertion const & thm = iter->second;
    Goal const & goal = game.goal();
    if (thm.expression.empty() || thm.exptypecode() != goal.typecode)
        return false; // Type code mismatch
// std::cout << "Trying " << iter->first << " with " << game.goal().expression();
    RPNspans subst(thm.maxvarid() + 1);
    if (!findsubst(goal, thm.expRPNAST(), subst))
        return false;
    // Move with all bound substitutions
    Move move(&*iter, subst);
    if (size > 0)
        return thm.nfreevar() > 0 && addhardmoves(move, size, moves);
    else if (thm.nfreevar() > 0)
        return assertion.nEhyps() > 0 && addhypmoves(move.pthm, moves, subst);
    else
        return addboundmove(move, moves);
}

// Add Hypothesis-oriented moves.
// Return true if it has no open hypotheses.
bool Environ::addhypmoves(pAss pthm, Moves & moves,
                          RPNspans const & substs) const
{
    Assertion const & thm = pthm->second;
    FOR (Hypsize thmhyp, thm.hypsorder)
    {
        if (thm.nfreevars[thmhyp] < thm.nfreevar())
            return false;
// std::cout << move.label() << ' ' << thm.hyplabel(thmhyp) << ' ';
        strview thmhyptype = thm.hyptypecode(thmhyp);
        for (Hypsize asshyp = 0; asshyp < nhyps; ++asshyp)
        {
            if (assertion.hypfloats(asshyp) ||
                assertion.hyptypecode(asshyp) != thmhyptype)
                continue;
            // Match hypothesis asshyp against key hypothesis thmhyp of the theorem.
            RPNspans newsubsts(substs);
            if (findsubst
                (assertion.hypRPNAST(asshyp), thm.hypRPNAST(thmhyp), newsubsts))
// std::cout << assertion.hyplabel(asshyp) << ' ' << assertion.hypexp(asshyp),
                if (addboundmove(Move(pthm, newsubsts), moves))
                    return true;
        }
    }
    return false;
}

// Add abstraction moves. Return true if it has no open hypotheses.
bool Environ::addabsmoves(Goal const & goal, Moves & moves) const
{
    goal.fillmaxabs();
    FOR (GovernedRPNspansbystep::const_reference rstep, goal.maxabs)
        FOR (GovernedRPNspans::const_reference subexp, rstep.second)
            if (addabsmoves(goal, RPNspanAST(subexp.first, subexp.second), moves))
                return true;
    return false;
}
bool Environ::addabsmoves
    (Goal const & goal, RPNspanAST const subexp, Moves & moves) const
{
    std::pair<Problem::Abstractions::iterator, bool> const result =
    pProb->abstractions.insert(std::make_pair(subexp.first, Moves()));
    Moves & substs = result.first->second;
    if (result.second) // New sub-expression
        substs = absubsts(subexp);
    return addabsmoves(goal, subexp, substs, moves);
}
bool Environ::addabsmoves
    (Goal const & goal, RPNspanAST const subexp,
    Moves const & absubsts, Moves & moves) const
{
    FOR (Move const & absubst, absubsts)
    {
        Goal const & conj = absubst.goal();
        if (goal != conj &&
            addconjmove(pProb->absmove(goal, conj, subexp), moves))
            return true;
    }
    return false;
}

static void addabsubst
    (RPNspanAST const subexp, pAss pthm, Moves & moves)
{
    Assertion const & thm = pthm->second;
    RPNspans subst(thm.maxvarid() + 1);
    GovernedRPNspansbystep::const_iterator const iter =
    thm.expmaxabs.find(subexp.first.root());
    if (iter == thm.expmaxabs.end())
        return;

    moves.reserve(moves.size() + iter->second.size());
    FOR (GovernedRPNspans::const_reference thmabs, iter->second)
    {
        subst.assign(thm.maxvarid() + 1, RPNspan());
        if (findsubst(subexp, RPNspanAST(thmabs.first, thmabs.second), subst))
            moves.push_back(Move(pthm, subst));
    }
}

// Return abstraction substitutions.
Moves Environ::absubsts(RPNspanAST const subexp) const
{
    Moves moves;
    Assiters const & assvec = database.assiters();
    for (Assiters::size_type i = 1; i < prob().numberlimit; ++i)
    {
        Assertion const & thm = assvec[i]->second;
        if (!thm.testtype(Asstype::USELESS) && thm.nEhyps() == 0
            && ontopic(thm))
            addabsubst(subexp, &*assvec[i], moves);
    }

    return moves;
}
