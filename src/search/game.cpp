#include "environ.h"
#include "game.h"
#include "goaldata.h"
#include "../io.h"
#include "problem.h"
#include "../proof/write.h"

Goaldata & Game::goaldata() const { return pgoal->second; }
Goaldatas & Game::goaldatas() const { return goaldata().goaldatas(); }
Goal const & Game::goal() const { return goaldata().goal(); }
Proofsteps const & Game::proof() const { return goaldata().proofsrc(); }
Environ const & Game::env() const { return *pgoal->first; }

std::ostream & operator<<(std::ostream & out, Game const & game)
{
    out << game.goal().expression();
    if (game.proven())
        out << "Proof: " << game.proof();
    if (game.attempt.type != Move::NONE)
        out << "Proof attempt (" << game.nDefer << ") "
            << game.attempt.label() << std::endl;
    return out;
}

// Return true if a move is legal.
bool Game::legal(Move const & move, bool ourturn) const
{
    if (move.isdefer())
        return true;
    if (ourturn && (move.isthm() || move.isconj()))
        return goal() == move.goal();
    if (!ourturn && (attempt.isthm())) // Check index bound.
        return move.index < attempt.subgoalcount();
    return true;
}

// Play a move.
Game Game::play(Move const & move, bool ourturn) const
{
    Game game(pgoal, nDefer);

    if (ourturn) // Record the move.
    {
        game.attempt = move;
        game.nDefer = move.isdefer() * (nDefer + 1);
    }
    else if (attempt.isthm() || attempt.isconj()) // Pick the hyp.
        game.pgoal = attempt.subgoals[move.index];

    return game;
}

// Their moves correspond to essential hypotheses.
Moves Game::theirmoves() const
{
    Moves result;
    result.reserve(attempt.esssubgoalcount());

    for (Hypsize i = 0; i < attempt.subgoalcount(); ++i)
        if (!attempt.subgoalfloats(i))
            result.push_back(i);

    return result;
}

// Our moves are supplied by the environment.
Moves Game::ourmoves(stage_t stage) const
{
// std::cout << "Finding moves at stage " << stage << " for " << *this;
    if (proven())
        return Moves();
    if (env().staged)
        return env().ourmoves(*this, stage);
// std::cout << "with nDefer " << nDefer << ' ';
    Moves moves(env().ourmoves(*this, nDefer));
    moves.push_back(Move::DEFER);

    return moves;
}

static void writeprooferr
    (Game const & game, Expression const & exp, pProofs const & hyps)
{
    std::cerr << "When using " << game.attempt.label() << ", the proof\n";
    std::cerr << game.proof() << "proves\n" << exp;
    std::cerr << "instead of\n" << game.goal().expression();
    std::cerr << "Proofs of hypotheses are" << std::endl;
    for (Hypsize i = 0; i < game.attempt.subgoalcount(); ++i)
        std::cerr << game.attempt.subgoallabel(i) << '\t' << *hyps[i];
}

// Add proof for a node using a theorem.
// Return true if a new proof is written.
bool Game::writeproof() const
{
    if (attempt.type == Move::NONE || proven())
        return false;
    if (!attempt.checkDV(env().assertion, true))
        return false;
// std::cout << "Writing proof: " << goal().expression();
    if (attempt.isconj())
    {
        std::cout << attempt.subgoals[0]->second.goal().expression();
        std::cout << attempt.subgoals[0]->second.proofsrc();
        std::cout << attempt.subgoals[1]->second.goal().expression();
        std::cout << attempt.subgoals[1]->second.proofsrc();
        std::cout << goal().expression();
        Environ const & env = *attempt.subgoals[1]->first;
        std::cout << env.name << std::endl;
        FOR (Hypiter iter, env.assertion.hypiters)
            std::cout << iter->first << ' ';
        std::cout << "Not imp writeproof" << std::endl, throw;
    }
    // attempt.type == Move::THM, goal not proven
    Proofsteps & dest = goaldata().proofdst();
    // Pointers to proofs of hypotheses
    pProofs hyps(attempt.hypcount());
    for (Hypsize i = 0; i < hyps.size(); ++i)
    {
        if (attempt.hypfloats(i))
            hyps[i] = &attempt.substitutions[attempt.hypvar(i)];
        else
            hyps[i] = &attempt.subgoals[i]->second.proofsrc();
// std::cout << "Added hyp\n" << *hyps[i];
    }
    if (!::writeproof(dest, attempt.pthm, hyps))
        return false;
    // Verification
    const Expression & exp(verify(dest));
    const bool okay = (exp == goal().expression());
    if (okay)
    {
// std::cout << "Built proof for " << goal().expression();
        goaldata().setstatustrue();
    }
    else
    {
        writeprooferr(*this, exp, hyps);
        dest.clear();
    }
    return okay;
}
