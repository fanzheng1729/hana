#include "goaldata.h"

// Check if all p's open children are present.

bool Nodeptrs::haschildren(Nodeptr p) const
{
    FOR (Nodeptr child, *p.children())
        if (!child->game().goaldata().proven() && !count(child))
            return false;
    return true;
}

// Return a node that can be inferred.
// Return NULL if there is not any.
Nodeptr Nodeptrs::pop()
{
    Nodeptrs parentschecked;
    FOR (Nodeptr p, *this)
    {
        // Parent of the node
        Nodeptr parent = p.parent();
        if (parentschecked.count(parent))
            continue;
        // New open parent
        parentschecked.insert(parent);
        // Check if all its open children are present.
        if (haschildren(parent))
        {
            // All open children present. Clear all children.
            clearchildren(parent);
            // Add the parent.
            if (!parent->game().goaldata().proven())
            {
                insert(parent = parent.parent());
                return parent;
            }
        }
    }
    // No new node added
    return Nodeptr();
}