#include "../io.h"
#include "problem.h"

// Test proof search. Return tree.size if okay. Return 0 if not.
Treesize testsearch(Assiter iter, Problem & tree, Treesize maxsize)
{
    // printass(*iter);
    tree.play(maxsize);
    // tree.printstats();
    // std::cin.get();
    // if (iter->first == "biluk")
    //     tree.navigate();
    if (tree.size() > maxsize)
    {
        // printass(*iter); std::cout << std::endl;
        // tree.printstats();
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
