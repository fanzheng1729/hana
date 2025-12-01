#include "../proof/analyze.h"
#include "../CNF.h"
#include "../disjvars.h"
#include "goaldata.h"
#include "../io.h"
#include "problem.h"
#include "prop.h"
#include "../util/progress.h"
#include "../util/timer.h"

Prop::Prop(Assertion const & ass, Database const & db,
            std::size_t maxsize, bool staged) :
    Environ(ass, db, maxsize, staged),
    hypscnf(db.propctors().hypscnf(ass, hypatomcount))
{
    Propctors const & propctors = database.propctors();
    Propctors::size_type const propcount = propctors.size();
    // Preallocate for efficiency.
    propctorlabels.reserve(propcount);
    propctorfreqs.reserve(propcount);
    // Total frequency counts
    Proofsize total = 0;
    // Initialize propositional syntax axiom labels.
    FOR (Propctors::const_reference propctor, propctors)
    {
        propctorlabels.push_back(propctor.first);
        total += propctor.second.count;
    }
    // Initialize propositional syntax axiom frequencies.
    if (total == 0 && propcount > 0)
        propctorfreqs.assign(propcount, 1./propcount);
    else
        FOR (Propctors::const_reference propctor, propctors)
            propctorfreqs.push_back
                (static_cast<double>(propctor.second.count) / total);
    // Initialize propositional syntax axiom counts in hypotheses.
    for (Hypsize i = 0; i < ass.hypcount(); ++i)
        if (!ass.hypfloats(i))
            FOR (Proofstep step, ass.hypRPN(i))
                if (step.type == Proofstep::THM && step.pass)
                    if (const char * label = step.pass->first.c_str)
                    {
                        std::vector<strview>::size_type const i
                        = util::find(propctorlabels, label)
                        - propctorlabels.begin();
                        if (i < hypspropctorcounts.size())
                            ++hypspropctorcounts[i];
                    }
}

// Determine status of a goal.
Goalstatus Prop::status(Goal const & goal) const
{
    if (database.typecodes().isprimitive(goal.typecode) != FALSE)
        return GOALFALSE; // goal must start with a non-primitive type code.
    Proofsteps const & RPN = goal.RPN;
    CNFClauses cnf(hypscnf.first);
    Atom natom = hypatomcount;
    // Add hypotheses.
    if (!database.propctors().addclause(RPN, assertion.hypiters, cnf, natom))
    {
        std::cerr << "Bad CNF from\n" << RPN << "in " << name << std::endl;
        return GOALFALSE;
    }
    // Negate conclusion.
    cnf.closeoff((natom - 1) * 2 + 1);
    return cnf.sat() ? GOALFALSE : GOALTRUE;
}

// Return the hypotheses of a goal to trim.
Bvector Prop::hypstotrim(Goal const & goal) const
{
    Bvector result(assertion.hypcount(), false);

    Hypsize ntotrim = 0; // # essential hypothesis to trim
    for (Hypsize i = assertion.hypcount() - 1; i != Hypsize(-1); --i)
    {
        if (assertion.hypfloats(i)) continue;
// std::cout << "Trimming hypothesis " << assertion.hyplabel(i) << std::endl;
        result[i] = true;
        // Check if it can be trimmed.
        CNFClauses cnf2;
        Proofnumbers const & ends = hypscnf.second;
        // Add hypotheses.
        for (Hypsize j = 0; j < assertion.hypcount(); ++j)
        {
            if (assertion.hypfloats(j) || result[j])
                continue; // Skip floating or trimmed hypotheses.
// std::cout << "Adding hypothesis " << assertion.hyplabel(j) << std::endl;
            // Add CNF clauses.
            CNFClauses::size_type begin = j>0 ? ends[j - 1] : 0, end = ends[j];
            cnf2.insert(cnf2.end(), hypscnf.first.data() + begin,
                        hypscnf.first.data() + end);
        }
// std::cout << "hypcnf\n" << hypscnf.first << "cnf\n" << cnf2;
        Atom natom = cnf2.empty() ? assertion.hypcount() : cnf2.atomcount();
        // Add conclusion.
        database.propctors().addclause
        (goal.RPN, assertion.hypiters, cnf2, natom);
        // Negate conclusion.
        cnf2.closeoff((natom - 1) * 2 + 1);
        ntotrim += result[i] = !cnf2.sat();
    }

    if (ntotrim == 0)
        return Bvector();
    assertion.trimvars(result, goal.RPN);
    return result;
}

