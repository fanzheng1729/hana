#include "problem.h"
#include "../proof/skeleton.h"
#include "../io.h"
#include "../util/filter.h"

Database const * Problem::pDB;

// Add 1-step proofs of all hypotheses of a context.
Environ const & Problem::addhypproofs(Environ const & env)
{
    if (&env != &probEnv() && env.subsumedbyProb())
        return env; // Skip contexts properly subsumed by the problem context.
    // env is either problem context or not subsumed by it.
    Assertion const & ass = env.assertion;
    for (Hypsize i = 0; i < ass.nhyps(); ++i)
    {
        if (ass.hypfloats(i)) continue;
        addgoal(Goalview(ass.hypRPN(i), ass.hyptypecode(i)), env, GOALTRUE)
        ->second.proofdst().assign(1, ass.hypptr(i));
    }
    return env;
}

// Add implication relation for newly added context. Return env.
Environ const & Problem::addimps(Environ const & env)
{
    if (env.subsumedbyProb()) return env;
    pEnvs const & psubEnvs(envsbyhyp.psubEnvs(env.sortedhyps));
    FOR (Environ const * poldenv, psubEnvs)
        env.addimps(*poldenv);
    pEnvs const & psupEnvs = envsbyhyp.psupEnvs(env.sortedhyps);
    FOR (Environ const * poldenv, psupEnvs)
        poldenv->addimps(env);
    return env;
}

static Hypiters filterauxilliary(Hypiters const & hypiters)
{
    Hypiters result;
    FOR (Hypiter iter, hypiters)
        if (!islabeltoken(iter->first.c_str))
            result.push_back(iter);
    return result;
}

// Initialize a context if existent. Return its pointer.
Environ const * Problem::initEnv(Environ * p)
{
    if (!p) return p;

    p->pProb = this;
    p->m_subsumedbyProb = nEnvs() <= 1 || probEnv().implies(*p);
    updateimps(*p);

    if (!p->subsumedbyProb())
        envsbyhyp.addkeys(filterauxilliary(p->sortedhyps), p);

    return &addimps(addhypproofs(*p));
}

// Add a sub-context with hypotheses trimmed.
// Return pointer to the new context. Return nullptr if unsuccessful.
Environ const * Problem::addsubEnv(Environ const & env, Bvector const & hypstotrim)
{
    if (!util::filter(hypstotrim)(true))
        return Environs::mapped_type();
    // Hypiters of new context
    Hypiters const & hypiters(env.assertion.sortedhyps(hypstotrim));
    // Try add the context.
    std::pair<Enviter, bool> const result =
    environs.insert(std::make_pair(hypiters, Environs::mapped_type()));
    // Iterator to the new context
    Enviter const newEnviter = result.first;
    if (!result.second) // already added
        return newEnviter->second;
    // Simplified assertion
    Assertion & subAss = assertions[assertions.size()];
    // std::cout << "addsubEnv to " << env.label << ' ' << env.assertion.varusage;
    Assertion const & ass(env.assertion.makeAss(hypstotrim));
    // Pointer to the sub-context
    return newEnviter->second = initEnv(env.makeEnv(subAss = ass));
}

// Add a super-context with hypotheses trimmed.
// Return pointer to the new context. Return nullptr if unsuccessful.
Environ const * Problem::addsupEnv(Environ const & env, Move const & move)
{
    Expression const & newvars(move.absvars(bank));
    Hypiters const & newhyps(move.addconjsto(bank));
    // Hypiters of new context
    Hypiters const & hypiters(env.assertion.sortedhyps(newvars, newhyps));
    // Try add the context.
    std::pair<Enviter, bool> const result =
    environs.insert(std::make_pair(hypiters, Environs::mapped_type()));
    // Iterator to the new context
    Enviter const newEnviter = result.first;
    if (!result.second) // already added
        return newEnviter->second;
    // Simplified assertion
    Assertion & supAss = assertions[assertions.size()];
// std::cout << "addsupEnv to " << env.label << ' ' << newvars;
// std::cout << "env vars " << env.assertion.varusage;
    supAss.number = env.assnum();
    supAss.sethyps(env.assertion, newvars, newhyps);
    supAss.disjvars = move.findDV(supAss);
    // Pointer to the super-context
    return newEnviter->second = initEnv(env.makeEnv(supAss));
}

// Create an abstraction move.
Move Problem::absmove
    (Goal const & goal, Absubstmove const & absubstmove,
     RPNspanAST goalsubexp)
{
    Assertion const & thm = absubstmove.first.pthm->second;
    if (goal.rpn == thm.expRPN && goal.typecode == thm.exptypecode())
        return Move::NONE;

    // Abstract variable name
    Bank1var const absvar = bank.addabsvar(goalsubexp.first);
    // Abstract move
    Move move(2, absvar.id + 1);
    // Abstract variable RPN
    move.substitutions.back() = goalsubexp.first;
    // 1 conjecture + 1 goal
    Move::Conjectures & conjs = move.absconjs;
    // Conjecture
    conjs[0].rpn = absubstmove.second;
    conjs[0].typecode = thm.exptypecode();
    // Goal
    skeleton(goal, Keepspan(goalsubexp.first), absvar, conjs[1].rpn);
    conjs[1].typecode = goal.typecode;
    // move.printconj();
    return move;
}
