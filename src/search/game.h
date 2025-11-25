#ifndef GAME_H_INCLUDED
#define GAME_H_INCLUDED

#include "move.h"

// Stage
typedef std::size_t stage_t;

// Game state in proof search tree
struct Game
{
    typedef ::Move Move;
    typedef ::Moves Moves;
    // Pointer to goal to be proven
    Goaldataptr pGoal;
    // # defers to the game
    stage_t nDefer;
    // Proof attempt made, on their turn
    Move attempt;
    Game(Goaldataptr p = NULL) : pGoal(p), nDefer(0) {}
    // Does not copy the attempt.
    Game cheapcopy() const
    {
        Game other;
        other.pGoal = pGoal;
        other.nDefer = nDefer;
        return other;
    }
    Goaldata & goaldata() const;
    Goal const & goal() const;
    Proofsteps & proof() const;
    bool proven() const { return !proof().empty(); }
    Environ const * pEnv() const;
    Environ const & env() const { return *pEnv(); }
    friend std::ostream & operator<<(std::ostream & out, Game const & game);
    // Return true if a move is legal.
    bool legal(Move const & move, bool ourturn) const;
    // Play a move.
    Game play(Move const & move, bool ourturn) const;
    // Their moves correspond to essential hypotheses.
    Moves theirmoves() const;
    // Our moves are supplied by the environment.
    Moves ourmoves(stage_t stage) const;
    Moves moves(bool ourturn, stage_t stage) const
    {
        return ourturn ? ourmoves(stage) :
            stage == 0 ? theirmoves() : Moves();
    }
    // Add proof for a node using a theorem.
    // Return true if a new proof is written.
    bool writeproof() const;
};

#endif // GAME_H_INCLUDED
