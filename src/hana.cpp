#include <cstdlib>  // for EXIT_..., std::size_t
#include <fstream>
#include "comment.h"
#include "database.h"
#include "io.h"
#include "param.h"
#include "propctor.h"
#include "search/problem.h"
#include "sect.h"
#include "token.h"
#include "util/timer.h"

// Pre-run test
int test()
{
    Timer timer;
    bool pretest(); // should be 1
    if (!pretest()) return EXIT_FAILURE;
    std::cout << "done in " << timer << 's' << std::endl;
    return EXIT_SUCCESS;
}

// .mm file extension check
bool ismmfile(const char * filename)
{
    std::cout << filename;
    std::size_t len = std::strlen(filename);
    return len > 3 &&
    std::strncmp(filename + len - 3, ".mm", 3) == 0;
}

// Configuration only
int config(const char * cmd, const char * filename)
{
    switch (std::tolower(static_cast<unsigned char>(cmd[0])))
    {
    case 'r':
        if (!Param::default.save(filename))
            return EXIT_FAILURE;
        std::cout << "Default parameters\n";
        std::cout << Param::default << "saved" << std::endl;
        return EXIT_SUCCESS;
    default:
        unexpected(true, "Command", cmd);
        return EXIT_FAILURE;
    }
}

// Read tokens. Return true if okay.
bool read(const char * filename, Tokens & tokens, Comments & comments)
{
    std::cout << "Reading file " << filename << ' ';
    Timer timer;
    bool doread(const char * filename, Tokens & tokens, Comments & comments);
    if (!doread(filename, tokens, comments)) return false;
    std::cout << "done in " << timer << 's' << std::endl;
    return true;
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
    database.addtheorempool();

    database.loaddefinitions();
    std::cout << "Primitive syntax axioms\n" << database.primitivesyntaxioms();

    return database.checkdefinitions();
}

bool loadprop(Database & database)
{
    Propctors const * p = database.info().set("Propctors", Propctors(database));
    if (!p->check(database.definitions())) return false;

    printpercent(p->markassertions
                 (const_cast<Assertions&>(database.assertions()),
                 database.typecodes()), "/",
                 database.assertions().size(), " propositional assertions\n");
    return true;
}

bool checkprop(Database const & database)
{
    Timer timer;
    if (!GETINFO(database, Propctors).checkpropsat
        (database.assertions(), database.typecodes())) return false;
    std::cout << "checked in " << timer << 's' << std::endl;
    return true;
}

int main(int argc, char * argv[])
{
    if (argc <= 1) return test();

    static const char paramfilename[] = "param.bin";

    if (!ismmfile(argv[1]))
        return config(argv[1], paramfilename);

    Tokens tokens;
    Comments comments;
    if (!read(argv[1], tokens, comments))
        return EXIT_FAILURE;

    Sections const sections(comments);
    if (!sections.empty())
        std::cout << "Last section: " << sections.rbegin()->first,
        std::cout << sections.rbegin()->second;

    tokens.rewind();
    // Iterator to the end section
    Sections::const_iterator const end =
        argv[2] ? sections.find(argv[2]) : sections.end();
    // # tokens to read
    std::size_t size(end == sections.end() ? tokens.size() :
                     end->second.tokenpos());
    std::cout << "Reading and verifying " << size << " tokens";
    Database database;
    Timer timer;
    if (!database.read(tokens, comments, size))
        return EXIT_FAILURE;
    std::cout << " done in " << timer << 's' << std::endl;

    if (!addRPN(database))
        return EXIT_FAILURE;
    database.buildsyntaxDAG();
    std::cout << database.syntaxDAG();

    if (!loadprop(database) || !checkprop(database))
        return EXIT_FAILURE;

    Param param = Param::default;
    // param.update(paramfilename);
    param.read(paramfilename);

    bool testpropsearch(Database const &, Param const &);
    if (!testpropsearch(database, param))
        return EXIT_FAILURE;
// Uncomment these lines if you want to output to a file.
    // std::ofstream out("result.txt");
    // std::streambuf * coutbuf = std::cout.rdbuf(out.rdbuf());
    // bool const okay = testpropsearch(database, param);
    // std::cout.rdbuf(coutbuf);
    // if (!okay) return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
