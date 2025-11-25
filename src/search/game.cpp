#include <algorithm>    // for std::remove_copy_if and std::includes
#include <functional>   // for std::logical_not
#include "environ.h"
#include "game.h"
#include "goaldata.h"
#include "../io.h"
#include "../util/iter.h"
#include "../proof/write.h"

Goaldata & Game::goaldata() const { return pGoal->second; }
Goaldata & Game::goaldata2() const { return pGoaldata->second; }
Goal const & Game::goal() const { return goaldata2().pBigGoal->first; }
Proofsteps & Game::proof() const { return goaldata().proof; }
Proofsteps & Game::proof2() const { return goaldata2().proof; }
Environ const * Game::pEnv() const { return pGoaldata->first; }

std::ostream & operator<<(std::ostream & out, Game const & game)
{
    out << game.goal().expression();
    if (game.proven())
        out << "Proof: " << game.proof();
    if (game.attempt.type != Move::NONE)
        out << "Proof attempt (" << game.nDefer << ") "
            << game.attempt << std::endl;
    return out;
}

// Return true if a move is legal.
bool Game::legal(Move const & move, bool ourturn) const
{
    if (ourturn && move.type == Move::THM) // Check if the goal matches.
        return goal().RPN == move.expRPN() &&
                goal().typecode == move.exptypecode();
    if (!ourturn && attempt.type == Move::THM) // Check index bound.
        return move.index < attempt.hypcount();
    return true;
}

// Play a move.
Game Game::play(Move const & move, bool ourturn) const
{
    Game game(cheapcopy());

    if (ourturn) // Record the move.
    {
        game.attempt = move;
        game.nDefer = (move.type == Move::DEFER) * (nDefer + 1);
    }
    else if (attempt.type == Move::THM) // Pick the hypothesis.
    {
        game.pGoal = attempt.hypvec[move.index];
        game.pGoaldata = attempt.hypvec2[move.index];
    }

    return game;
}

// Their moves correspond to essential hypotheses.
Moves Game::theirmoves() const
{
// std::cout << "Finding thr moves ";
    if (attempt.type == Move::THM)
    {
        Moves result;
        result.reserve(attempt.esshypcount());
// std::cout << "for " << attempt.label() << ": ";
        for (Hypsize i = 0; i < attempt.hypcount(); ++i)
            if (!attempt.hypfloats(i))
                result.push_back(i);
// std::cout << result.size() << " moves added" << std::endl;
        return result;
    }
    return Moves(attempt.type == Move::DEFER, Move::DEFER);
}

// Our moves are supplied by the environment.
Moves Game::ourmoves(stage_t stage) const
{
// std::cout << "Finding moves at stage " << stage << " for " << *this;
    if (!pEnv() || proven())
        return Moves();
    if (env().staged)
        return env().ourmoves(*this, stage);
// std::cout << "with nDefer " << nDefer << ' ';
    Moves moves(env().ourmoves(*this, nDefer));
    moves.push_back(Move::DEFER);

    return moves;
}

// Add proof for a node using a theorem.
// Return true if a new proof is written.
bool Game::writeproof() const
{
    if (attempt.type == Move::NONE || proven())
        return false;
    // attempt.type == Move::THM, goal not proven
    if (!pEnv() || !checkDV(attempt, env().assertion, true))
        return false;
// std::cout << "Writing proof: " << goal().expression();
    // Pointers to proofs of hypotheses
    pProofs hyps(attempt.hypcount());
    for (Hypsize i = 0; i < attempt.hypcount(); ++i)
    {
        hyps[i] = attempt.hypfloats(i) ?
            &attempt.substitutions[attempt.hypexp(i)[1]] :
            &attempt.hypvec[i]->second.proof;
// std::cout << "Added hyp\n" << *hyps.back();
    }
    // The whose proof
    if (!::writeproof(proof2(), attempt.pthm, hyps))
        return false;
    // Verification
    const Expression & exp(verify(proof2()));
    const bool okay = (exp == goal().expression());
    if (okay)
    {
        pGoal->second.status = pGoaldata->second.status = GOALTRUE;
        proof() = proof2();
    }
    else
    {
        void writeprooferr(Game const & game, Expression const & exp, pProofs const & hyps);
        writeprooferr(*this, exp, hyps);
        proof().clear();
    }
    return okay;
}

void writeprooferr(Game const & game, Expression const & exp, pProofs const & hyps)
{
    std::cerr << "In attempt to use " << game.attempt << ", the proof\n";
    std::cerr << game.proof() << "proves\n" << exp;
    std::cerr << "instead of\n" << game.goal().expression();
    std::cerr << "Proofs of hypotheses are" << std::endl;
    for (Hypsize i = 0; i < game.attempt.hypcount(); ++i)
    {
        Proofsteps const & steps = *hyps[i];
        std::cerr << game.attempt.hyplabel(i) << '\t' << steps;
    }
}
