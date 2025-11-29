#ifndef ENVIRON_H_INCLUDED
#define ENVIRON_H_INCLUDED

#include "../ass.h"
#include "game.h"
#include "gen.h"
#include "goalstat.h"
#include "../MCTS/stageval.h"

class Database;
class Problem;
struct Environ;
// Pointers to contexts
typedef std::vector<Environ const *> pEnvs;
// Move in proof search tree
// struct Move;
// List of moves
// typedef std::vector<Move> Moves;
// Game state in proof search tree
// struct Game;

// Size-based score
inline Value score(Proofsize size) { return 1. / (size + 1); }

// Return true if a move satisfies disjoint variable hypotheses.
bool checkDV(Move const & move, Assertion const & ass, bool verbose = false);

// Map: name -> polymorphic contexts
typedef std::map<std::string, Environ const *> Environs;
// Iterator to polymorphic contexts
typedef Environs::const_iterator Enviter;
// Polymorphic context, with move generation and goal evaluation
struct Environ : protected Gen
{
    Environ(Assertion const & ass, Database const & db,
            std::size_t maxsize, bool isstaged = false) :
        Gen(ass.varusage, maxsize),
        database(db),
        assertion(ass), name0(ass.hypslabel()), hypslen(ass.hypslen()),
        staged(isstaged),
        m_subsumedbyProb(false) {}
    Problem const & prob() const { return *pProb; }
    // Context relations
    pEnvs const & psubEnvs() const { return m_psubEnvs; }
    pEnvs const & psupEnvs() const { return m_psupEnvs; }
    // Return true if *this <= problem context
    bool subsumedbyProb() const { return m_subsumedbyProb; }
    // Return true if an assertion is on topic.
    virtual bool ontopic(Assertion const & ass) const { return ass.number; }
    // Return the hypotheses of a goal to be trimmed.
    virtual Bvector hypstotrim(Goal const & goal) const
    { return Bvector(0 && &goal); }
    // Determine status of a goal.
    virtual Goalstatus status(Goal const & goal) const
    { return goal.RPN.empty() ? GOALFALSE : GOALOPEN; }
    // Return true if all hypotheses of a move are valid.
    bool valid(Move const & move) const;
    // Moves generated at a given stage
    virtual Moves ourmoves(Game const & game, stage_t stage) const;
    // Evaluate leaf games, and record the proof if proven.
    virtual Eval evalourleaf(Game const & game) const
    { return score(game.env().hypslen + game.goal().size() + game.nDefer); }
    // Allocate a new context constructed from an assertion on the heap.
    // Return its address. Return NULL if unsuccessful.
    virtual Environ * makeEnv(Assertion const &) const { return NULL; };
    // Return the simplified assertion for the goal of the game to hold.
    virtual Assertion makeAss(Bvector const &) const { return Assertion(); }
 
    // Database used
    Database const & database;
    // The assertion to be proved
    Assertion const & assertion;
    strview name;
    std::string name0;
    // Length of the rev Polish notation of all hypotheses combined
    Proofsize const hypslen;
    // Is staged move generation used?
    bool const staged;
protected:
    // Pointer to the problem
    Problem * pProb;
    friend Problem;
private:
// Move generation
    // Add a move with only bound substitutions.
    // Return true if it has no essential hypotheses.
    bool addboundmove(Move const & move, Moves & moves) const;
    // Add Hypothesis-oriented moves. Return false.
    bool addhypmoves(pAss pthm, Moves & moves,
                     Stepranges const & stepranges,
                     Hypsize maxfreehyps) const;
    bool addhypmoves(pAss pthm, Moves & moves,
                     Stepranges const & stepranges) const;
    // Add a move with free variables. Return false.
    virtual bool addhardmoves
        (pAss pthm, Proofsize size, Move & move, Moves & moves) const
        { return pthm && !pthm && size && &move && &moves; }
    // Try applying the theorem, and add moves if successful.
    // Return true if a move closes the goal.
    bool trythm(Game const & game, AST const & ast, Assiter iter,
                Proofsize size, Moves & moves) const;
    // true if *this <= problem context
    bool m_subsumedbyProb;
    // Cache for context implication relations
    mutable pEnvs m_psubEnvs, m_psupEnvs;
    // Update context implication relations, given comparison result.
    void addEnv(Environ const & env, int cmp) const;
    // Compare contexts. Return -1 if *this < env, 1 if *this > env, 0 if not comparable.
    int compare(Environ const & env) const;
};

#endif // ENVIRON_H_INCLUDED
