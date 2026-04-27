#include "../database.h"
#include "environ.h"
#include "../io.h"
#include "goaldata.h"
#include "problem.h"
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
    std::cerr << "Bad goal\n" << badRPN << "in env " << label;
    std::cerr << " in Problem #" << assnum() << std::endl;
    // std::cin.get();
    return GOALFALSE;
}

// Validate a move applying a theorem.
Environ::MoveValidity Environ::validthmmove(Move const & move) const
{
    if (!pProb)
        return MoveINVALID;
    if (prob().mdatabase.typecodes().isprimitive(move.goaltypecode()) != FALSE)
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
//     std::cout << label << "\n->\n" << (psimpEnv ? psimpEnv->label : "") << std::endl;
        }
        // Record the goal in the hypotheses of the move.
        move.subgoals[i] = addsimpgoal(pgoal);
    }
    return allproven ? MoveCLOSED : MoveVALID;
}

Environ::MoveValidity Environ::validconjmove(Move const & move) const
{
// std::cout << "Validating conjecture move in env " << label << std::endl;
    MoveValidity const validity = validthmmove(move);
    if (validity == MoveINVALID)
        return MoveINVALID;
// return MoveINVALID;
    // Super-context corresponding to the CONJ move
    Environ const * const penv = pProb->addsupEnv(*this, move);
    // The goal
    pGoal const pgoal = pProb->addgoal(move.absconjs.back(), *penv, GOALNEW);
// std::cout << "Validating " << pgoal->second.goal().expression();
// std::cout << "In env " << penv->label << std::endl;
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
// std::cout << penv->label << "\n->\n" << (psimpEnv ? psimpEnv->label : "") << std::endl;
    return MoveVALID;
}

// Return true if a proof is legal.
bool Environ::legal(RPN const & proof) const
{
    if (!pProb)
        return false;

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

// Return true if x includes y.
template <class C, class Comp>
static bool includes(C const & x, C const & y, Comp comp)
{
    return std::includes(x.begin(), x.end(), y.begin(), y.end(), comp);
}

// Return true if hypotheses of *this contains those of env.
bool Environ::implies(Environ const & env) const
{
    return nhyps() > env.nhyps() &&
            includes(sortedhyps, env.sortedhyps, comphypiter);
}
