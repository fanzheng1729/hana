#include "../problem.h"

// Focus on simpler contexts.
void Problem::focusenvs()
{
    FOR (Environs::const_reference env, environs)
        env.second->m_rankssimplerthanProb
        = database.syntaxDAG().simplerthan
        (env.second->maxranks, maxranks);
}

// Focus the sub-tree at p, with updated maxranks, if almost won.
void Problem::focus(pNode p)
{
    if (value(p) < ALMOSTWIN)
        return;
    if (p.haschild())
    {
        FOR (pNode child, *p.children())
            focus(child);
        seteval(p, minimax(p));
    }
    else if (!p->game().env().rankssimplerthanProb())
        seteval(p, evalleaf(p));
}
