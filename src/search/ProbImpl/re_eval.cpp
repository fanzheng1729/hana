#include "../problem.h"

// Refocus the tree on simpler sub-tree, if almost won.
void Problem::re_eval()
{
    if (value() != ALMOSTWIN)
        return;
    // printranksinfo();
    numberlimit = maxranknumber;
    maxranks.clear();
    prune(root());
    updateimps();
    focus(root());
    maxranknumber = database.syntaxDAG().maxranknumber(maxranks);
    // printranksinfo();
}

// Add the ranks of a node to maxranks, if almost won.
void Problem::addranks(pNode p)
{
    if (value(p) < ALMOSTWIN)
        return;
    database.syntaxDAG().addranks(maxranks, p->game().env().maxranks);
    database.syntaxDAG().addexp(maxranks, p->game().goal().RPN);
}

// Prune the sub-tree at p and update maxranks, if almost won.
void Problem::prune(pNode p)
{
    if (p.haschild())
    {
        FOR (pNode child, *p.children())
            prune(child);
        seteval(p, minimax(p));
        if (p->won() && !p->game().proven())
            std::cout << "prune" << std::endl, navigate(p);
    }
    else if (value(p) < ALMOSTWIN)
        setalmostloss(p);
    else
        addranks(p);
}

// Update implications after problem context is simplified.
void Problem::updateimps()
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
        // if (p->won() && !p->game().proven())
        //     std::cout << "prune" << std::endl, navigate(p);
    }
    else seteval(p, evalleaf(p));
}
