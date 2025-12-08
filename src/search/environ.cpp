#include <algorithm>    // for std::min and ordered algorithms
#include "../proof/analyze.h"
#include "environ.h"
#include "../io.h"
#include "goaldata.h"
#include "problem.h"
#include "../proof/skeleton.h"
#include "../util/progress.h"

// Return true if the context is a sub-context of the problem context
bool subsumedbyProb(Environ const & env) { return env.subsumedbyProb(); }
// Return sub-contexts of env.
pEnvs const & subEnvs(Environ const & env) { return env.psubEnvs(); }
// Return super-contexts of env.
pEnvs const & supEnvs(Environ const & env) { return env.psupEnvs(); }

// Moves generated at a given stage
Moves Environ::ourmoves(Game const & game, stage_t stage) const
{
    if (game.goal().ast.empty())
        game.goal().ast = ast(game.goal().RPN);
// if (stage >= 5)
// std::cout << "Finding moves at stage " << stage << " for " << game;
    Moves moves;

    Assiters const & assvec = database.assiters();
    Assiters::size_type limit = std::min(assertion.number, prob().numberlimit);
    for (Assiters::size_type i = 1; i < limit; ++i)
    {
        Assertion const & ass = assvec[i]->second;
        if ((ass.type & Asstype::USELESS) || !ontopic(ass))
            continue; // Skip non propositional theorems.
        if (stage == 0 || (ass.nfreevar() > 0 && stage >= ass.nfreevar()))
            if (trythm(game, assvec[i], stage, moves))
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
    switch (valid(move))
    {
    case MoveCLOSED:
        moves.assign(1, move);
        return true;
    case MoveVALID:
        // std::cout << move << std::endl;
        // std::cout << move.substitutions;
        moves.push_back(move);
        // std::cin.get();
        return false;
    default:
        return false;
    }
}

// Add abstraction moves. Return true if it has no open hypotheses.
bool Environ::addabsmoves(Goal const & goal, pAss pthm, Moves & moves) const
{
    Assertion const & thm = pthm->second;
    if (thm.expRPN == goal.RPN)
        return false;
    // return false;
    if (!goal.maxrangescomputed)
        goal.maxranges = maxranges(goal.RPN, goal.ast);
    SteprangeAST thmexp(thm.expRPN, thm.expAST.begin());
    SteprangeAST goalexp(goal.RPN, goal.ast.begin());
    FOR (GovernedSteprangesbystep::const_reference rstep, thm.expmaxranges)
    {
        GovernedSteprangesbystep::const_iterator const iter
        = goal.maxranges.find(rstep.first);
        if (iter == goal.maxranges.end())
            continue;
        std::cout << thm.expression;
        std::cout << goal.expression();
        FOR (GovernedStepranges::const_reference thmrange, rstep.second)
        {
            Proofsteps thmrangeRPN(thmrange.first.first, thmrange.first.second);
            AST const & thmrangeAST(ast(thmrangeRPN));
            SteprangeAST const thmrangeRPNAST(thmrangeRPN, thmrangeAST);
            FOR (GovernedStepranges::const_reference goalrange, iter->second)
            {
                Proofsteps goalrangeRPN(goalrange.first.first, goalrange.first.second);
                AST const & goalrangeAST(ast(goalrangeRPN));
                SteprangeAST const goalrangeRPNAST(goalrangeRPN, goalrangeAST);
                Stepranges subst(thm.maxvarid() + 1);
                if (findsubstitutions(goalrangeRPNAST, thmrangeRPNAST, subst))
                {
                    Move const substmove(pthm, subst);
                    std::cout << substmove.goal().expression();
                }
            }
        }
        std::cin.get();
        // FOR (GovernedStepranges::const_reference rrange, rstep.second)
        // {
        //     Move::Conjectures conjs(2);
        //     Bank & bank = pProb->bank;
        //     if (skeleton(goalexp, Keeprange(rrange.first), bank, conjs[1].RPN) == TRUE)
        //     {
        //         conjs[1].typecode = goal.typecode;
        //         skeleton(thmexp, Keeprange(rrange.first), bank, conjs[0].RPN);
        //         conjs[0].typecode = thm.exptypecode();
        //         std::cout << conjs[0].expression() << conjs[1].expression();
        //         Move move(conjs, bank);
        //         std::cout << valid(move);
        //         std::cin.get();
        //     }
        // }
    }
    
    return false;
}

// Return true if all variables in use have been substituted.
bool allvarsfilled(Varusage const & varusage, Stepranges const & subst)
{
    FOR (Varusage::const_reference rvar, varusage)
        if (Symbol2::ID id = rvar.first)
            if (subst[id].first == subst[id].second)
                return false;
    return true;
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
            substack[hypstack.size()] = substack[hypstack.size() - 1];
            if (asshyp == ass.hypcount())
                return delta; // No new match
            if (findsubstitutions
                (ass.hypRPNAST(asshyp), thm.hypRPNAST(thmhyp),
                 substack[hypstack.size()]))
                return ++delta; // New match
        }
        hypstack.pop_back();
    }
    return STACKEMPTY;
}

