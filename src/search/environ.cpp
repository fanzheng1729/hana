#include <algorithm>    // for ordered algorithms
#include "../proof/analyze.h"
#include "environ.h"
#include "../io.h"
#include "goaldata.h"
#include "problem.h"
#include "../util/progress.h"

// Return true if the context is a sub-context of the problem context
bool subsumedbyProb(Environ const & env) { return env.subsumedbyProb(); }
// Return sub-contexts of env.
pEnvs const & subEnvs(Environ const & env) { return env.psubEnvs(); }
// Return super-contexts of env.
pEnvs const & supEnvs(Environ const & env) { return env.psupEnvs(); }

// Return true if all variables in use have been substituted.
static bool allvarsfilled(Varusage const & varusage, Stepranges const & subst)
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

        if (pgoal->second.proven())
        {
            move.subgoals[i] = pgoal;
            continue;
        }
        // Goal not proven
        allproven = false;

        if (s >= GOALOPEN) // Valid
        {
            move.subgoals[i] = addsimpgoal(pgoal);
            continue;
        }

        Goal const & goal = pgoal->second.goal();
        s = status(goal);
        if (s == GOALFALSE) // Refuted
            return MoveINVALID;
// if (!hypstotrim(goal).empty())
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
// std::cout << "In env " << penv->name << std::endl;
    Goalstatus & s = pgoal->second.getstatus();
    if (s == GOALFALSE) // Refuted
        return MoveINVALID;

    if (pgoal->second.proven())
        return move.subgoals.back() = pgoal, validity;
    // Goal not proven
    if (s >= GOALOPEN)
        return move.subgoals.back() = addsimpgoal(pgoal), MoveVALID;

    Goal const & goal = pgoal->second.goal();
// std::cout << "New goal when validating conj move " << goal.expression();
    s = penv->status(goal);
    if (s == GOALFALSE) // Refuted
        return MoveINVALID;
// if (!penv->hypstotrim(goal).empty())
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
