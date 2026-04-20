#include "problem.h"
#include "../proof/skeleton.h"

// Initialize a context if existent. Return its pointer.
Environ const * Problem::initEnv(Environ * p)
{
    if (!p) return p;
    p->pProb = this;
    p->m_subsumedbyProb = environs.size()<=1 || (probEnv().compEnv(*p)==1);
    p->updateimps(maxranks);
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

static void addabsubst
    (RPNspanAST const abs, pAss pthm, Moves & moves)
{
    if (!pthm)
        return;
        
    Assertion const & thm = pthm->second;
    RPNspans subst(thm.maxvarid() + 1);
    GovernedRPNspansbystep::const_iterator const iter =
    thm.expmaxabs.find(abs.first.root());
    if (iter == thm.expmaxabs.end())
        return;
    
    FOR (GovernedRPNspans::const_reference thmabs, iter->second)
    {
        subst.assign(thm.maxvarid() + 1, RPNspan());
        if (findsubst(abs, RPNspanAST(thmabs.first, thmabs.second), subst))
            moves.push_back(Move(pthm, subst));
    }
}

// Create an abstract move.
Move Problem::absmove
    (Goal const & goal, Goal const & conj, RPNspan const absRPN)
{
    if (absRPN.empty())
        return Move::NONE;

    // Abstract variable name
    Bank1var const absvar = bank.addabsvar(absRPN);
    // Abstract move
    Move move(2, absvar.id + 1);
    // Abstract variable RPN
    move.substitutions.back() = absRPN;
    // 1 conjecture + 1 goal
    AST  const & conjAST(ast(conj.rpn));
    RPNspanAST const conjexp(conj.rpn, conjAST);
    RPNspanAST const goalexp(goal.rpn, goal.ast);
    Move::Conjectures & conjs = move.absconjs;
    if (skeleton(conjexp, Keepspan(absRPN), absvar, conjs[0].rpn) != TRUE ||
        skeleton(goalexp, Keepspan(absRPN), absvar, conjs[1].rpn) != TRUE)
        return Move::NONE;
    // Their typecodes
    conjs[0].typecode = conj.typecode;
    conjs[1].typecode = goal.typecode;

    absRPNs[absRPN];

    return move;
}
