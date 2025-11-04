#include "../proof/analyze.h"
#include "../cnf.h"
#include "../disjvars.h"
#include "goaldata.h"
#include "../io.h"
#include "problem.h"
#include "prop.h"
#include "../util/progress.h"
#include "../util/timer.h"

// Return true if a goal is valid.
bool Prop::valid(Proofsteps const & goal) const
{
    CNFClauses cnf(hypscnf.first);
    Atom natom = hypatomcount;
    if (!database.propctors().addclause(goal, assertion.hypiters, cnf, natom))
    {
        std::cerr << "CNF validation failure with vars " << m_varusage;
        return false;
    }

    cnf.closeoff((natom - 1) * 2 + 1);
    return !cnf.sat();
}

// Return the hypotheses of a goal to trim.
Bvector Prop::hypstotrim(Goalptr goalptr) const
{
    Bvector result(assertion.hypcount(), false);

    Hypsize ntotrim = 0; // # essential hypothesis to trim
    for (Hypsize i = assertion.hypcount() - 1; i != Hypsize(-1); --i)
    {
        Hypiter const hypiter = assertion.hypiters[i];
        if (assertion.hypfloats(i)) continue;
// std::cout << "Trimming hypothesis " << hypiter->first << std::endl;
        result[i] = true;
        // Check if it can be trimmed.
        CNFClauses cnf2;
        Proofnumbers const & ends = hypscnf.second;
        // Add hypotheses.
        for (Hypsize j = 0; j < assertion.hypcount(); ++j)
        {
            if (assertion.hypfloats(j) || result[j])
                continue; // Skip floating or trimmed hypotheses.
// std::cout << "Adding hypothesis " << assertion.hypiters[j]->first << std::endl;
            // Add CNF clauses.
            CNFClauses::size_type begin = j>0 ? ends[j - 1] : 0, end = ends[j];
            cnf2.insert(cnf2.end(), hypscnf.first.data() + begin,
                        hypscnf.first.data() + end);
        }
// std::cout << "hypcnf\n" << hypscnf.first << "cnf\n" << cnf2;
        Atom natom = cnf2.empty() ? assertion.hypcount() : cnf2.atomcount();
        // Add conclusion.
        database.propctors().addclause
        (goalptr->first.RPN, assertion.hypiters, cnf2, natom);
        // Negate conclusion.
        cnf2.closeoff((natom - 1) * 2 + 1);
        ntotrim += result[i] = !cnf2.sat();
    }

    if (ntotrim > 0)
    {
        assertion.trimvars(result, goalptr->first.RPN);
        return result;
    }
    else
        return Bvector();
}

// Return the simplified assertion for the goal of the game to hold.
Assertion Prop::makeass(Game const & game) const
{
    Bvector const & hypstotrim = game.goaldata().hypstotrim;
    Assertion result;
    result.number = assertion.number;
    result.sethyps(assertion, hypstotrim);
    result.expression.resize(1);
    result.disjvars = assertion.disjvars & result.varusage;
    return result;
}

// Adds substitutions to a move.
struct Substadder : Adder
{
    Expression const & freevars;
    Moves & moves;
    Move & move;
    Environ const & env;
    Substadder
    (Expression const & freevars, Moves & moves, Move & move,
        Environ const & env) :
        freevars(freevars), moves(moves), move(move), env(env) {}
    void operator()(Argtypes const & types, Genresult const & result,
                    Genstack const & stack)
    {
        for (Proofsize i = 0; i < types.size(); ++i)
            move.substitutions[freevars[i]] =
            result.at(freevars[i].typecode())[stack[i]];
        // Filter move by SAT.
        if (env.valid(move))
            // std::cout << move << std::endl,
            // std::cout << move.substitutions,
            moves.push_back(move);
    }
};

// Add a move with free variables. Return false.
bool Prop::addhardmoves
    (Assptr pthm, Proofsize size, Move & move, Moves & moves) const
{
// if (size >= 5)
// std::cout << "Trying " << pthm->first << ' ' << size << std::endl;
    Assertion const & thm = pthm->second;
    // Free variables in the theorem to be used
    Expression freevars;
    // Type codes of free variables
    Argtypes types;
    // Preallocate for efficiency.
    freevars.reserve(thm.nfreevar()), types.reserve(thm.nfreevar());
    FOR (Varusage::const_reference var, thm.varusage)
        if (!var.second.back())
            freevars.push_back(var.first), types.push_back(var.first.typecode());
    // Generate substitution terms.
    FOR (Symbol3 var, freevars)
        generateupto(var.typecode(), size);
    // Generate substitutions.
    Substadder adder(freevars, moves, move, *this);
    dogenerate(types, size + 1, adder);
// if (size >= 5)
// std::cout << moves.size() << " hard moves found" << std::endl;
    return false;
}

// Test proof search for propositional theorems.
// Return the size of tree if okay. Otherwise return 0.
static Problem::size_type testpropsearch
    (Assiter iter, Problem & tree, Problem::size_type maxsize)
{
    // printass(*iter);
    tree.play(maxsize);
    // if (iter->first == "biass")
        // tree.navigate();
    if (tree.size() > maxsize)
    {
        // printass(*iter);
        // std::cout << std::endl;
        // tree.printstats();
        // tree.printenvs();
        // tree.navigate();
    }
    else if (tree.value() != 1)
    {
        std::cerr << "Prop search returned " << tree.value() << "\n";
        tree.printmainline();
        return 0;
    }
    else
    if (!provesrightthing(iter->first, verify(tree.proof(), &*iter),
                            iter->second.expression))
    {
        std::cerr << "Wrong proof: " << tree.proof();
        // tree.navigate();
        std::cin.get();
    }
    else if (iter->first == "biluk_")
        tree.writeproof((std::string(iter->first) + ".txt").c_str());

    return tree.size();
}

static void printtime(Problem::size_type nodecount, Time time)
{
    std::cout << nodecount << " nodes / " << time << "s = ";
    std::cout << nodecount/time << " nps\n";
}

// Test propositional proof search. Return 1 iff okay.
bool testpropsearch
    (Database const & database, std::size_t maxsize,
     double const parameters[3])
{
    std::cout << "Testing propositional proof search";
    Progress progress(std::cerr);
    Timer timer;
    bool okay = true;
    Problem::size_type nodecount = 0;
    Assiters::size_type all = 0, proven = 0;
    // Test assertions
    Assiters const & assiters = database.assiters();
    for (Assiters::size_type i = 1; i < assiters.size(); ++i)
    {
        Assiter const iter = assiters[i];
// printass(*iter);
        // Skip axioms, trivial and duplicate theorems.
        if (iter->second.type
            & (Asstype::AXIOM + Asstype::TRIVIAL + Asstype::DUPLICATE))
            continue;

        // Skip non propositional theorems
        static const Prop prop1(assiters[1]->second, database, 0);
        if (!(prop1.ontopic(iter->second)))
            continue;

        // Try search proof.
        Prop prop(iter->second, database, maxsize, parameters[2]);
        Problem tree(prop, parameters);
        Problem::size_type const n = testpropsearch(iter, tree, maxsize);
        ++all;
        if (n == 0)
        {
            okay = false;
            break;
        }
        nodecount += n;
        proven += n <= maxsize;
        progress << i/static_cast<Ratio>(assiters.size() - 1);
    }

    // Print stats.
    std::cout << '\n';
    printtime(nodecount, timer);
    printpercent(proven, "/", all, " = ", "% proven\n");
    return okay;
}
