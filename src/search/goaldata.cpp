#include "goaldata.h"

// Return true if all open children of p are present.
bool Goalptrs::haschildren(Nodeptr p) const
{
    FOR (Nodeptr const child, *p.children())
    {
        Goalptr const childgoal = child->game().pgoal;
        if (childgoal->second.proven())
            continue;
        if (!count(childgoal))
            return false;
    }
    return true;
}

// Return pointer to a new goal implied by the existing goals.
// Return NULL if there is no such goal.
Goalptr Goalptrs::saturate()
{
    FOR (Goalptr const pgoal, *this)
        FOR (Nodeptr const pnode, pgoal->second.nodeptrs)
        {
            Nodeptr const parent = pnode.parent();
            Goalptr const newgoal = parent->game().pgoal;
            if (newgoal->second.proven())
                continue;
            if (haschildren(parent))
                if (insert(newgoal).second)
                    return newgoal;
        }
    return Goalptr();
}
