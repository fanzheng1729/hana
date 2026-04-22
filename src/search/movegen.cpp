#include <algorithm>    // for std::min
#include "environ.h"
#include "problem.h"
#include "../io.h"
#include "../proof/skeleton.h"

// Add a move with validation. Return true if it has no open hypotheses.
template<Environ::MoveValidity (Environ::*Validator)(Move const &) const>
bool Environ::addvalidmove(Move const & move, Moves & moves) const
{
    if (!move.isthmorconj())
        return false;
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
            if (addabsmoves(goal, subexp, moves))
                return true;
    return false;
}
bool Environ::addabsmoves
    (Goal const & goal, RPNspanAST const subexp, Moves & moves) const
{
    if (subexp.empty())
        return false;

    std::pair<Problem::Abstractions::iterator, bool> const result =
    pProb->abstractions.insert(std::make_pair(subexp.first, Absubstmoves()));
    Absubstmoves & absubstmoves = result.first->second;
    if (result.second) // New sub-expression
        absubstmoves = absubsts(subexp);

    FOR (Absubstmove const & absubstmove, absubstmoves)
// std::cout << absubstmove.first.pthm->first << std::endl,
        if (addconjmove(pProb->absmove(goal, absubstmove, subexp), moves))
            return true;
    return false;
}

static void addabsubst
    (RPNspanAST const subexp, pAss const pthm,
     Absubstmoves & absubstmoves)
{
    Assertion const & thm = pthm->second;
    RPNspans subst(thm.maxvarid() + 1);
    GovernedRPNspansbystep::const_iterator const iter =
    thm.expmaxabs.find(subexp.first.root());
    if (iter == thm.expmaxabs.end())
        return;

    absubstmoves.reserve(absubstmoves.size() + iter->second.size());
    FOR (GovernedRPNspans::const_reference thmabs, iter->second)
    {
        subst.assign(subst.size(), RPNspan());
        RPNspanAST thmabsAST(thmabs.first, thmabs.second);
        if (findsubst(subexp, thmabs, subst))
        {
            absubstmoves.push_back(std::make_pair(Move(pthm, subst), RPN()));
            Move const & move = absubstmoves.back().first;
            Goal const & conj = move.goal();
            conj.fillast();
            RPN & rpn = absubstmoves.back().second;
            skeleton(conj, Keepspan(subexp.first), NullBank(), rpn);
            // std::cout << thm.expression;
            // std::cout << conj.expression();
            // std::cout << subexp.first;
            // std::cout << rpn.size();
            // std::cin.get();
        }
    }
}

// Abstraction-substitutions for a sub-expression
Absubstmoves Environ::absubsts(RPNspanAST const subexp) const
{
    Absubstmoves moves;
    Assiters const & assvec = database.assiters();
    for (Assiters::size_type i = 1; i < prob().numberlimit; ++i)
    {
        Assertion const & thm = assvec[i]->second;
        if (!thm.testtype(Asstype::USELESS) && thm.nEhyps() == 0
            && database.typecodes().isprimitive(thm.exptypecode()) == FALSE
            && ontopic(thm))
            addabsubst(subexp, &*assvec[i], moves);
    }
    return moves;
}
