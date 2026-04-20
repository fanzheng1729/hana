#include "../io.h"
#include "problem.h"

// Add node pointer to p's goal data.
void addpNode(pNode p)
{
    if (!p->game().proven())
        p->game().goaldata().m_pnodes.insert(p);
}

// UCB threshold for generating a new batch of moves
// Change this to turn on staged move generation.
Value Problem::UCBnewstage(pNode p) const
{
    if (!staged)
        return MCTS<Game>::UCBnewstage(p);
    // Staged
    if (!isourturn(p))
        return std::numeric_limits<Value>::max();
    // Our turn
    Treesize const self = static_cast<Treesize>(1) << (p->stage()*2);
    return score(p->game().env().weight(p->game()) + p->stage())
            + UCBbonus(true, p.size(), self);
}

// Do singular extension. Return the value.
// p should != nullptr.
Value Problem::singularext(pNode p)
{
    if (isourturn(p)) return value(p);
    // Their turn
    Value value = WDL::WIN;
    if (expand<&Game::moves>(p))
    {
        evalnewleaves(p);
        value = minimax(p);
    }
    return value;
}

// Evaluate the leaf. Return {value, sure?}.
// p should != nullptr.
Eval Problem::evalleaf(pNode p) const
{
    Game const & game = p->game();
    if (game.attempt.type == Move::THM)
    {
        bool loops(pNode p);
        if (loops(p))
            return EvalLOSS;
    }

    // if (isourturn(p) && p.parent() && p.parent()->game().attempt.isconj())
    //     navigate(p);
    return !isourturn(p) ? evaltheirleaf(p) :
    // Our leaf
        game.proven() ? EvalWIN :
        ranksimplerthanProb(game) ? ALMOSTWIN :
        game.env().evalourleaf(game);
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

// Test proof search. Return tree.size if okay. Return 0 if not.
Treesize testsearch(Assiter iter, Problem & tree, Treesize maxsize)
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
    else if (unexpected(tree.value() < WDL::WIN, "game value", tree.value()))
        return tree.navigate(), 0;
    else if (unexpected(!tree.checkproof(iter), "wrong proof", tree.proof()))
        return tree.navigate(), 0;
    else if (iter->first == "biass_")
        tree.writeproof((std::string(iter->first) + ".txt").c_str());
    return tree.size();
}
