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
    p->second.proof.assign(1, ass.hypiters[i]);
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
    if (!move.pthm) return false;

    FOR (Disjvars::const_reference vars, move.pthm->second.disjvars)
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

    // Vector of the hypotheses of the move.
    std::vector<Goalptr> & hypvec = const_cast<Move &>(move).hypvec;
    // Record the hypotheses.
    hypvec.resize(move.hypcount());
    for (Hypsize i = 0; i < move.hypcount(); ++i)
    {
        if (move.hypfloats(i))
            continue;
        // Add the essential hypothesis as a goal.
        Goalptr pgoal = const_cast<Environ *>(this)->addgoal
            (move.hypRPN(i), move.hyptypecode(i), GOALNEW);
// std::cout << "Validating " << pgoal->first.RPN;
        // Status of the goal
        Goalstatus & status = pgoal->second.status;
        if (status == GOALFALSE)
            return false; // Refuted
        // Record the goal in the hypotheses of the move.
        hypvec[i] = pgoal;
        // Check if the goal is a hypothesis or already proven.
        if (proven(pgoal, assertion) || status == GOALOPEN)
            continue; // Valid
        // New goal (status == GOALNEW)
        status = valid(pgoal->first.RPN) ? GOALOPEN : GOALFALSE;
        if (status == GOALFALSE)
            return false; // Refuted
        // Simplify hypotheses needed.
        pgoal->second.hypstotrim = hypstotrim(pgoal);
// std::cout << "added " << pgoal << " in " << this;
// std::cout << ' ' << pgoal->first.RPN << pgoal->second.hypstotrim;
    }
// std::cout << moves;

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
        if (stage == 0 || (ass.nfreevar() > 0 && stage >= ass.nfreevar()))
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
    return score(game.env().hypslen + game.goal().size() + game.ndefer);
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
        // std::cout << move << std::endl,
        // std::cout << move.substitutions,
        moves.push_back(move);
// std::cin.get();
    return false;
}

static int const STACKEMPTY = -2;
// Advance the stack and return the difference in # matched hypotheses.
// Return STACKEMPTY if stack cannot be advanced.
static int next(Hypsizes & hypstack, std::vector<Stepranges> & substack,
                Assertion const & ass, Assertion const & thm)
{
    int delta = 0;
    while (!hypstack.empty())
    {
        Hypsize const thmhyp = thm.hypsorder[hypstack.size()-1];
        strview thmhyptype = thm.hyptypecode(thmhyp);
        // Advance the last hypothesis in the stack.
        Hypsize & asshyp = hypstack.back();
        if (asshyp < ass.hypcount())
            --delta; // 1 match removed
        for (++asshyp; asshyp <= ass.hypcount(); ++asshyp)
        {
            if (asshyp < ass.hypcount() &&
                (ass.hypfloats(asshyp) ||
                 ass.hyptypecode(asshyp) != thmhyptype))
                continue; // Skip floating hypothesis.
            // Copy the last substitution.
            substack[hypstack.size()] = substack[hypstack.size()-1];
            if (asshyp == ass.hypcount())
                return delta; // No new match
            if (findsubstitutions
                (ass.hypRPN(asshyp), ass.hypAST(asshyp),
                 thm.hypRPN(thmhyp), thm.hypAST(thmhyp),
                 substack[hypstack.size()]))
                return ++delta; // New match
        }
        hypstack.pop_back();
    }
    return STACKEMPTY;
}

// Add Hypothesis-oriented moves. Return false.
bool Environ::addhypmoves(Assptr pthm, Moves & moves,
                          Stepranges const & stepranges,
                          Hypsize maxfreehyps) const
{
    Assertion const & thm = pthm->second;
    // Hypothesis stack
    Hypsizes hypstack;
    // Substitution stack
    std::vector<Stepranges> substack(thm.nfreehyps() + 1);
    if (substack.empty() || assertion.hypcount() + 1 == 0)
        return false; // size overflow
    // Preallocate for efficiency.
    Hypsize const nfreehyps = thm.nfreehyps();
    hypstack.reserve(nfreehyps);
    // Bound substitutions
    substack[0] = stepranges;
    // substack.size() == hypstack.size() + 1
    // # matched hypotheses
    Hypsize matchedhyps = 0;
    int delta;
    do
    {
        // std::cout << hypstack;
        if (thm.allvarsfilled(substack[hypstack.size()]))
        {
            Move move(pthm, substack[hypstack.size()]);
            if (valid(move))
                // std::cout << move << std::endl,
                // std::cout << move.substitutions,
                moves.push_back(move);
// std::cin.get();
        }
        else
        if (hypstack.size() < nfreehyps && matchedhyps < maxfreehyps)
        {
            // Match new hypothesis.
            hypstack.push_back(static_cast<Hypsize>(-1));
        }
        delta = ::next(hypstack, substack, assertion, thm);
        matchedhyps += delta;
    } while (delta != STACKEMPTY);

    return false;
}
bool Environ::addhypmoves(Assptr pthm, Moves & moves,
                          Stepranges const & stepranges) const
{
    Assertion const & thm = pthm->second;
    FOR (Hypsize thmhyp, thm.hypsorder)
    {
        if (thm.nfreevars[thmhyp] < thm.nfreevar())
            return false;
// std::cout << move.label() << ' ' << thm.hyplabel(thmhyp) << ' ';
        strview thmhyptype = thm.hyptypecode(thmhyp);
        for (Hypsize asshyp = 0; asshyp < assertion.hypcount(); ++asshyp)
        {
            if (assertion.hypfloats(asshyp) ||
                assertion.hyptypecode(asshyp) != thmhyptype)
                continue;
            // Match hypothesis asshyp against key hypothesis thmhyp of the theorem.
            Stepranges newsubst(stepranges);
            if (findsubstitutions(assertion.hypRPN(asshyp), assertion.hypAST(asshyp),
                                  thm.hypRPN(thmhyp), thm.hypAST(thmhyp),
                                  newsubst))
            {
// std::cout << assertion.hyplabel(asshyp) << ' ' << assertion.hypexp(asshyp);
                Move move(pthm, newsubst);
                if (valid(move))
                    // std::cout << move << std::endl,
                    // std::cout << move.substitutions,
                    moves.push_back(move);
// std::cin.get();
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

    // Move with all bound substitutions
    Move move(&*iter, stepranges);
    if (size > 0)
        return thm.nfreevar() > 0 && addhardmoves(&*iter, size, move, moves);
    else if (thm.nfreevar() > 0)
        return assertion.esshypcount() > 0 && addhypmoves(move.pthm, moves, stepranges);
    else
        return addboundmove(move, moves);
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
