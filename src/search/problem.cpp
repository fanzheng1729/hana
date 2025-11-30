#include "goaldata.h"
#include "problem.h"

// UCB threshold for generating a new batch of moves
// Change this to turn on staged move generation.
Value Problem::UCBnewstage(pNode p) const
{
    if (!(staged & STAGED))
        return MCTS<Game>::UCBnewstage(p);
    // Staged
    if (!isourturn(p))
        return std::numeric_limits<Value>::max();
    // Our turn
    Game const & game = p->game();
    stage_t const stage = p->stage();
    Proofsize size = game.env().hypslen + game.goal().size() + stage;
    size_type const self = static_cast<size_type>(1) << (stage * 2);
    return score(size) + UCBbonus(true, p.size(), self);
}

// Evaluate the leaf. Return {value, sure?}.
// p should != NULL.
Eval Problem::evalleaf(pNode p) const
{
    Game const & game = p->game();
    if (game.attempt.type == Move::THM)
    {
        bool loops(pNode p);
        if (loops(p))
            return EvalLOSS;
    }

    if (!isourturn(p))
        return evaltheirleaf(p);
    // Our leaf
    if (game.proven())
        return EvalWIN;
    if (ranksimplerthanProb(game))
        return ALMOSTWIN;
    return game.env().evalourleaf(game);
}

Eval Problem::evaltheirleaf(pNode p) const
{
    Value value = const_cast<Problem *>(this)->singularext(p);
    if (value == WDL::WIN)
    {
        if (!p->game().writeproof())
            return EvalLOSS;
        const_cast<Problem *>(this)->closenodes(p);
        return EvalWIN;
    }
    // value is between WDL::LOSS and WDL::WIN.
    if (!p->game().nDefer)
        FOR (pNode child, *p.children())
            addpNode(child);
    return Eval(value, false);
}

// Copy proof of the game to other contexts.
void Problem::copyproof(Game const & game)
{
    if (!game.proven() || game.env().subsumedbyProb())
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
            if (supiter != supend && *supiter == &otherEnv)
            {
                // Super-context found. Copy proof.
                goaldata.second.proofdst() = game.proof();
                goaldata.second.setstatustrue();
                closenodesexcept(goaldata.second.pnodes());
            }
            if (supiter == supend) break;
        }
}

// Add a sub-context with hypotheses trimmed.
// Return pointer to the new context. Return NULL if unsuccessful.
Environ const * Problem::addsubEnv(Environ const & env, Bvector const & hypstotrim)
{
    if (hypstotrim.empty())
        return NULL;
    // Name of new context
    std::string const & name(env.assertion.hypslabel(hypstotrim));
    // Try add the context.
    std::pair<Environs::iterator, bool> const result =
    environs.insert(std::pair<strview, Environ const *>(name, NULL));
    // Iterator to the new context
    Environs::iterator const newEnviter = result.first;
    // If it already exists, set the game's context pointer.
    if (!result.second)
        return newEnviter->second;
    // If it does not exist, add the simplified assertion.
    Assertion & subAss = assertions[newEnviter->first];
    if (!subAss.expression.empty())
        return NULL;
    Assertion const & ass = env.makeAss(hypstotrim);
    if (ass.expression.empty())
        return NULL;
    // Add the sub-context.
    Environ * const psubEnv = env.makeEnv(subAss = ass);
    if (psubEnv)
    {
        psubEnv->pProb = this;
        psubEnv->m_subsumedbyProb = (probEnv().compare(*psubEnv) == 1);
        psubEnv->m_rankssimplerthanProb
        = database.syntaxDAG().simplerthan(psubEnv->maxranks, maxranks);
        newEnviter->second = psubEnv;
        addhypproofs(*psubEnv);
        updateimplication(*psubEnv);
    }
    return psubEnv;
}
