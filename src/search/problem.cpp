#include "problem.h"

// Copy proof of the game to other contexts.
void Problem::copyproof(Game const & game)
{
    if (game.goaldatas().proven() || !game.proven())
        return;
    // Super-contexts
    pEnvs const & psupEnvs = supEnvs(game.env());
    pEnvs::const_iterator supiter = psupEnvs.begin();
    pEnvs::const_iterator const supend = psupEnvs.end();
    // Loop through super-contexts.
    FOR (Goaldatas::reference goaldata, game.goaldatas())
        if (!goaldata.second.proven())
        {
            Environ const & otherEnv = *goaldata.first;
            supiter = std::lower_bound(supiter, supend, &otherEnv, less);
            // while (supiter != supend && less(*supiter, &otherEnv))
            //     ++supiter;
            if (supiter == supend)
                break;  // end reached
            if (*supiter != &otherEnv)
                continue;
            // Super-context found. Copy proof.
            goaldata.second.proofdst() = game.proof();
            goaldata.second.settrue();
            closenodesexcept(goaldata.second.pnodes());
        }
}

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
