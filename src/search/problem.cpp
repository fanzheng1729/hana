#include "problem.h"
#include "../proof/skeleton.h"
#include "../io.h"

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
    pEnvs const & psubEnvs = envsbyhyp.keyin(env.hypiters);
    FOR (Environ const * poldenv, psubEnvs)
        env.addimps(*poldenv);
    pEnvs const & psupEnvs = *allenvsbyhyp.hasall(env.hypiters);
    FOR (Environ const * poldenv, psupEnvs)
        poldenv->addimps(env);
    return env;
}

// Initialize a context if existent. Return its pointer.
Environ const * Problem::initEnv(Environ * p)
{
    if (!p) return p;

    p->pProb = this;
    p->m_subsumedbyProb =
    environs.size() <= 1 || probEnv().implies(*p);
    p->updateimps(maxranks);

    if (!p->m_subsumedbyProb)
    {
        Hypiters hyps;
        FOR (Hypiter const iter, p->hypiters)
            if (!islabeltoken(iter->first.c_str))
                hyps.push_back(iter);
        envsbyhyp.addkeys(hyps, p);
        allenvsbyhyp.addkeys(hyps, p);

        // pEnvs const * penvs = allenvsbyhyp.hasall(hyps);
        // std::cout << p->name << std::endl;
        // FOR (Environ const * q, *penvs)
        //     std::cout << q->name << ' ';
        // std::cin.get();
    }

    return &addimps(addhypproofs(*p));
}

// Add a sub-context with hypotheses trimmed.
// Return pointer to the new context. Return nullptr if unsuccessful.
Environ const * Problem::addsubEnv(Environ const & env, Bvector const & hypstotrim)
{
    if (hypstotrim.empty())
        return Environs::mapped_type();
    // Name of new context
    std::string const & name(env.assertion.hypslabel(hypstotrim));
    // Try add the context.
    std::pair<Environs::iterator, bool> const result =
    environs.insert(std::make_pair(name, Environs::mapped_type()));
    // Iterator to the new context
    Environs::iterator const newEnviter = result.first;
    if (!result.second) // already added
        return newEnviter->second;
    // Simplified assertion
    Assertion & subAss = assertions[newEnviter->first];
    if (subAss.number > 0)
        return Environs::mapped_type();
    // std::cout << "addsubEnv to " << env.name << ' ' << env.assertion.varusage;
    Assertion const & ass = env.assertion.makeAss(hypstotrim);
    // Pointer to the sub-context
    return newEnviter->second = initEnv(env.makeEnv(subAss = ass));
}

// Add a super-context with hypotheses trimmed.
// Return pointer to the new context. Return nullptr if unsuccessful.
Environ const * Problem::addsupEnv(Environ const & env, Move const & move)
{
    Expression const & newvars(move.absvars(bank));
    Hypiters const & newhyps(move.addconjsto(bank));
    // Name of new context
    std::string const & name(env.assertion.hypslabel(newvars, newhyps));
    // Try add the context.
    std::pair<Environs::iterator, bool> const result =
    environs.insert(std::make_pair(name, Environs::mapped_type()));
    // Iterator to the new context
    Environs::iterator const newEnviter = result.first;
    if (!result.second) // already added
        return newEnviter->second;
    // Simplified assertion
    Assertion & supAss = assertions[newEnviter->first];
    if (supAss.number > 0)
        return Environs::mapped_type();
// std::cout << "addsupEnv to " << env.name << ' ' << newvars;
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
