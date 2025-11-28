#include <algorithm>    // for std::find_if, std::count_if
#include <vector>
#include "util/arith.h"
#include "util/filter.h"
#include "util/find.h"
#include "util/for.h"
#include "ass.h"
#include "DAG.h"
#include "io.h"
#include "MCTS/tree.h"

template<class T> static T testlog2()
{
    T const static digits(std::numeric_limits<T>::digits);
    for (T d = 0, n = 1; d < digits; ++d, n *= 2)
    {
        if (util::log2(n) != d)
            return (std::cerr << util::log2(n) << ' ' << d, n);
        if (util::log2(T(2*n - 1)) != d)
            return (std::cerr << util::log2(T(2*n - 1)) << ' ' << d, 2*n - 1);
    }
    return 0;
}

static bool testlog2()
{
    const char static * const types[] = {"short", "int", "long"};
    const unsigned long results[] =
    {testlog2<unsigned short>(),testlog2<unsigned>(),testlog2<unsigned long>()};
    const unsigned n = sizeof(results) / sizeof(unsigned long);
    const unsigned long * perr
    = std::find_if(results, util::end(results), util::identity());
    const std::ptrdiff_t i = perr - results;
    if (i != n)
        std::cerr << "ERROR: " << types[i] << ' ' << results[i] << std::endl;
    return i != n; // should be 0
}

template<template<bool, class> class Filter, bool B, class Container, class A>
static bool checkfilter
    (Filter<B, Container> const & filter, A const & alien)
{
    Container const & container = filter.container;

    std::size_t const count
        (std::count_if(container.begin(), container.end(), filter));
    if (count < container.size())
    {
        std::cerr << "Only " << count << " items out of ";
        std::cerr << container.size() << " found by filter" << std::endl;
        return false;
    }

    if (filter(alien))
    {
        std::cerr << "Alien item found by filter" << std::endl;
        return false;
    }

    return true;
}

static bool testfilter(unsigned n)
{
    std::vector<unsigned> v;
    for (unsigned i = 0; i <= n; v.push_back(++i))
    {
        if (!checkfilter(util::filter(v), 0))
        {
            std::cerr << "filter check failed with " << i << " elements\n";
            return false;
        }
    }

    return true;
}

// Return true if iterators to assertions are sorted in ascending number.
bool checkassiters
    (Assertions const & assertions, Assiters const & assiters)
{
    for (Assiter iter = assertions.begin(); iter != assertions.end(); ++iter)
    {
        std::size_t const n(iter->second.number);

        if (n > assertions.size())
        {
            std::cerr << "Assertion " << iter->first << "'s number " << n;
            std::cerr << " is larger than the total number ";
            std::cerr << assertions.size() << std::endl;
            return false;
        }

        if (assiters[n] != iter)
        {
            std::cerr << "Assertion " << iter->first << "'s number " << n;
            std::cerr << " != its position ";
            std::cerr << util::find(assiters, iter)-assiters.begin() << std::endl;
            return false;
        }
    }

    return true;
}

bool pretest()
{
    std::cout << "Testing util" << std::endl;
    if (testlog2() || !testfilter(8)) return false;

    const char * testsat1(); // should be "Okay"
    unsigned testsat2(unsigned n); // should be 0
    std::cout << "Checking SAT: " << testsat1() << std::endl;
    if (testsat2(8) != 0) return false;
    
    std::cout << "Checking DAG" << std::endl;
    if (!testDAG(8)) return false;

    std::cout << "Testing tree" << std::endl;
    if (!chain1(8).check()) return false;

    bool testMCTS(std::size_t maxsize, double const exploration[2]); // should be 1
    std::cout << "Testing MTCS" << std::endl;
    static double const exploration[] = {1, 1};
    return testMCTS(1 << 20, exploration);
}
