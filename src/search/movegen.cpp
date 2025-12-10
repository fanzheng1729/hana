#include <algorithm>    // for std::min
#include "../bank.h"
#include "../proof/analyze.h"
#include "../proof/skeleton.h"
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

// Add abstraction moves. Return true if it has no open hypotheses.
bool Environ::addabsmoves(Goal const & goal, pAss pthm, Moves & moves) const
{
    Assertion const & thm = pthm->second;
    if (!goal.maxabscomputed)
        goal.maxabs = maxabs(goal.RPN, goal.ast);

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

    SteprangeAST thmexp(thmgoal.RPN, thmgoalast);
    SteprangeAST goalexp(goal.RPN, goal.ast);

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

