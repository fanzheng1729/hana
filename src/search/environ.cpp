#include <algorithm>    // for std::transform and std::min
#include "../proof/analyze.h"
#include "../database.h"
#include "../disjvars.h"
#include "environ.h"
#include "../io.h"
#include "goaldata.h"
#include "problem.h"
#include "../util/iter.h"
#include "../util/progress.h"

// Check if an expression is proven or is a hypothesis.
// If so, record its proof and return true.
bool proven(Goalptr p, Assertion const & ass)
{
    if (p->second.proven())
        return true;
    // Match hypotheses of the assertion.
    Hypsize i = ass.matchhyp(p->first.RPN, p->first.typecode);
    if (i == ass.hypcount())
        return false; // Hypotheses not matched
    // 1-step proof using the matched hypothesis
    p->second.proofsteps.assign(1, ass.hypiters[i]);
    return true;
}

// Add a goal. Return its pointer.
Goalptr Environ::addgoal(Proofsteps const & RPN, strview typecode, Goalstatus s)
{
    Goalview goal(RPN, typecode);
    Goals::iterator iter = goals.insert(Goals::value_type(goal, s)).first;
    return &*iter;
}
// # goals of a given status
Goals::size_type Environ::countgoal(int status) const
{
    Goals::size_type n = 0;
    FOR (Goals::const_reference goal, goals)
        n += (goal.second.status == status);
    return n;
}

// Return true if a move satisfies disjoint variable hypotheses.
bool Environ::checkdisjvars(Move const & move) const
{
    if (!move.pass) return false;

    FOR (Disjvars::const_reference vars, move.pass->second.disjvars)
    {
        // std::cout << "Checking DV " << move.pass->first;
        // revPolish notation of the expression substituted
        Proofsteps const & RPN1 = move.substitutions[vars.first];
        Proofsteps const & RPN2 = move.substitutions[vars.second];
        // Variables in the two substitutions
        Symbol3s set1, set2;
        std::transform
        (RPN1.begin(), RPN1.end(), end_inserter(set1), util::mem_fn(&Proofstep::symbol));
        std::transform
        (RPN2.begin(), RPN2.end(), end_inserter(set2), util::mem_fn(&Proofstep::symbol));
        // Erase empty variable.
        set1.erase(Symbol3()), set2.erase(Symbol3());
        // Check disjoint variable hypothesis.
        if (!::checkdisjvars(set1, set2, assertion.disjvars, &assertion.varusage, false))
            return false;
    }

    return true;
}

// Return true if all hypotheses of a move are valid.
bool Environ::valid(Move const & move) const
{
    if (!checkdisjvars(move))
        return false;

    std::vector<Goalptr> hypvec(move.hypcount());
    for (Hypsize i = 0; i < move.hypcount(); ++i)
    {
        if (move.hypfloats(i)) continue;
        // Add the essential hypothesis as a goal.
        Goalptr const goalptr = const_cast<Environ *>(this)
        ->addgoal(move.hypRPN(i), move.hyptypecode(i), GOALNEW);
// std::cout << "Validating " << goalptr->first.RPN;
        // Status of the goal
        Goalstatus & status = goalptr->second.status;
        if (status == GOALFALSE)
            return false; // Invalid goal
        // Record the goal in the hypotheses of the move.
        hypvec[i] = goalptr;
        // Check if the goal is a hypothesis or already proven.
        proven(goalptr, assertion);
        if (status == GOALOPEN || goalptr->second.proven())
            continue; // Valid goal
        // New goal (status == GOALNEW)
        status = valid(goalptr->first.RPN) ? GOALOPEN : GOALFALSE;
        if (status == GOALFALSE)
            return false; // Invalid goal
        // Simplify hypotheses needed.
        goalptr->second.hypstotrim = hypstotrim(goalptr);
// std::cout << "added " << goalptr << " in " << this;
// std::cout << ' ' << goalptr->first.RPN << goalptr->second.hypstotrim;
    }
// std::cout << moves;
    // Record the goal in the hypotheses of the move.
    const_cast<Move &>(move).hypvec.swap(hypvec);

    return true;
}

// Moves generated at a given stage
Moves Environ::ourmoves(Game const & game, stage_t stage) const
{
// if (stage >= 5)
// std::cout << "Finding moves at stage " << stage << " for " << game;
    AST & tree = game.goal().tree;
    if (tree.empty())
        tree = ast(game.goal().RPN);
    Moves moves;

    Assiters const & assvec = database.assiters();
    Assiters::size_type limit = std::min(assvec.size(), pProb->assertion.number);
    for (Assiters::size_type i = 1; i < limit; ++i)
    {
        Assiter const iter = assvec[i];
        Assertion const & ass = iter->second;
        if ((ass.type & Asstype::USELESS) || !ontopic(ass))
            continue; // Skip non propositional theorems.
        if (stage == 0 || (ass.nfreevar > 0 && stage >= ass.nfreevar))
            if (trythm(game, tree, iter, stage, moves))
                break; // Move closes the goal.
    }
// if (stage >= 5)
// std::cout << moves.size() << " moves found" << std::endl;
    return moves;
}

