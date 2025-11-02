#include "goaldata.h"

// Return a node that can be inferred.
// Return NULL if there is not any.
Nodeptr Nodeptrs::pop()
{
    Nodeptrs parentschecked;
    FOR (Nodeptr p, *this)
    {
        // Parent of the node
        Nodeptr const parent = p.parent();
        Goaldata const & goaldata = parent->game().goaldata();
        if (parentschecked.count(parent))
            continue;
        // New parent
        parentschecked.insert(parent);
        // Check if all its open children are present.
        bool allchildrenhere = !goaldata.proven();
        if (allchildrenhere)
            FOR (Nodeptr child, *parent.children())
                if (!child->game().goaldata().proven() && !count(child))
                    allchildrenhere = false;
        // All open children present
        if (allchildrenhere)
        {
            // Clear all children.
            FOR (Nodeptr child, *parent.children())
                erase(child);
            // Infer parent.
            insert(parent.parent());
            return parent.parent();
        }
    }
    // No inference possible
    return Nodeptr();
}