// Add Hypothesis-oriented moves.
// Return true if it has no open hypotheses.
bool Environ::addhypmoves(pAss pthm, Moves & moves,
                          Stepranges const & substitutions,
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
    substack[0] = substitutions;
    // substack.size() == hypstack.size() + 1
    // # matched hypotheses
    Hypsize matchedhyps = 0;
    int delta;
    do
    {
        // std::cout << hypstack;
        if (allvarsfilled(thm.varusage, substack[hypstack.size()]))
        {
            Move move(pthm, substack[hypstack.size()]);
            switch (valid(move))
            {
            case MoveCLOSED:
                moves.assign(1, move);
                return true;
            case MoveVALID:
                // std::cout << move << std::endl;
                // std::cout << move.substitutions;
                moves.push_back(move);
                // std::cin.get();
                break;
            default:
                break;
            }
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
bool Environ::addhypmoves(pAss pthm, Moves & moves,
                          Stepranges const & substitutions) const
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
            Stepranges newsubstitutions(substitutions);
            if (findsubstitutions
                (assertion.hypRPNAST(asshyp), thm.hypRPNAST(thmhyp),
                 newsubstitutions))
            {
// std::cout << assertion.hyplabel(asshyp) << ' ' << assertion.hypexp(asshyp);
                Move move(pthm, newsubstitutions);
                switch (valid(move))
                {
                case MoveCLOSED:
                    moves.assign(1, move);
                    return true;
                case MoveVALID:
                    // std::cout << move << std::endl;
                    // std::cout << move.substitutions;
                    moves.push_back(move);
                    // std::cin.get();
                    break;
                default:
                    break;
                }
            }
        }
    }
    return false;
}

