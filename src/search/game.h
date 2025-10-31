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
    // Pointer to rev Polish of expression to be proved
    Goalptr goalptr;
    // Pointer to the current environment
    Environ * penv;
    // # defers to the game
    stage_t ndefer;
    // If the game has been evaluated, on their turn
    bool evaled;
    // Proof attempt made, on their turn
    Move attempt;
    // Essential hypotheses needed, on their turn
    std::set<Goalptr> hypset;
    Game(Goalptr pgoal = NULL, Environ * p = NULL, stage_t defer = 0) :
        goalptr(pgoal), penv(p), ndefer(defer), evaled(false) {}
    Goal const & goal() const;
    Goaldata & goaldata() const;
    Environ const & env() const { return *penv; }
    friend std::ostream & operator<<(std::ostream & out, Game const & game);
    // Set the hypothesis set of a game.
    void sethyps() const;
    void setevaled() const { const_cast<bool &>(evaled) = true; }
    // Return true if the hypotheses of game 1 includes those of game 2.
    bool operator>=(Game const & game) const;
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
