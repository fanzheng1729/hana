#include "../problem.h"

// Return true if proof() solves the problem *iter.
bool Problem::checkproof(Assiter iter) const
{
    if (countabs() > 4)
        printabs(), std::cout << countabs(), std::cin.get();
    return probEnv().legal(proof()) &&
        checkconclusion(iter->first,
                        verify(proof(), &*iter),
                        iter->second.expression);
}

// # goals of a given status
Goals::size_type Problem::countgoal(int status) const
{
    Goals::size_type n = 0;
    FOR (Goals::const_reference goaldatas, goals)
        FOR (Goaldatas::const_reference goaldata, goaldatas.second)
            n += (goaldata.second.getstatus() == status);
    return n;
}

// # proven goals
Goals::size_type Problem::countproof() const
{
    Goals::size_type n = 0;
    FOR (Goals::const_reference goaldatas, goals)
        FOR (Goaldatas::const_reference goaldata, goaldatas.second)
            n += goaldata.second.proven();
    return n;
}
