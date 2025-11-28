#ifndef ENVIRON_H_INCLUDED
#define ENVIRON_H_INCLUDED

#include "../ass.h"
#include "game.h"
#include "gen.h"
#include "goalstat.h"
#include "../MCTS/stageval.h"

// Compare two hypiters by address.
inline int comphypiters(Hypiter x, Hypiter y)
{
    return std::less<Hypptr>()(&*x, &*y);
}

class Database;
class Problem;
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
typedef std::map<std::string, struct Environ const *> Environs;
// Iterator to polymorphic contexts
typedef Environs::const_iterator Enviter;
// Polymorphic context, with move generation and goal evaluation
struct Environ : protected Gen
{
    Environ(Assertion const & ass, Database const & db,
            std::size_t maxsize, bool isstaged = false) :
        database(db), assertion(ass), hypslen(ass.hypslen()), staged(isstaged),
        Gen(ass.varusage, maxsize), m_subProb(false) {}
    Problem const & prob() const { return *pProb; }
    // Context relations
    bool impliedby(Environ const & env) const;
    bool implies(Environ const & env) const;
    // Return true if the context is a sub-context of the problem context
    bool issubProb() const { return m_subProb; }
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
    // Iterator to the context
    Enviter enviter;
    strview name() const { return enviter->first; }
    // Database used
    Database const & database;
    // The assertion to be proved
    Assertion const & assertion;
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
    bool addhypmoves(Assptr pthm, Moves & moves,
                     Stepranges const & stepranges,
                     Hypsize maxfreehyps) const;
    bool addhypmoves(Assptr pthm, Moves & moves,
                     Stepranges const & stepranges) const;
    // Add a move with free variables. Return false.
    virtual bool addhardmoves
        (Assptr pthm, Proofsize size, Move & move, Moves & moves) const
        { return pthm && !pthm && size && &move && &moves; }
    // Try applying the theorem, and add moves if successful.
    // Return true if a move closes the goal.
    bool trythm(Game const & game, AST const & ast, Assiter iter,
                Proofsize size, Moves & moves) const;
// Cache for context implications
    bool m_subProb;
    // Pointers to contexts
    typedef std::vector<Environ const *> pEnvs;
    mutable pEnvs psubEnvs;
    mutable pEnvs psupEnvs;
    // Add env to context relations, given cmp = compEnvs(*this, env).
    int addEnv(Environ const & env, int cmp) const;
    // Add env to context relations. Return compEnvs(*this, env).
    int addEnv(Environ const & env) const;
};

// Compare two contexts. Return -1 if x < y, 1 if x > y, 0 if not comparable.
int compEnvs(Environ const & x, Environ const & y);

#endif // ENVIRON_H_INCLUDED
