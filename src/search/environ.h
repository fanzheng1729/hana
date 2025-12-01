#ifndef ENVIRON_H_INCLUDED
#define ENVIRON_H_INCLUDED

#include "../database.h"
#include "game.h"
#include "gen.h"
#include "goalstat.h"
#include "../MCTS/stageval.h"
#include "../syntaxDAG.h"

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

// Weight-based score
inline Value score(Weight weight) { return 1. / (weight + 1); }
inline Value score(double weight)
{
    if (weight < 1) weight = 1;
    return 1 / (weight + 1);
}

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
        assertion(ass),
        maxranks(db.hypsmaxranks(ass)),
        name(ass.hypslabel()),
        hypsweight(ass.hypslen()),
        staged(isstaged),
        m_subsumedbyProb(false), m_rankssimplerthanProb(false)
    {
        // Relevant syntax axioms
        FOR (Syntaxioms::const_reference syntaxiom, database.syntaxioms())
            if (syntaxiom.second.pass->second.number < assertion.number)
                syntaxioms.insert(syntaxiom);
    }
    Problem const & prob() const { return *pProb; }
    // Context implication relations
    // Updated when new context is added
    pEnvs const & psubEnvs() const { return m_psubEnvs; }
    pEnvs const & psupEnvs() const { return m_psupEnvs; }
    // Return true if *this <= problem context
    bool subsumedbyProb() const { return m_subsumedbyProb; }
    // Return true if maxranks is simpler than problem maxranks
    // Updated when problem is simplified
    bool rankssimplerthanProb() const { return m_rankssimplerthanProb; }
    // Return true if an assertion is on topic.
    virtual bool ontopic(Assertion const & ass) const { return ass.number>0; }
    // Determine status of a goal.
    virtual Goalstatus status(Goal const & goal) const
    { return goal.RPN.empty() ? GOALFALSE : GOALOPEN; }
    // Validity of a move.
    enum MoveValidity { MoveINVALID = -1, MoveVALID = 0, MoveCLOSED = 1 };
    // Validate a move.
    MoveValidity valid(Move const & move) const;
    // Return the hypotheses of a goal to be trimmed.
    virtual Bvector hypstotrim(Goal const & goal) const
    { return Bvector(0 && &goal); }
    // Moves generated at a given stage
    virtual Moves ourmoves(Game const & game, stage_t stage) const;
    // Weight of the game
    virtual Weight weight(Game const & game) const
    { return hypsweight + game.goal().RPN.size(); }
    // Evaluate leaf games, and record the proof if proven.
    virtual Eval evalourleaf(Game const & game) const
    { return score(weight(game) + game.nDefer); }
    // Allocate a new context constructed from an assertion on the heap.
    // Return its address. Return NULL if unsuccessful.
    virtual Environ * makeEnv(Assertion const &) const { return NULL; };
    // Return the simplified assertion for the goal of the game to hold.
    virtual Assertion makeAss(Bvector const &) const { return Assertion(); }
// Data members 
    // Database used
    Database const & database;
    // The assertion to be proved
    Assertion const & assertion;
    // Maximal ranks of all the hypotheses combined
    SyntaxDAG::Ranks const maxranks;
    std::string name;
    // Weight of all the hypotheses combined
    Weight const hypsweight;
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
    // Updated when new context is added
    mutable pEnvs m_psubEnvs, m_psupEnvs;
    // true if maxranks is simpler than problem maxranks
    // Updated when problem is simplified
    mutable bool m_rankssimplerthanProb;
    // Update context implication relations, given comparison result.
    void addEnv(Environ const & env, int cmp) const;
    // Compare contexts. Return -1 if *this < env, 1 if *this > env, 0 if not comparable.
    int compare(Environ const & env) const;
};

#endif // ENVIRON_H_INCLUDED
