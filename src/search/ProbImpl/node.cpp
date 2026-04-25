#include "../problem.h"

// Close all the nodes except p.
void Problem::closenodesexcept(pNodes const & pnodes, pNode p)
{
    FOR (pNode other, pnodes)
        if (other != p && !other->won())
        {
            setwin(other);
            pNode parent = other.parent();
            if (parent && !parent->won())
                backprop(parent);
        }
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

// Close all the nodes with p's proven goal.
void Problem::closenodes(pNode p)
{
    if (!p->game().proven()) return;
    closenodesexcept(p->game().goaldata().pnodes(), p);
    copyproof(p->game());
}

void Problem::backpropcallback(pNode p)
{
    if (p->game().proven())
        setwin(p); // Fix seteval in backprop.
    else if (p->won() && p->game().writeproof())
        closenodes(p);
}

// Called after each playonce()
void Problem::playoncecallback()
{
    reval();
    // checkmainline(root());
// navigate();
}
