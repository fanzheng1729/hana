#include <algorithm>    // for std::min
#include "environ.h"
#include "problem.h"
#include "../io.h"
#include "../proof/skeleton.h"

// Prepare term generator
void Environ::prepareGen() const
{
    if (!pProb || !syntaxioms.empty())
        return;
    // Relevant syntax axioms
    FOR (Syntaxioms::const_reference syntaxiom, prob().database().syntaxioms())
        if (syntaxiom.second.pass->second.number < assnum())
            if (ontopic(syntaxiom.second.pass->second))
                syntaxioms.insert(syntaxiom);
    // std::cout << syntaxioms, std::cin.get();
}

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
    if (!pProb)
        return Moves();
    Goal const & goal = game.goal();
    // Check goal type code.
    Problem::Theorempool::const_iterator iter =
    prob().theorempool.find(goal.typecode);
    if (iter == prob().theorempool.end())
        return Moves();
    Assiters const & assvec = iter->second;
    // Fill AST of goal.
    goal.fillast();
    // Adjust assertion # limit.
    nAss & limit = pProb->numberlimit;
    if (limit > assnum())
        limit = assnum();
    // Prepare term generator
    if (!pProb) throw;
    prepareGen();

    Moves moves;
    FOR (Assiter iter, assvec)
    {
        Assertion const & ass = iter->second;
        if (ass.number >= limit) break;
        if (ontopic(ass))
            if (stage == 0 ||
                (ass.nfreevar() > 0 && stage >= ass.nfreevar()))
                if (tryass(game, iter, stage, moves))
                    return moves; // Move closes the goal.
    }
    if (stage == 0)
        addabsmoves(game.goal(), moves);
    return moves;
}

// Try applying an assertion, and add moves if okay.
// Return true if it has no open hypotheses.
bool Environ::tryass
    (Game const & game, Assiter iter, RPNsize size, Moves & moves) const
{
    Assertion const & ass = iter->second;
    Goal const & goal = game.goal();
// std::cout << "Trying " << iter->first << " with " << goal.expression();
    RPNspans subst(ass.maxvarid + 1);
    if (!findsubst(goal, ass.expRPNAST(), subst))
        return false;
    // Move with all bound substitutions
    Move move(&*iter, subst);
    if (size > 0)
        return ass.nfreevar() > 0 && addhardmoves(move, size, moves);
    else if (ass.nfreevar() > 0)
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
    if (!goal.maxabsfilled)
    {
        goal.fillast();
        goal.maxabs = maxabs(goal.rpn, goal.ast);
        goal.maxabsfilled = true;
    }
    FOR (GovernedRPNspansbystep::const_reference rstep, goal.maxabs)
    {
// std::cout << rstep.second.size() << std::endl,
// std::cout << rstep.second;
        FOR (GovernedRPNspans::const_reference subexp, rstep.second)
            if (addabsmoves(goal, subexp, moves))
                return true;
    }
    return false;
}
bool Environ::addabsmoves
    (Goal const & goal, RPNspanAST subexp, Moves & moves) const
{
    if (!pProb || subexp.empty())
        return false;
// if (prob().nplays() == 8)
// std::cout << subexp.first;
    std::pair<Problem::Abstractions::iterator, bool> const result =
    pProb->abstractions.insert(std::make_pair(subexp.first, Absubstmoves()));
    Absubstmoves & absubstmoves = result.first->second;
    if (result.second) // New sub-expression
        absubstmoves = absubsts(subexp);

    FOR (Absubstmove const & absubstmove, absubstmoves)
// std::cout << absubstmove.first.pthm->first << std::endl,
        if (usableasconj(absubstmove.first.pthm->second) &&
            addconjmove(pProb->absmove(goal, absubstmove, subexp), moves))
            return true;
    return false;
}

static void addabsubst
    (RPNspanAST subexp, Symbol3 absvar, pAss pass,
     Absubstmoves & absubstmoves)
{
    Assertion const & ass = pass->second;
    RPNspans subst(ass.maxvarid + 1);
    GovernedRPNspansbystep::const_iterator const iter =
    ass.expmaxabs.find(subexp.first.root());
    if (iter == ass.expmaxabs.end())
        return;

    absubstmoves.reserve(absubstmoves.size() + iter->second.size());
    FOR (GovernedRPNspans::const_reference thmabs, iter->second)
    {
        subst.assign(subst.size(), RPNspan());
        RPNspanAST thmabsAST(thmabs.first, thmabs.second);
        if (findsubst(subexp, thmabs, subst))
        {
            absubstmoves.push_back(std::make_pair(Move(pass, subst), RPN()));
            Move const & move = absubstmoves.back().first;
            Goal const & conj = move.goal();
            conj.fillast();
            RPN & rpn = absubstmoves.back().second;
            skeleton(conj, Keepspan(subexp.first), Bank1var(absvar), rpn);
            // std::cout << ass.expression;
            // std::cout << conj.expression();
            // std::cout << subexp.first;
            // std::cout << rpn;
            // std::cin.get();
        }
    }
}

// Abstraction-substitutions for a sub-expression
Absubstmoves Environ::absubsts(RPNspanAST subexp) const
{
    Absubstmoves moves;

    Assiters const & assvec = prob().database().assiters();
    for (nAss i = 1; i < prob().numberlimit; ++i)
    {
        Assiter const iter = assvec[i];
        if (usableasconj(iter->second))
            addabsubst(subexp, pProb->bank.addabsvar(subexp.first), &*iter, moves);
    }

    return moves;
}
