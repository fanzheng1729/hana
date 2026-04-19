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
    return addvalidmove<&Environ::valid>(move, moves);
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
    Assiters::size_type const limit
        = std::min(assnum(), prob().numberlimit);
    for (Assiters::size_type i = 1; i < limit; ++i)
    {
        Assertion const & thm = assvec[i]->second;
        if (!thm.testtype(Asstype::USELESS) && ontopic(thm))
            if (stage == 0 ||
                (thm.nfreevar() > 0 && stage >= thm.nfreevar()))
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
        return size == 0 && thm.nEhyps() == 0// && false
                && addabsmoves(goal, &*iter, moves);

    // Move with all bound substitutions
    Move move(&*iter, subst);
    if (size > 0)
        return thm.nfreevar() > 0 && addhardmoves(move.pthm, size, move, moves);
    else if (thm.nfreevar() > 0)
        return assertion.nEhyps() > 0 && addhypmoves(move.pthm, moves, subst);
    else
        return addboundmove(move, moves);
}

// Add abstraction moves. Return true if it has no open hypotheses.
bool Environ::addabsmoves(Goal const & goal, pAss pthm, Moves & moves) const
{
    Assertion const & thm = pthm->second;
    if (thm.expmaxabs.empty())
        return false;

    goal.fillmaxabs();
    if (goal.maxabs.empty())
        return false;

    RPNspans subst(thm.maxvarid() + 1);

    FOR (GovernedRPNspansbystep::const_reference rstep, thm.expmaxabs)
    {
        GovernedRPNspansbystep::const_iterator const iter
        = goal.maxabs.find(rstep.first);
        if (iter == goal.maxabs.end())
            continue;

        FOR (GovernedRPNspans::const_reference thmspan, rstep.second)
        {
            RPNspanAST thmsubexp(thmspan.first, thmspan.second);

            FOR (GovernedRPNspans::const_reference goalspan, iter->second)
            {
                RPNspanAST goalsubexp(goalspan.first, goalspan.second);

                subst.assign(thm.maxvarid() + 1, RPNspan());
                if (findsubst(goalsubexp, thmsubexp, subst) &&
                    addabsmove(goal, goalspan.first, Move(pthm, subst), moves))
                    return true;
            }
        }
    }
    
    return false;
}

// Add an abstraction move. Return true if it has no open hypotheses.
bool Environ::addabsmove
    (Goal const & goal, RPNspan absRPN,
     Move const & move, Moves & moves) const
{
    if (absRPN.empty())
        return false;

    Goal const & thmgoal(move.goal());
    AST  const & thmgoalast(ast(thmgoal.rpn));
    RPNspanAST   thmexp(thmgoal.rpn, thmgoalast), goalexp(goal.rpn, goal.ast);
    // Abstract variable
    Bank1var const absvar = pProb->bank.addabsvar(absRPN);
    // 1 conjecture + 1 goal
    Move::Conjectures conjs(2);
    if (skeleton(thmexp, Keepspan(absRPN), absvar, conjs[0].rpn) != TRUE)
        return false;
    if (skeleton(goalexp, Keepspan(absRPN), absvar, conjs[1].rpn) != TRUE)
        return false;
    conjs[0].typecode = thmgoal.typecode;
    conjs[1].typecode = goal.typecode;
    // Abstractions of abstract variables
    RPNspans absRPNs(absvar.id + 1);
    absRPNs.back() = absRPN;
    return addconjmove(Move(conjs, absRPNs), moves);
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
            {
// std::cout << assertion.hyplabel(asshyp) << ' ' << assertion.hypexp(asshyp);
                if (addboundmove(Move(pthm, newsubsts), moves))
                    return true;
            }
        }
    }
    return false;
}