// Try applying the theorem, and add moves if successful.
// Return true if a move closes the goal.
bool Environ::trythm
    (Game const & game, Assiter iter, Proofsize size, Moves & moves) const
{
    Assertion const & thm = iter->second;
    Goal const & goal = game.goal();
    if (thm.expression.empty() || thm.exptypecode() != goal.typecode)
        return false; // Type code mismatch
// std::cout << "Trying " << iter->first << " with " << goal.expression();
    if (thm.esshypcount() == 0)
        addabsmoves(goal, &*iter, moves);
    Stepranges stepranges(thm.maxvarid() + 1);
    if (!findsubstitutions(goal, thm.expRPNAST(), stepranges))
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

// Validate a move applying a theorem.
Environ::MoveValidity Environ::validthmmove(Move const & move) const
{
    // Check if all goals of the move are proven.
    bool allproven = true;
    // Record the hypotheses.
    move.subgoals.resize(move.subgoalcount());
    for (Hypsize i = 0; i < move.subgoalcount() - move.isconj(); ++i)
    {
        if (move.subgoalfloats(i))
            continue;
        // Add the essential hypothesis as a goal.
        pGoal const pgoal = pProb->addgoal(move.subgoal(i), *this, GOALNEW);
// std::cout << "Validating " << pgoal->second.goal().expression();
        Goalstatus & s = pgoal->second.getstatus();
        if (s == GOALFALSE) // Refuted
            return MoveINVALID;
        // Check if the goal is proven.
        if (pgoal->second.proven())
        {
            move.subgoals[i] = pgoal;
            continue;
        }
        // Not proven
        allproven = false;

        if (s >= GOALOPEN) // Valid
        {
            move.subgoals[i] = addsimpgoal(pgoal);
            continue;
        }
        // New goal (s == GOALNEW)
        Goal const & goal = pgoal->second.goal();
// std::cout << "New goal " << goal.expression();
        s = status(goal);
        if (s == GOALFALSE) // Refuted
            return MoveINVALID;
// std::cout << "Simplifying " << goal.expression();
        Environ const * & psimpEnv = pgoal->second.psimpEnv;
        psimpEnv = pProb->addsubEnv(*pgoal->first, hypstotrim(goal));
        // Record the goal in the hypotheses of the move.
        move.subgoals[i] = addsimpgoal(pgoal);
// std::cout << name << "\n->\n" << (psimpEnv ? psimpEnv->name : "") << std::endl;
    }
    return allproven ? MoveCLOSED : MoveVALID;
}

Environ::MoveValidity Environ::validconjmove(Move const & move) const
{
    if (move.absconjs.empty())
        return MoveINVALID;

    MoveValidity const validity = validthmmove(move);
    if (validity == MoveINVALID)
        return MoveINVALID;

    Environ const * const penv = pProb->addsupEnv(*this, move);
    // Add the essential hypothesis as a goal.
    pGoal const pgoal = pProb->addgoal(move.absconjs.back(), *penv, GOALNEW);
// std::cout << "Validating " << pgoal->second.goal().expression();
    Goalstatus & s = pgoal->second.getstatus();
    if (s == GOALFALSE) // Refuted
        return MoveINVALID;
    // Check if the goal is proven.
    if (pgoal->second.proven())
        return move.subgoals.back() = pgoal, validity;
    // Not proven
    if (s >= GOALOPEN)
        return move.subgoals.back() = addsimpgoal(pgoal), MoveVALID;
    // New goal (s == GOALNEW)
    Goal const & goal = pgoal->second.goal();
// std::cout << "New goal " << goal.expression();
    s = penv->status(goal);
    if (s == GOALFALSE) // Refuted
        return MoveINVALID;
// std::cout << "Simplifying " << goal.expression();
    Environ const * & psimpEnv = pgoal->second.psimpEnv;
    psimpEnv = pProb->addsubEnv(*pgoal->first, penv->hypstotrim(goal));
    // Record the goal in the hypotheses of the move.
    move.subgoals.back() = addsimpgoal(pgoal);
// std::cout << penv->name << "\n->\n" << (psimpEnv ? psimpEnv->name : "") << std::endl;
    return MoveVALID;
}

// Add an item to an ordered vector if not already present.
template <typename T>
static void additeminorder(std::vector<T> & vec, T const & item)
{
    typename std::vector<T>::iterator const iter =
        std::lower_bound(vec.begin(), vec.end(), item, std::less<T>());
    if (iter == vec.end() || *iter != item)
        vec.insert(iter, item);
}

// Add env to context implication relations, given comparison result.
void Environ::addEnv(Environ const & env, int cmp) const
{
    if (cmp == 1)
        additeminorder(m_psubEnvs, &env);
    else if (cmp == -1)
        additeminorder(m_psupEnvs, &env);
}

// Compare two hypiters by address.
static int comphypiters(Hypiter x, Hypiter y)
{
    return std::less<pHyp>()(&*x, &*y);
}

// Return true if x includes y.
template <class C, class Comp>
static bool includes(C const & x, C const & y, Comp comp)
{
    return std::includes(x.begin(), x.end(), y.begin(), y.end(), comp);
}

// Compare contexts. Return -1 if *this < env, 1 if *this > env, 0 if not comparable.
int Environ::compare(Environ const & env) const
{
    if (assertion.hypcount() == env.assertion.hypcount())
        return 0;

    Hypiters xhypiters(assertion.hypiters);
    Hypiters yhypiters(env.assertion.hypiters);
    std::sort(xhypiters.begin(), xhypiters.end(), comphypiters);
    std::sort(yhypiters.begin(), yhypiters.end(), comphypiters);

    if (assertion.hypcount() > env.assertion.hypcount() &&
        includes(xhypiters, yhypiters, comphypiters))
        return 1; // x

    if (assertion.hypcount() < env.assertion.hypcount() &&
        includes(yhypiters, xhypiters, comphypiters))
        return -1; // x < y

    return 0; // Not comparable
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
