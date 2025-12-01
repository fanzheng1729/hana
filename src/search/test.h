#include <cstddef>  // for std::size_t

class Database;

// Test propositional proof search. Return 1 iff okay.
bool testpropsearch
    (Database const & database, std::size_t maxsize, double const parameters[4]);
