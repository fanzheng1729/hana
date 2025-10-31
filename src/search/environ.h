#ifndef ENVIRON_H_INCLUDED
#define ENVIRON_H_INCLUDED

#include "../ass.h"
#include "gen.h"
#include "goal.h"
#include "goalstat.h"
#include "../MCTS/stageval.h"

class Database;
// Move in proof search tree
struct Move;
// List of moves
typedef std::vector<Move> Moves;
// Game state in proof search tree
struct Game;
// Problem statement + Proof search tree with loop detection
// + environment management + UI
class Problem;

// Size-based score
inline Value score(Proofsize size) { return 1. / (size + 1); }
inline Eval eval(Proofsize size) { return Eval(score(size)); }

// Check if an expression is proven or is a hypothesis.
// If so, record its proof and return true.
bool proven(Goalptr p, Assertion const & ass);

// Proof search environment
struct Environ : protected Gen
{
    Environ(Assertion const & ass, Database const & db,
            std::size_t maxsize, bool isstaged = false) :
        database(db), assertion(ass), staged(isstaged), hypslen(ass.hypslen()),
        Gen(ass.varusage, maxsize) {}
    // Add a goal. Return its pointer.
    Goalptr addgoal(Proofsteps const & RPN, strview typecode, Goalstatus s);
    // # goals of a given status
    Goals::size_type countgoal(int status) const;
    // Return true if an assertion is on topic.
    virtual bool ontopic(Assertion const & ass) const { return ass.number; }
    // Return the hypotheses of a goal to be trimmed.
    virtual Bvector hypstotrim(Goalptr p) const { return Bvector(0 && p); }
    // Return true if a goal is valid.
    virtual bool valid(Proofsteps const & goal) const { return !goal.empty(); }
    // Return true if a move satisfies disjoint variable hypotheses.
    bool checkdisjvars(Move const & move) const;
    // Return true if all hypotheses of a move are valid.
    bool valid(Move const & move) const;
    // Moves generated at a given stage
    virtual Moves ourmoves(Game const & game, stage_t stage) const;
    // Evaluate leaf games, and record the proof if proven.
    virtual Eval evalourleaf(Game const & game) const;
    // Allocate a new sub environment constructed from a sub assertion on the heap.
    // Return its address. Return NULL if unsuccessful.
    virtual Environ * makeenv(Assertion const &) const { return NULL; };
    // Return the simplified assertion for the goal of the game to hold.
    virtual Assertion makeass(Game const &) const { return Assertion(); }
    // Database to be used
    Database const & database;
    // The assertion to be proved
    Assertion const & assertion;
    // Is staged move generation turned on?
    bool const staged;
    // Length of the rev Polish notation of all hypotheses combined
    Proofsize const hypslen;
protected:
    // Pointer to the problem
    Problem * pProb;
    friend Problem;
private:
    // Set of goals looked at
    Goals goals;
// private methods
    // Add a move with only bound substitutions.
    // Return true if it has no essential hypotheses.
    bool addboundmove(Move const & move, Moves & moves) const;
    // Add Hypothesis-oriented moves. Return false.
    bool addhypmoves(Move const & move, Moves & moves,
                     Stepranges const & stepranges) const;
    // Add a move with free variables. Return false.
    virtual bool addhardmoves
        (Assiter iter, Proofsize size, Move & move, Moves & moves) const
        { return size == 0 && &move && &moves && !&*iter; }
    // Try applying the theorem, and add moves if successful.
    // Return true if a move closes the goal.
    bool trythm(Game const & game, AST const & ast, Assiter iter,
                Proofsize size, Moves & moves) const;
};

#endif // ENVIRON_H_INCLUDED