// Evaluate leaf games, and record the proof if proven.
Eval Prop::evalourleaf(Game const & game) const
{
    Proofsize len = game.env().hypslen + game.goal().size() + game.nDefer;
    std::vector<Proofsize> propctorcounts = hypspropctorcounts;
    // Add propositional syntax axiom counts for goal.
    FOR (Proofstep step, game.goal().RPN)
        if (step.type == Proofstep::THM && step.pass)
            if (const char * label = step.pass->first.c_str)
            {
                std::vector<strview>::size_type const i
                = util::find(propctorlabels, label) - propctorlabels.begin();
                if (i < propctorcounts.size())
                    ++propctorcounts[i];
            }
    // Total occurrence count
    Proofsize const total
    = std::accumulate(propctorcounts.begin(), propctorcounts.end(),
                        static_cast<Proofsize>(0));
    // L2 distance
    double l2dist = 0;
    for (std::vector<Proofsize>::size_type i = 0; i < propctorcounts.size(); ++i)
    {
        double const diff =
        static_cast<double>(propctorcounts[i] - hypspropctorcounts[i]);
        l2dist += diff * diff;
    }
    return score(len);
}

// Return the simplified assertion for the goal of the game to hold.
Assertion Prop::makeAss(Bvector const & hypstotrim) const
{
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
    // Add a move. Return true if the move closed the goal.
    bool operator()(Argtypes const & types, Genresult const & result,
                    Genstack const & stack)
    {
        for (Proofsize i = 0; i < types.size(); ++i)
            move.substitutions[freevars[i]] =
            result.at(freevars[i].typecode())[stack[i]];
        // Filter move by SAT.
        switch (env.valid(move))
        {
        case Environ::MoveCLOSED:
            moves.assign(1, move);
            return true;
        case Environ::MoveVALID:
            // std::cout << move << std::endl;
            // std::cout << move.substitutions;
            moves.push_back(move);
            // std::cin.get();
            return false;
        default:
            return false;
        }
    }
};

// Add a move with free variables. Return false.
bool Prop::addhardmoves
    (pAss pthm, Proofsize size, Move & move, Moves & moves) const
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

static void printtime(Problem::size_type nodes, Time time)
{
    std::cout << nodes << " nodes / " << time << "s = ";
    std::cout << nodes/time << " nps" << std::endl;
}

// Test propositional proof search. Return 1 iff okay.
bool testpropsearch
    (Database const & database, std::size_t maxsize, double const parameters[4])
{
    std::cout << "Testing propositional proof search";
    Progress progress(std::cerr);
    Timer timer;
    bool okay = true;
    Problem::size_type nodes = 0;
    Assiters::size_type all = 0, proven = 0;
    // Test assertions
    Assiters const & assiters = database.assiters();
    for (Assiters::size_type i = 1; i < assiters.size(); ++i)
    {
        Assiter const iter = assiters[i];

        // Skip axioms, trivial and duplicate theorems.
        if (iter->second.type
            & (Asstype::AXIOM + Asstype::DUPLICATE))
            continue;

        // Skip non propositional theorems.
        static const Prop prop1(assiters[1]->second, database, 0);
        if (!(prop1.ontopic(iter->second)))
            continue;

        // Try search proof.
        ++all;
        Prop prop(iter->second, database, maxsize, parameters[3]);
        Problem tree(prop, parameters);
        Problem::size_type const n = testsearch(iter, tree, maxsize);
        if (n == 0)
        {
            okay = false;
            break;
        }
        nodes += n;
        proven += n <= maxsize;
        progress << i/static_cast<Ratio>(assiters.size() - 1);
    }

    // Print stats.
    std::cout << '\n';
    printtime(nodes, timer);
    printpercent(proven, "/", all, " = ", "% proven\n");
    return okay;
}
