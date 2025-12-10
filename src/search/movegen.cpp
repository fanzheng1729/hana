#include <algorithm>    // for std::min
#include "../proof/analyze.h"
#include "environ.h"
#include "problem.h"

// Moves generated at a given stage
Moves Environ::ourmoves(Game const & game, stage_t stage) const
{
    if (game.goal().ast.empty())
        game.goal().ast = ast(game.goal().RPN);
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
        return size == 0 && thm.esshypcount() == 0// && false
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

