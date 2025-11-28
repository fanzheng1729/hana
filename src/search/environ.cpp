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

static Symbol3s symbols(Proofsteps const & RPN)
{
    Symbol3s set;
    std::transform(RPN.begin(), RPN.end(), end_inserter(set),
                   util::mem_fn(&Proofstep::var));
    set.erase(Symbol3());
    return set;
}

// Return true if a move satisfies disjoint variable hypotheses.
bool checkDV(Move const & move, Assertion const & ass, bool verbose)
{
    if (!move.pthm)
        return false;
// std::cout << "Checking DV of move " << move.label() << std::endl;
    FOR (Disjvars::const_reference vars, move.pthm->second.disjvars)
    {
        const Proofsteps & RPN1 = move.substitutions[vars.first];
        const Proofsteps & RPN2 = move.substitutions[vars.second];
// std::cout << vars.first << ":\t" << RPN1 << vars.second << ":\t" << RPN2;
        const Symbol3s & set1(symbols(RPN1));
        const Symbol3s & set2(symbols(RPN2));
        if (!checkDV(set1, set2, ass.disjvars, ass.varusage, verbose))
            return false;
    }

    return true;
}

// Context relations
bool Environ::impliedby(Environ const & env) const
{
    return prob().environs.reachable(env.enviter, enviter);
}
bool Environ::implies(Environ const & env) const
{
    return prob().environs.reachable(enviter, env.enviter);
}
bool hasimplication(Environ const & from, Environ const & to)
{
    return from.implies(to);
}

// Return true if the context is a sub-context of the problem context
bool issubProb(Environ const & env)
{
    return env.issubProb();
}

// Return true if all hypotheses of a move are valid.
bool Environ::valid(Move const & move) const
{
    if (!checkDV(move, assertion))
        return false;
    // Record the hypotheses.
    move.hypvec.resize(move.hypcount());
    for (Hypsize i = 0; i < move.hypcount(); ++i)
    {
        if (move.hypfloats(i)) continue;
        // Add the essential hypothesis as a goal.
        Goalptr const pGoal = pProb->addGoal
        (move.hypRPN(i), move.hyptypecode(i), const_cast<Environ *>(this), GOALNEW);
// std::cout << "Validating " << pGoal->second.goal().expression();
        Goalstatus & s = pGoal->second.getstatus(*pGoal->first);
        if (s == GOALFALSE)
            return false; // Refuted
        if (s >= GOALOPEN)// Valid
        {
            move.hypvec[i]
            = pGoal->second.proven(*pGoal->first) ? pGoal : addsimpGoal(pGoal);
            continue;
        }
        // New goal (s == GOALNEW)
        Goal const & goal = pGoal->second.goal();
        s = status(goal);
        if (s == GOALFALSE)
            return false; // Refuted
        // Simplified context for the child, if !NULL
        Environ * & pnewEnv = pGoal->second.pnewEnv;
        pnewEnv = pProb->addsubEnv(this, hypstotrim(goal));
        // Record the goal in the hypotheses of the move.
        move.hypvec[i] = addsimpGoal(pGoal);
// if (pnewEnv)
// std::cout << pGoal->second.goal().expression() << label() << "\n->\n",
// std::cout << (pnewEnv ? pnewEnv->label() : "") << std::endl;
    }
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
    Assiters::size_type limit = std::min(assvec.size(), prob().assertion.number);
    for (Assiters::size_type i = 1; i < limit; ++i)
    {
        Assertion const & ass = assvec[i]->second;
        if ((ass.type & Asstype::USELESS) || !ontopic(ass))
            continue; // Skip non propositional theorems.
        if (stage == 0 || (ass.nfreevar() > 0 && stage >= ass.nfreevar()))
            if (trythm(game, tree, assvec[i], stage, moves))
                break; // Move closes the goal.
    }
// if (stage >= 5)
// std::cout << moves.size() << " moves found" << std::endl;
    return moves;
}

// Add a move with only bound substitutions.
// Return true if it has no essential hypotheses.
bool Environ::addboundmove(Move const & move, Moves & moves) const
{
    if (move.closes())
    {
        if (checkDV(move, assertion))
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
    if (thm.expression.empty() || thm.exptypecode() != game.goal().typecode)
        return false; // Type code mismatch
// std::cout << "Trying " << iter->first << " with " << game.goal().expression();
    Stepranges stepranges;
    prealloc(stepranges, thm.varusage);
    if (!findsubstitutions(game.goal().RPN, ast, thm.expRPN, thm.expAST, stepranges))
        return false; // Conclusion mismatch

    // Move with all bound substitutions
    Move move(&*iter, stepranges);
    if (size > 0)
        return thm.nfreevar() > 0 && addhardmoves(move.pthm, size, move, moves);
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
        bool const isdup = isduplicate(ass, *this);
        ass.type |= isdup * Asstype::DUPLICATE;
        // Check if it is duplicate and starts with a non-primitive type code.
        if (isdup && !ass.expression.empty()
            && typecodes().isprimitive(ass.exptypecode()) == FALSE)
            ++count;
        progress << ++n/static_cast<Ratio>(assertions().size());
    }
    return count;
}
