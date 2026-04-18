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

// Weight-based score
inline Value score(Weight w) { return 1. / (w + 1); }
inline Value score(double w) { return 1. / ((w < 1 ? 1 : w) + 1); }

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
        nhyps(ass.nhyps()),
        hypsweight(ass.hypslen()),
        staged(isstaged),
        pProb(NULL),
        m_subsumedbyProb(false),
        m_rankssimplerthanProb(false)
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
    bool rankssimplerthanProb() const { return m_rankssimplerthanProb; }
    // Update when problem is simplified
    void updateimps(SyntaxDAG::Ranks const & probmaxranks) const
    {
        m_rankssimplerthanProb =
        database.syntaxDAG().simplerthan(maxranks, probmaxranks);
    }
    // Return true if an assertion is on topic.
    virtual bool ontopic(Assertion const & ass) const { return ass.number; }
    // Determine status of a goal.
    virtual Goalstatus status(Goal const & goal) const
    { return goal.rpn.empty() ? GOALFALSE : GOALOPEN; }
    // Report false goal and return GOALFALSE.
    Goalstatus printbadgoal(RPN const & badRPN) const;
    // Validity of a move.
    enum MoveValidity { MoveINVALID = -1, MoveVALID = 0, MoveCLOSED = 1 };
    // Validate a move.
    MoveValidity valid(Move const & move) const
    {
        if (database.typecodes().isprimitive(move.goaltypecode()) != FALSE)
            return MoveINVALID;
        if (move.isconj())
            return validconjmove(move);
        if (!move.isthm() || !move.checkDV(assertion))
            return MoveINVALID;
        return validthmmove(move);
    }
    // Return the hypotheses of a goal to be trimmed.
    virtual Bvector hypstotrim(Goal const & goal) const
    { return Bvector(0 && &goal); }
    // Moves generated at a given stage
    virtual Moves ourmoves(Game const & game, stage_t stage) const;
    // Weight of the goal
    virtual Weight weight(RPN const & goal) const { return goal.size(); }
    // Weight of the game
    Weight weight(Game const & game) const
    { return hypsweight + weight(game.goal().rpn); }
    // Evaluate leaf games, and record the proof if proven.
    virtual Eval evalourleaf(Game const & game) const
    { return score(weight(game) + game.wDefer()); }
    // Create a new context constructed from an assertion on the heap.
    // Return its address. Return NULL if unsuccessful.
    virtual Environ * makeEnv(Assertion const &) const { return NULL; };
    // Return true if a proof is legal.
    bool legal(RPN const & proof) const;
    // A minimal context in which a proof is legal
    Environ const & minLegalEnv(RPN const & proof) const
    {
        FOR (Environ const * psubEnv, psubEnvs())
            if (psubEnv->legal(proof))
                return psubEnv->minLegalEnv(proof);
        return *this;
    }
// Data members 
    // Database used
    Database const & database;
    // The assertion to be proved
    Assertion const & assertion;
    // Maximal ranks of all the hypotheses combined
    SyntaxDAG::Ranks const maxranks;
    // Name of context = labels of all hypotheses combined
    std::string const name;
    // # hypotheses
    Hypsize const nhyps;
    // Weight of all the hypotheses combined
    Weight hypsweight;
    // Is staged move generation used?
    bool const staged;
protected:
    // Pointer to the problem
    Problem * pProb;
    friend Problem;
private:
// Move generation
    // Add a move with only bound substitutions.
    // Return true if it has no open hypotheses.
    bool addboundmove(Move const & move, Moves & moves) const;
    // Add a conjectural move. Return true if it has no open hypotheses.
    bool addconjmove(Move const & move, Moves & moves) const;
    // Add abstraction moves. Return true if it has no open hypotheses.
    bool addabsmoves(Goal const & goal, pAss pthm, Moves & moves) const;
    // Add an abstraction move. Return true if it has no open hypotheses.
    bool addabsmove
        (Goal const & goal, RPNspan abstraction,
         Move const & move, Moves & moves) const;
    // Add Hypothesis-oriented moves.
    // Return true if it has no open hypotheses.
    bool addhypmoves(pAss pthm, Moves & moves,
                     RPNspans const & substs) const;
    bool addhypmoves(pAss pthm, Moves & moves,
                     RPNspans const & substs,
                     Hypsize maxfreehyps) const;
    // Add moves with free variables.
    // Return true if it has no open hypotheses.
    virtual bool addhardmoves
        (pAss pthm, RPNsize size, Move & move, Moves & moves) const
        { return pthm && !pthm && size && &move && &moves; }
    // Try applying the theorem, and add moves if successful.
    // Return true if it has no open hypotheses.
    bool trythm
        (Game const & game, Assiter iter, RPNsize size, Moves & moves) const;
// Validate
    // a move applying a theorem.
    MoveValidity validthmmove(Move const & move) const;
    // a conjectural move.
    MoveValidity validconjmove(Move const & move) const;
// Private members
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