// Evaluate leaf games, and record the proof if proven.
Eval Environ::evalourleaf(Game const & game) const
{
    if (game.ndefer == 0 && !game.goaldata().hypstotrim.empty())
        pProb->addsubenv(game); // Simplify non-defer leaf by trimming hyps.
    return eval(game.env().hypslen + game.goal().size() + game.ndefer);
}

// Add a move with only bound substitutions.
// Return true if it has no essential hypotheses.
bool Environ::addboundmove(Move const & move, Moves & moves) const
{
    if (move.closes())
    {
        if (checkdisjvars(move))
            return moves.assign(1, move), true;
        else
            return false;
    }

    if (valid(move))
        moves.push_back(move);
    return false;
}

// Add Hypothesis-oriented moves. Return false.
bool Environ::addhypmoves(Move const & move, Moves & moves,
                          Stepranges const & stepranges) const
{
    Assertion const & thm = move.pass->second;
    // Iterate through key hypotheses i of the theorem.
    FOR (Hypsize i, thm.keyhyps)
    {
// std::cout << move.pass->first << ' ' << thm.hypiters[i]->first << std::endl;
        for (Hypsize j = 0; j < assertion.hypcount(); ++j)
        {
            if (assertion.hypfloats(j)) continue;
            // Match hypothesis j against key hypothesis i of the theorem.
// std::cout << assertion.hyplabel(j) << ' ' << assertion.hypRPN(j);
            Stepranges newsubs(stepranges);
            if (findsubstitutions
                (assertion.hypRPN(j), assertion.hypAST(j),
                 thm.hypRPN(i), thm.hypAST(i), newsubs))
            {
                // Free substitutions from the key hypothesis
                Move::Substitutions substitutions(stepranges.size());
                for (Stepranges::size_type k = 1; k < newsubs.size(); ++k)
                    substitutions[k].assign(newsubs[k].first,
                                            newsubs[k].second);
                Move newmove(move.pass, substitutions);
                if (valid(newmove))
                    moves.push_back(newmove);
//std::cin.get();
            }
        }
    }
    return false;
}

// Try applying the theorem, and add moves if successful.
// Return true if a move closes the goal.
bool Environ::trythm(Game const & game, AST const & ast, Assiter iter,
                     Proofsize size, Moves & moves) const
{
    Assertion const & thm = iter->second;
    if (thm.expression.empty() || thm.expression[0] != game.goal().typecode)
        return false; // Type code mismatch
// std::cout << "Trying " << iter->first << " with " << game.goal().expression();
    Stepranges stepranges;
    prealloc(stepranges, thm.varusage);
    if (!findsubstitutions(game.goal().RPN, ast, thm.expRPN, thm.expAST, stepranges))
        return false; // Conclusion mismatch

    // Bound substitutions
    Move::Substitutions substitutions(stepranges.size());
    for (Stepranges::size_type i = 1; i < stepranges.size(); ++i)
        substitutions[i] = Proofsteps(stepranges[i].first,
                                      stepranges[i].second);

    // Move with all bound substitutions
    Move move(&*iter, substitutions);
    return size > 0 ? addhardmoves(iter, size, move, moves) :
        thm.nfreevar > 0 ? addhypmoves(move, moves, stepranges) :
            addboundmove(move, moves);
}

// Return true if an assertion duplicates a previous one.
static bool isduplicate(Assertion const & ass, Database const & db)
{
    if (ass.type & (Asstype::AXIOM + Asstype::TRIVIAL))
        return false;

    static Value const zeros[2] = {0, 0};
    return Problem(Environ(ass, db, -1), zeros).playonce() == WDL::WIN;
}

// Mark duplicate assertions. Return its number.
Assertions::size_type Database::markduplicate()
{
    Assertions::size_type n = 0, count = 0;
    Progress progress;
    FOR (Assertions::reference rass, m_assertions)
    {
        // printass(rass);
        Assertion & ass = rass.second;
        bool const is = isduplicate(ass, *this);
        ass.type |= is * Asstype::DUPLICATE;
        if (is && !ass.expression.empty()
            && !typecodes().isprimitive(ass.expression[0]))
            ++count;
        progress << ++n/static_cast<Ratio>(assertions().size());
    }
    return count;
}
