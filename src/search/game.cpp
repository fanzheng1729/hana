#include <algorithm>    // for std::remove_copy_if and std::includes
#include <functional>   // for std::logical_not
#include "environ.h"
#include "game.h"
#include "goaldata.h"
#include "../io.h"
#include "../util/iter.h"
#include "../proof/write.h"

Goal const & Game::goal() const { return goalptr->first; }
Goaldata & Game::goaldata() const { return goalptr->second; }

std::ostream & operator<<(std::ostream & out, Game const & game)
{
    out << game.goal().expression();
    if (game.attempt.type != Move::NONE)
        out << "Proof attempt (" << game.ndefer << ") "
            << game.attempt << std::endl;
    return out;
}

// Return true if a move is legal.
bool Game::legal(Move const & move, bool ourturn) const
{
    if (ourturn && move.type == Move::ASS) // Check if the goal matches.
        return goal().RPN == move.expRPN() &&
                goal().typecode == move.exptypecode();
    if (!ourturn && attempt.type == Move::ASS) // Check index bound.
        return move.index < attempt.hypcount();
    return true;
}

// Play a move.
Game Game::play(Move const & move, bool ourturn) const
{
    Game game(goalptr, penv, ndefer);

    if (ourturn)
    {
        // On our turn, record the move.
        game.attempt = move;
        game.ndefer = (move.type == Move::DEFER) * (ndefer + 1);
    }
    else if (attempt.type == Move::ASS)
    {
        // On thr turn, pick the hypothesis.
        game.goalptr = attempt.hypvec[move.index];
    }

    return game;
}

// Their moves correspond to essential hypotheses.
Moves Game::theirmoves() const
{
// std::cout << "Finding thr moves ";
    if (attempt.type == Move::ASS)
    {
        Moves result;
        result.reserve(attempt.esshypcount());
// std::cout << "for " << this << ' ' << attempt.pass->first << ": ";
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
    if (goaldata().proven())
        return Moves();
    if (env().staged)
        return env().ourmoves(*this, stage);
// std::cout << "with ndefer " << ndefer << ' ';
    Moves moves(env().ourmoves(*this, ndefer));
    moves.push_back(Move::DEFER);

    return moves;
}

// Add proof for a node using a theorem.
// Return true if a new proof is written.
bool Game::writeproof() const
{
    if (attempt.type == Move::NONE || attempt.type == Move::DEFER)
        return false;
    // attempt.type == Move::ASS
    // Pointers to proofs of hypotheses
    pProofs hyps(attempt.hypcount());
    for (Hypsize i = 0; i < attempt.hypcount(); ++i)
    {
        hyps[i] = attempt.hypfloats(i) ?
            &attempt.substitutions[attempt.hypexp(i)[1]] :
            &attempt.hypvec[i]->second.proofsteps;
// std::cout << "Added hyp\n" << *hyps.back();
    }
    // The whose proof
    Proofsteps & steps = goaldata().proofsteps;
    if (!::writeproof(steps, attempt.pass, hyps))
        return false;
    // Verification
    Expression const & exp(verify(steps));
    bool const okay = exp == goal().expression();
    if (!okay)
    {
        std::cerr << "In attempt to use " << attempt << ", the proof steps\n";
        std::cerr << steps << "proves\n" << exp;
        std::cerr << "instead of\n" << goal().expression();
        std::cerr << "Proofs of hypotheses are" << std::endl;
        for (Hypsize i = 0; i < attempt.hypcount(); ++i)
        {
            Proofsteps const & steps = *hyps[i];
            std::cerr << attempt.hyplabel(i) << '\t' << steps;
        }
        steps.clear();
    }
    return okay;
}
