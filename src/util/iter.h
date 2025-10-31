#ifndef ITER_H_INCLUDED
#define ITER_H_INCLUDED

#include <iterator> // for std::inserter

// Inserter to the end of a container
template<class Container>
std::insert_iterator<Container> end_inserter(Container & container)
{
    return std::inserter(container, container.end());
}

#endif // ITER_H_INCLUDED
