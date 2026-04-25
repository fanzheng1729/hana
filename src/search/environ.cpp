#include "environ.h"
#include "../io.h"
#include "goaldata.h"
#include "problem.h"
#include "../util/algo.h"   // for util::addordered
#include "../util/filter.h"
#include "../util/progress.h"

// Return true if the context is a sub-context of the problem context
bool subsumedbyProb(Environ const & env) { return env.subsumedbyProb(); }
// Return sub-contexts of env.
pEnvs const & subEnvs(Environ const & env) { return env.psubEnvs(); }
// Return super-contexts of env.
pEnvs const & supEnvs(Environ const & env) { return env.psupEnvs(); }

// Report false goal and return GOALFALSE.
Goalstatus Environ::printbadgoal(RPN const & badRPN) const
{
    std::cerr << "Bad goal\n" << badRPN << "in env " << name;
    std::cerr << " in Problem #" << assnum() << std::endl;
    // std::cin.get();
    return GOALFALSE;
}

// Validate a move applying a theorem.
Environ::MoveValidity Environ::validthmmove(Move const & move) const
{
    if (database.typecodes().isprimitive(move.goaltypecode()) != FALSE)
        return MoveINVALID;
    if (!move.checkDV(assertion))
        return MoveINVALID;
    // True if all goals of the move are proven
    bool allproven = true;
    // Record the hypotheses.
    move.subgoals.resize(move.nsubgoals());
    for (Hypsize i = 0; i < move.nsubgoals() - move.isconj(); ++i)
    {
        if (move.subgoalfloats(i))
            continue;
        // Add the essential hypothesis as a goal.
        pGoal const pgoal = pProb->addgoal(move.subgoal(i), *this, GOALNEW);
// std::cout << "Validating " << pgoal->second.goal().expression();
        Goalstatus & s = pgoal->second.getstatus();
        if (s == GOALFALSE) // Refuted
            return MoveINVALID;

        if (pgoal->second.proven()) // Proven
        {
            move.subgoals[i] = pgoal;
            continue;
        }
        // Goal not proven
        allproven = false;

        if (s >= GOALOPEN)  // Valid
        {
            move.subgoals[i] = addsimpgoal(pgoal);
            continue;
        }

        Goal const & goal = pgoal->second.goal();
        s = status(goal);
        if (s == GOALFALSE) // Refuted
            return MoveINVALID;

        if (s == GOALTRUE)  // True
        {
            Environ const * & psimpEnv = pgoal->second.psimpEnv;
            psimpEnv = pProb->addsubEnv(*pgoal->first, hypstotrim(goal));
// if (psimpEnv)
//     std::cout << "Simplified " << goal.expression();
// if (psimpEnv)
//     std::cout << name << "\n->\n" << (psimpEnv ? psimpEnv->name : "") << std::endl;
        }
        // Record the goal in the hypotheses of the move.
        move.subgoals[i] = addsimpgoal(pgoal);
    }
    return allproven ? MoveCLOSED : MoveVALID;
}

Environ::MoveValidity Environ::validconjmove(Move const & move) const
{
// std::cout << "Validating conjecture move in env " << name << std::endl;
    MoveValidity const validity = validthmmove(move);
    if (validity == MoveINVALID)
        return MoveINVALID;
// return MoveINVALID;
    // Super-context corresponding to the CONJ move
    Environ const * const penv = pProb->addsupEnv(*this, move);
    // The goal
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

// Return true if a proof is legal.
bool Environ::legal(RPN const & proof) const
{
    Bank const & bank = prob().bank;
    Hypiter const end = bank.hypotheses().end();

    FOR (RPNstep const step, proof)
        if (step.isthm() && step.pass->second.number >= assnum())
            return false; // Theorem # too large
        else
        if (step.ishyp() && !step.phyp->second.floats)
        {
            Hypiter const iter = bank.findhyp(step.phyp->first);
            if (iter == end)
                continue; // Problem hypothesis
            if (subsumedbyProb())
                return false; // Non-problem hypothesis in sub-context
            if (!util::filter(assertion.hypiters)(iter))
                return false; // Not hypothesis in this context
        }
    return true;
}

// Add env to context implication relations, given comparison result.
void Environ::addEnv(Environ const & env, int cmp) const
{
    util::addordered(cmp == 1 ? m_psubEnvs : m_psupEnvs, &env);
}

// Return true if x includes y.
template <class C, class Comp>
static bool includes(C const & x, C const & y, Comp comp)
{
    return std::includes(x.begin(), x.end(), y.begin(), y.end(), comp);
}

// Compare contexts. Return -1 if *this < env, 1 if *this > env, 0 if not comparable.
int Environ::compEnv(Environ const & env) const
{
    if (nhyps == env.nhyps)
        return 0;

    Hypiters const & xhypiters = hypiters;
    Hypiters const & yhypiters = env.hypiters;
    
    if (nhyps > env.nhyps && includes(xhypiters, yhypiters, comphypiters))
        return 1; // *this > env

    if (nhyps < env.nhyps && includes(yhypiters, xhypiters, comphypiters))
        return -1; // *this < env

    return 0; // Not comparable
}

// Return true if an assertion duplicates a previous one.
static bool isduplicate(Assertion const & ass, Database const & db)
{
    static Value const zeros[2] = {};
    return !ass.testtype(Asstype::AXIOM + Asstype::TRIVIAL) &&
            Problem(Environ(ass, db, -1), zeros).playonce() == WDL::WIN;
}

// Mark duplicate assertions. Return its number.
Assertions::size_type Database::markduplicate()
{
    Assertions::size_type n = 0, ndups = 0;
    Progress progress;
    FOR (Assertions::reference rass, m_assertions)
    {
        // printass(rass);
        Assertion & ass = rass.second;
        bool const isdup = isduplicate(ass, *this);
        ass.settype(isdup * Asstype::DUPLICATE);
        // Check if it is duplicate and starts with a non-primitive type code.
        if (isdup && typecodes().isprimitive(ass.exptypecode()) == FALSE)
            ++ndups;
        progress << ++n/static_cast<Ratio>(assertions().size());
    }
    return ndups;
}
