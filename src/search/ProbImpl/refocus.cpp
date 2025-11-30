#include "../problem.h"

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
