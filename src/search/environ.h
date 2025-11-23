#ifndef ENVIRON_H_INCLUDED
#define ENVIRON_H_INCLUDED

#include "../ass.h"
#include "game.h"
#include "gen.h"
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
// + context management + UI
class Problem;

// Size-based score
inline Value score(Proofsize size) { return 1. / (size + 1); }

// Check if an expression is proven or is a hypothesis.
// If so, record its proof and return true.
bool proven(Goalptr p, Assertion const & ass);

// Return true if a move satisfies disjoint variable hypotheses.
bool checkDV(Move const & move, Assertion const & ass, bool verbose = false);

// Map: name -> polymorphic contexts
typedef std::map<std::string, struct Environ *> Environs;
// Polymorphic context
struct Environ : protected Gen
{
    Environ(Assertion const & ass, Database const & db,
            std::size_t maxsize, bool isstaged = false) :
        database(db), assertion(ass), staged(isstaged), hypslen(ass.hypslen()),
        Gen(ass.varusage, maxsize) {}
    Problem const & prob() const { return *pProb; }
    // Add a goal. Return its pointer.
    Goalptr addgoal(Proofsteps const & RPN, strview typecode, Goalstatus s);
    // # goals of a given status
    Goals::size_type countgoal(int status) const;
    // # proven goals
    Goals::size_type countproof() const;
    // Printing utilities
    void printgoal() const;
    void printgoal(int status) const;
    void printstats() const;
    // Return true if an assertion is on topic.
    virtual bool ontopic(Assertion const & ass) const { return ass.number; }
    // Return the hypotheses of a goal to be trimmed.
    virtual Bvector hypstotrim(Goalptr p) const { return Bvector(0 && p); }
    // Determine status of a goal.
    virtual Goalstatus valid(Proofsteps const & goal) const
    { return goal.empty() ? GOALFALSE : GOALOPEN; }
    // Return true if all hypotheses of a move are valid.
    bool valid(Move const & move) const;
    // Moves generated at a given stage
    virtual Moves ourmoves(Game const & game, stage_t stage) const;
    // Evaluate leaf games, and record the proof if proven.
    virtual Eval evalourleaf(Game const & game) const
    { return score(game.env().hypslen + game.goal().size() + game.ndefer); }
    // Allocate a new context constructed from an assertion on the heap.
    // Return its address. Return NULL if unsuccessful.
    virtual Environ * makeenv(Assertion const &) const { return NULL; };
    // Return the simplified assertion for the goal of the game to hold.
    virtual Assertion makeass(Bvector const &) const { return Assertion(); }
    // Iterator to the context
    Environs::const_iterator enviter;
    strview label() const { return enviter->first; }
    // Database used
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
    // Map: goal -> Evaluation
    Goals goals;
// private methods
    // Add a move with only bound substitutions.
    // Return true if it has no essential hypotheses.
    bool addboundmove(Move const & move, Moves & moves) const;
    // Add Hypothesis-oriented moves. Return false.
    bool addhypmoves(Assptr pthm, Moves & moves,
                     Stepranges const & stepranges,
                     Hypsize maxfreehyps) const;
    bool addhypmoves(Assptr pthm, Moves & moves,
                     Stepranges const & stepranges) const;
    // Add a move with free variables. Return false.
    virtual bool addhardmoves
        (Assptr pthm, Proofsize size, Move & move, Moves & moves) const
        { return !pthm && size == 0 && &move && &moves; }
    // Try applying the theorem, and add moves if successful.
    // Return true if a move closes the goal.
    bool trythm(Game const & game, AST const & ast, Assiter iter,
                Proofsize size, Moves & moves) const;
};

#endif // ENVIRON_H_INCLUDED
