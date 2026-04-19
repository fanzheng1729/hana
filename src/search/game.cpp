#include <algorithm>    // for std::lower_bound
#include "game.h"
#include "goaldata.h"
#include "problem.h"
#include "../io.h"

Goaldata & Game::goaldata() const { return pgoal->second; }
Goaldatas & Game::goaldatas() const { return goaldata().goaldatas(); }
Goal const & Game::goal() const { return goaldata().goal(); }
RPN const & Game::proof() const { return goaldata().proofsrc(); }
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
        return move.index < attempt.nsubgoals();
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
        game.pgoal = static_cast<pGoal>(attempt.subgoals[move.index]);

    return game;
}

// Their moves correspond to essential hypotheses.
Moves Game::theirmoves() const
{
    Moves result;
    result.reserve(attempt.nEsubgoals());

    for (Hypsize i = 0; i < attempt.nsubgoals(); ++i)
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
    for (Hypsize i = 0; i < move.nsubgoals(); ++i)
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
    RPN * pProof = &goaldata().proofdst();
    // Pointers to proofs of sub-goals
    pProofs const phyps(attempt.phyps());
    // Write proof.
    if (!attempt.writeproof(*pProof, phyps))
        return false;
    if (false && pProof != &goaldatas().proof)
        if (env().prob().probEnv().legal(*pProof))
        {
            // Proof works in problem context. Move it there.
            goaldatas().proof.swap(*pProof);
            pProof = &goaldatas().proof;
        }
        else
        if (false)
        {
            pGoal pcurgoal = pgoal;
            bool found;
            do
            {
                found = false;
                pEnvs const & psubEnvs = pcurgoal->first->psubEnvs();
                pEnvs::const_iterator subiter = psubEnvs.begin();
                pEnvs::const_iterator const subend = psubEnvs.end();

                // Loop through sub-contexts.
                FOR (Goaldatas::reference goaldata, goaldatas())
                    if (!goaldata.first->subsumedbyProb())
                    {
                        Environ const & subEnv = *goaldata.first;
                        subiter = std::lower_bound(subiter, subend, &subEnv);
                        // while (subiter != subend && less(*subiter, &subEnv))
                        //     ++subiter;
                        if (subiter == subend)
                            break;  // end reached
                        if (*subiter == &subEnv &&
                            subEnv.legal(*pProof))
                        {
                            // Proof holds in sub-context.
                            pcurgoal = &goaldata;
                            found = true;
                            break;
                        }
                    }
            } while (found);
            if (pgoal != pcurgoal)
            {
                RPN * pdest = &pcurgoal->second.proofdst();
                pdest->swap(*pProof);
                pProof = pdest;
            }
        }
    // Verification
    const Expression & exp(verify(*pProof));
    const bool okay = (exp == goal().expression());
    if (!okay)
        printthmhypproofs(attempt, phyps);
    if (okay)
    {
// std::cout << "Built proof for " << goal().expression();
        goaldata().settrue();
    }
    else
    {
        writeprooferr(exp);
        pProof->clear();
    }
    return okay;
}

void Game::writeprooferr(Expression const & exp) const
{
    std::cerr << "When using " << attempt.label() << ", the proof\n";
    std::cerr << proof() << "proves\n" << exp;
    std::cerr << "instead of\n" << goal().expression();
}
