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
    pGoal pgoal;
    // # defers to the game
    stage_t nDefer;
    // Proof attempt made, on their turn
    Move attempt;
    Game(pGoal p = NULL, stage_t n = 0) : pgoal(p), nDefer(n) {}
    Goaldata & goaldata() const;
    Goaldatas & goaldatas() const;
    Goal const & goal() const;
    Proofsteps const & proof() const;
    bool proven() const { return !proof().empty(); }
    Environ const & env() const;
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
    // Copy proof of the game to other contexts.
    void copyproof() const;
};

#endif // GAME_H_INCLUDED
