#include <cstdlib>  // for EXIT_..., std::size_t
#include <fstream>
#include "comment.h"
#include "database.h"
#include "io.h"
#include "sect.h"
#include "token.h"
#include "search/problem.h"
#include "search/test.h"
#include "util/timer.h"

int test()
{
    Timer timer;
    bool pretest(); // should be 1
    if (!pretest()) return EXIT_FAILURE;
    std::cout << "done in " << timer << 's' << std::endl;
    return EXIT_SUCCESS;
}

bool read(const char * filename, Tokens & tokens, Comments & comments)
{
    std::cout << "Reading file " << filename;
    Timer timer;
    bool doread(const char * filename, Tokens & tokens, Comments & comments);
    if (!doread(filename, tokens, comments)) return false;
    std::cout << " done in " << timer << 's' << std::endl;
    return true;
}

bool checkassiters(Database const & database)
{
    std::cout << "Checking iterators to assertions" << std::endl;
    bool checkassiters
        (Assertions const & assertions, Assiters const & assiters);
    return checkassiters(database.assertions(), database.assiters());
}

bool addRPN(Database & database)
{
    std::cout << "Parsing RPN";
    Timer timer;
    if (!database.addRPN()) return false;
    std::cout << "done in " << timer << 's' << std::endl;

    std::cout << "Testing RPN";
    timer.reset();
    if (!database.checkRPN()) return false;
    std::cout << "done in " << timer << 's' << std::endl;
    return true;
}

int main(int argc, char * argv[])
{
    if (argc < 2) return test();

    Tokens tokens;
    Comments comments;
    if (!read(argv[1], tokens, comments)) return EXIT_FAILURE;

    Sections sections(comments);
    if (!sections.empty())
        std::cout << "Last section: " << sections.rbegin()->first,
        std::cout << sections.rbegin()->second;

    tokens.position = 0;
    // Iterator to the end section
    Sections::const_iterator const end
        (argv[2] ? sections.find(argv[2]) : sections.end());
    // # tokens to read
    std::size_t size(end == sections.end() ? tokens.size() :
                     end->second.tokenpos());
    std::cout << "Reading and verifying " << size << " tokens";
    Database database;
    Timer timer;
    if (!database.read(tokens, comments, size)) return EXIT_FAILURE;
    std::cout << " done in " << timer << "s\n";
    if (!checkassiters(database)) return EXIT_FAILURE;
    if (!addRPN(database)) return EXIT_FAILURE;

    // std::cout << "Marking duplicate assertions";
    // timer.reset();
    // printpercent(database.markduplicate(), "/",
    //              database.assertions().size(), " duplicates");
    // std::cout << " marked in " << timer << 's' << std::endl;

    database.loaddefinitions();
    std::cout << "Primitive syntax axioms\n" << database.primitivesyntaxioms();
    if (!database.checkdefinitions()) return EXIT_FAILURE;

    database.loadpropctors();
    if (!database.propctors().check(database.definitions())) return EXIT_FAILURE;

    printpercent(database.markpropassertions(), "/",
                 database.assertions().size(), " propositional assertions\n");
    timer.reset();
    if (!database.checkpropassertion()) return EXIT_FAILURE;
    std::cout << "checked in " << timer << 's' << std::endl;

    std::cout << "Propositional syntax axioms frequency count" << std::endl;
    FOR (Propctors::const_reference propctor, database.propctors())
        std::cout << propctor.first << ' ' << propctor.second.count << ' ';
    std::cout << std::endl;

    database.buildsyntaxDAG();
    std::cout << database.syntaxDAG();
    // return EXIT_SUCCESS;

    Value parameters[] = {0, 1e-3, 1, 0};
    // Value parameters[] = {0, 1e-4, 0, Problem::STAGED};
    Problem::size_type maxsize = 1u << 12;
    if (!testpropsearch(database, maxsize, parameters))
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
// Uncomment these lines if you want to output to a file.
    // std::ofstream out("result.txt");
    // std::streambuf * coutbuf = std::cout.rdbuf(out.rdbuf());
    // bool const okay = testpropsearch(database, maxsize, parameters);
    // std::cout.rdbuf(coutbuf);
    // return okay ? EXIT_SUCCESS : EXIT_FAILURE;
}
