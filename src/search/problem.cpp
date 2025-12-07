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
    size_type const self = static_cast<size_type>(1) << (p->stage()*2);
    return score(p->game().env().weight(p->game()) + p->stage())
            + UCBbonus(true, p.size(), self);
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
    if (!result.second) // already added
        return newEnviter->second;
    // Simplified assertion
    Assertion & subAss = assertions[newEnviter->first];
    if (subAss.number > 0)
        return NULL;
    Assertion const & ass = env.assertion.makeAss(hypstotrim);
    // Pointer to the sub-context
    Environ * const psubEnv = env.makeEnv(subAss = ass);
    newEnviter->second = psubEnv;
    initEnv(psubEnv);
    return psubEnv;
}

// Add a super-context with hypotheses trimmed.
// Return pointer to the new context. Return NULL if unsuccessful.
Environ const * Problem::addsupEnv(Environ const & env, Move const & move)
{
    Expression const & newvars(move.absvars(bank));
    Hypiters const & newhypiters(move.addconjsto(bank));
    for (Hypsize i = 0; i < newhypiters.size(); ++i)
        std::cout << newhypiters[i]->first << ' ',
        std::cout << newhypiters[i]->second.expression;
    // Name of new context
    std::string const & name(env.assertion.hypslabel(newvars, newhypiters));
    std::cout << name << std::endl;
    Assertion ass(assertion.number);
    ass.sethyps(assertion, newvars, newhypiters);
    std::cout << move.findDV(ass).size();
    std::cerr << "Not implemented" << std::endl;
    throw;
    // Try add the context.
    std::pair<Environs::iterator, bool> const result =
    environs.insert(std::pair<strview, Environ const *>(name, NULL));
    // Iterator to the new context
    Environs::iterator const newEnviter = result.first;
    if (!result.second) // already added
        return newEnviter->second;
    // Simplified assertion
    Assertion & subAss = assertions[newEnviter->first];
    if (subAss.number > 0)
        return NULL;
    return NULL;
}

// Test proof search.
// Return the size of tree if okay. Otherwise return 0.
Problem::size_type testsearch
    (Assiter iter, Problem & tree, Problem::size_type maxsize)
{
    // printass(*iter);
    tree.play(maxsize);
    // tree.printstats();
    // std::cin.get();
    // if (iter->first == "impbii")
    //     tree.navigate();
    // if (iter->second.number == 203)
    //     tree.printstats(), std::cin.get();
    if (tree.size() > maxsize)
    {
        // printass(*iter);
        // std::cout << std::endl;
        // tree.printstats();
        // tree.printenvs();
        // tree.navigate();
    }
    else if (unexpected(tree.empty(), "empty tree for", iter->first))
        return 0;
    else if (unexpected(tree.value() != 1, "game value", tree.value()))
        return tree.navigate(), 0;
    else if (unexpected(!checkconclusion(iter->first,
                                         verify(tree.proof(), &*iter),
                                         iter->second.expression),
                        "wrong proof", tree.proof()))
        return tree.navigate(), 0;
    else if (iter->first == "biass_")
        tree.writeproof((std::string(iter->first) + ".txt").c_str());
    return tree.size();
}
