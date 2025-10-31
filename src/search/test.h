class Database;
class Problem;

// Test propositional proof search. Return 1 iff okay.
bool testpropsearch
    (Database const & database, Problem::size_type maxsize,
     double const parameters[3]);
