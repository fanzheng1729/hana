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

static void printthmhypproofs(Move const & move, pProofs const & phyps)
{
    std::cerr << "Proofs of hypotheses are" << std::endl;
    for (Hypsize i = 0; i < move.subgoalcount(); ++i)
        std::cerr << move.subgoallabel(i) << '\t' << *phyps[i];
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
    // attempt.type == Move::THM || Move::CONJ, goal not proven
    Proofsteps & dest = goaldata().proofdst();
    // Return pointers to proofs of sub-goals
    pProofs phyps(attempt.subgoalcount());
    for (Hypsize i = 0; i < phyps.size(); ++i)
        phyps[i] = attempt.psubgoalproof(i);
    if (attempt.isconj())
    {
        std::cout << attempt.absconjs[0].expression();
        std::cout << attempt.subgoals[0]->second.goal().expression();
        std::cout << *phyps[0];
        std::cout << attempt.absconjs[1].expression();
        std::cout << attempt.subgoals[1]->second.goal().expression();
        std::cout << *phyps[1];
        std::cout << goal().expression();
        std::cout << *phyps.back();
        std::cout << attempt.fullproofsize(phyps);
        std::cout << "Not imp writeproof" << std::endl, throw;
    }
    if (!::writeproof(dest, attempt.pthm, phyps))
        return false;
    // Verification
    const Expression & exp(verify(dest));
    const bool okay = (exp == goal().expression());
    if (!okay)
        printthmhypproofs(attempt, phyps);
    if (okay)
    {
// std::cout << "Built proof for " << goal().expression();
        goaldata().setstatustrue();
    }
    else
    {
        writeprooferr(exp);
        dest.clear();
    }
    return okay;
}

void Game::writeprooferr(Expression const & exp) const
{
    std::cerr << "When using " << attempt.label() << ", the proof\n";
    std::cerr << proof() << "proves\n" << exp;
    std::cerr << "instead of\n" << goal().expression();
}
