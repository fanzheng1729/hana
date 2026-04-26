#ifndef ENVIRON_H_INCLUDED
#define ENVIRON_H_INCLUDED

#include "../database.h"
#include "game.h"
#include "gen.h"
#include "goalstat.h"
#include "../MCTS/stageval.h"
#include "../syntaxDAG.h"
#include "../util/algo.h"   // for util::addordered

class Problem;
struct Environ;
// Pointers to contexts
typedef std::vector<Environ const *> pEnvs;

// Weight-based score
inline Value score(Weight w) { return 1. / (w + 1); }
inline Value score(double w) { return 1. / ((w < 1 ? 1 : w) + 1); }

// Polymorphic context, with move generation and goal evaluation
struct Environ : protected Gen
{
    // Prepare term generator
    void prepareGen() const;
    Environ(Assertion const & ass, Database const & db,
            std::size_t maxsize, bool isstaged = false) :
        Gen(ass.varusage, maxsize),
        assertion(ass),
        maxranks(db.hypsmaxranks(ass)),
        label(ass.hypslabel()),
        nhyps(ass.nhyps()),
        hypsweight(ass.hypslen()),
        hasnewvarinexp(ass.hasnewvarinexp()),
        staged(isstaged),
        pProb(),
        sortedhyps(ass.hypiters),
        m_subsumedbyProb(false),
        m_rankssimplerthanProb(false)
    { std::sort(sortedhyps.begin(), sortedhyps.end(), comphypiter); }
    nAss assnum() const { return assertion.number; }
    Problem const & prob() const { return *pProb; }
    // Context implication relations
    // Updated when new context is added
    pEnvs const & psubEnvs() const { return m_psubEnvs; }
    pEnvs const & psupEnvs() const { return m_psupEnvs; }
    pEnvs::size_type nsubEnvs() const { return psubEnvs().size(); }
    pEnvs::size_type nsupEnvs() const { return psupEnvs().size(); }
    // Return true if *this <= problem context
    bool subsumedbyProb() const { return m_subsumedbyProb; }
    // Return true if maxranks is simpler than problem maxranks
    bool rankssimplerthanProb() const { return m_rankssimplerthanProb; }
    // Return true if an assertion is on topic.
    virtual bool ontopic(Assertion const & ass) const
    { return ass.number; }
    // Return true if an assertion can be used in conjecture moves.
    bool usableasconj(Assertion const & ass) const
    { return ontopic(ass) && ass.nEhyps() == 0; }
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
        return move.isconj() ? validconjmove(move) :
                move.pthm ? validthmmove(move) : MoveINVALID;
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
    // Return its address. Return nullptr if unsuccessful.
    virtual Environ * makeEnv(Assertion const &) const { return NULL; };
    // Return true if a proof is legal.
    bool legal(RPN const & proof) const;
// Data members
    // The assertion to be proved
    Assertion const & assertion;
    // Max ranks of all the hypotheses combined
    SyntaxDAG::Ranks const maxranks;
    // Name of context = labels of all hypotheses combined
    std::string const label;
    // # hypotheses
    Hypsize const nhyps;
    // Weight of all the hypotheses combined
    Weight hypsweight;
    // True if there is a var only used in exp.
    bool const hasnewvarinexp;
    // Is staged move generation used?
    bool const staged;
protected:
    // Pointer to the problem
    Problem * pProb;
    friend Problem;
// Validate
    // a move applying a theorem
    MoveValidity validthmmove(Move const & move) const;
    // a conjectural move
    MoveValidity validconjmove(Move const & move) const;
private:
// Move generation
    // Add a move with validation. Return true if it has no open hypotheses.
    template<Environ::MoveValidity (Environ::*Validator)(Move const &) const>
    bool addvalidmove(Move const & move, Moves & moves) const;
    bool addboundmove(Move const & move, Moves & moves) const;
    // Add a conjectural move. Return true if it has no open hypotheses.
    bool addconjmove(Move const & move, Moves & moves) const;
    // Try applying an assertion, and add moves if okay.
    // Return true if it has no open hypotheses.
    bool tryass
        (Game const & game, Assiter iter, RPNsize size, Moves & moves) const;
    // Add Hypothesis-oriented moves.
    // Return true if it has no open hypotheses.
    bool addhypmoves(pAss pthm, Moves & moves,
                     RPNspans const & substs) const;
    bool addhypmoves(pAss pthm, Moves & moves,
                     RPNspans const & substs,
                     Hypsize maxfreehyps) const;
    // Add moves with free variables.
    // Return true if it has no open hypotheses.
    virtual bool addhardmoves(Move & move, RPNsize size, Moves & moves) const
        { return size && !size && &move && &moves; }
    // Add abstraction moves. Return true if it has no open hypotheses.
    bool addabsmoves(Goal const & goal, Moves & moves) const;
    bool addabsmoves
        (Goal const & goal, RPNspanAST subexp, Moves & moves) const;
    // Abstraction-substitutions for a sub-expression
    Absubstmoves absubsts(RPNspanAST subexp) const;
// Private members
    // Sorted iterators to hypotheses
    Hypiters sortedhyps;
    // true if *this <= problem context
    bool m_subsumedbyProb;
    // Cache for context implication relations
    // Updated when new context is added
    mutable pEnvs m_psubEnvs, m_psupEnvs;
    // true if maxranks is simpler than problem maxranks
    // Updated when problem is simplified
    mutable bool m_rankssimplerthanProb;
    // Return true if hypotheses of *this contains those of env.
    bool implies(Environ const & env) const;
    // Update context implication relations.
    void addsubEnv(Environ const & env) const
    { util::addordered(m_psubEnvs, &env); }
    void addsupEnv(Environ const & env) const
    { util::addordered(m_psupEnvs, &env); }
    void addimps(Environ const & env) const
    { if (implies(env)) addsubEnv(env), env.addsupEnv(*this); }
};

#endif // ENVIRON_H_INCLUDED
