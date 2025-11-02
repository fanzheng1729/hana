#include "goaldata.h"

// Check if all p's open children are present.
bool Goalptrs::haschildren(Nodeptr p) const
{
    FOR (Nodeptr const child, *p.children())
    {
        Goalptr const childgoal = child->game().goalptr;
        if (childgoal->second.proven())
            continue;
        if (!count(childgoal))
            return false;
    }
    return true;
}

Goalptr Goalptrs::saturate()
{
    FOR (Goalptr const pgoal, *this)
        FOR (Nodeptr const pnode, pgoal->second.nodeptrs)
        {
            Nodeptr const parent = pnode.parent();
            Goalptr const newgoal = parent->game().goalptr;
            if (newgoal->second.proven())
                continue;
            if (haschildren(parent))
                if (insert(newgoal).second)
                    return newgoal;
        }
    return Goalptr();
}
