#include <algorithm>    // for std::find
#include "ass.h"
#include "io.h"
#include "proof/analyze.h"
#include "proof/skeleton.h"
#include "propctor.h"
#include "relation.h"
#include "typecode.h"
#include "util/arith.h"
#include "util/find.h"
#include "util/msg.h"
#include "bank.h"

std::ostream & operator<<(std::ostream & out, Propctor const & propctor)
{
    if (propctor.pdef)
        out << propctor.pdef->first << ": " << std::endl;
    else
        out << "undefined" << std::endl;
    out << "LHS = " << propctor.lhs;
    out << "RHS = " << propctor.rhs;
    out << "Truth table: " << propctor.truthtable;
    out << "CNF:" << std::endl << propctor.cnf;
    out << "arg# = " << propctor.argcount << std::endl;

    return out;
}

// If a syntax constructor is propositional,
// i.e., if all hypotheses and conclusion are floating and begins with "wff",
// Return the size of its truth table. Otherwise return 0.
static TTindex truthtablesize(Assertion const & ass)
{
    static const char wff[] = "wff";
    if (ass.exptypecode() != wff)
        return 0;

    TTindex result = 1;

    for (Hypsize i = 0; i < ass.hypcount(); ++i)
    {
        if (!ass.hypfloats(i) || ass.hyptypecode(i) != wff)
            return 0;
        result *= 2;
    }

    return result;
}

// Return true the data of a propositional syntax constructor is okay.
static bool checkpropctor(Propctor const & propctor)
{
    if (propctor.truthtable.size()
        != static_cast<TTindex>(1) << propctor.argcount)
        return false;
    if (propctor.cnf.atomcount() != propctor.argcount + 1)
        return false;

    CNFClauses cnf(propctor.cnf);
    cnf.closeoff();
    return cnf.truthtable(propctor.argcount) == propctor.truthtable;
}

static void printmsg(strview label, strview prompt)
{
    std::cout << label << '\t' << prompt << " definition" << std::endl;
}

// Return true if data for all propositional syntax constructor are okay.
bool Propctors::check(Definitions const & definitions) const
{
    FOR (const_reference propctor, *this)
    {
        strview label = propctor.first;
        if (!definitions.count(label))
            printmsg(label, "no");
        else if (!propctor.second.pdef)
            printmsg(label, "unused");
        if (unexpected(!checkpropctor(propctor.second), "bad data for", label))
            return false;
    }

    return true;
}

// Print all propositional syntax constructors with truth tables.
void Propctors::printtruthtables() const
{
    FOR (const_reference propctor, *this)
        std::cout << propctor.first << '\t' << propctor.second.truthtable;    
}

// Add a batch of definitions with a given truth table. Return # items added.
Propctors::size_type Propctors::addbatch
    (struct Relations const & batch, bool const truthtable[])
{
    size_type count = 0;

    FOR (Relations::const_reference rdef, batch)
    {
        pAss  pass = rdef.second.pass;
        if (!pass) continue;
        strview label = pass->first;
        TTindex ttsize = truthtablesize(pass->second);
        if (ttsize == 0) continue;
        Bvector tt(truthtable, truthtable + ttsize);
        // New propctor
        Propctor & propctor = (*this)[label];
        propctor.cnf = propctor.truthtable = tt;
        propctor.argcount = util::log2(ttsize);
        if (unexpected(!checkpropctor(propctor), "bad data for", label))
            continue;
        ++count;
        // std::cout << label << '\t' << tt;
    }

    return count;
}

// Add a definition. Return the iterator to the entry. Otherwise return end.
Propctors::const_iterator Propctors::adddef
    (Definitions const & defs, Definitions::const_reference rdef)
{
    strview label = rdef.first;
    Definition const & def(rdef.second);
    if (unexpected(!def.pdef, "empty definition", label))
        return end();
    // Extended definition using the propositional skeleton
    Definition xdef(def);
    Bank bank;
    // RHS = propositonal skeleton
    xdef.rhs = propskeleton(def.rhs, ast(def.rhs), bank);
    // Preallocate for efficiency.
    xdef.lhs.reserve(xdef.lhs.size() + bank.varcount());
    // Add pseudo variables to LHS.
    FOR (Hypotheses::const_reference rhyp, bank.hypotheses())
        xdef.lhs.insert(xdef.lhs.end() - 1, &rhyp);
// std::cout << "Def of " << label << ": " << xdef.lhs << xdef.rhs;
    // Truth table of the definition
    Bvector tt(truthtable(defs, xdef));
    // # arguments of the definition
    Atom const argcount = def.lhs.size() - 1;
    // real size of the truth table
    TTindex const ttsize = static_cast<TTindex>(1) << argcount;
    // Check if truth table is independent of added variables.
    if (!util::isperiodic(tt.begin(), tt.end(), ttsize))
        return end();
    // Data for new propositional syntax constructor
    iterator iter = insert(value_type(label, Propctor())).first;
    static_cast<Definition &>(iter->second) = def;
    tt.resize(ttsize);
    iter->second.cnf = iter->second.truthtable = tt;
    iter->second.argcount = argcount;
    // std::cout << label << '\t' << tt;
    return iter;
}

// Return the truth table of a definition. Return empty Bvector if not okay.
Bvector Propctors::truthtable(Definitions const & defs, Definition const & def)
{
    // # arguments of the definition
    Atom const argcount = def.lhs.size() - 1;
    Atom const maxargc  = std::numeric_limits<TTindex>::digits;
    static const char varallowed[] = " variables allowed";
    if (!is1stle2nd(argcount, maxargc, varfound, varallowed))
        return Bvector();
    // Truth table of the definition
    Bvector truthtable(static_cast<TTindex>(1) << argcount);
    for (TTindex arg = 0; arg < truthtable.size(); ++arg)
    {
        int const value = calcbool(defs, def, arg);
        if (value == UNKNOWN)
            return Bvector();
        truthtable[arg] = value;
    }
// std::cout << "Truth table of " << label << " = " << truthtable;
    return truthtable;
}

// Evaluate the truth table against stack. Return true if okay.
static bool calc_stack(Bvector const & truthtable, Bvector & stack)
{
    if (truthtable.empty())
        return false;
    // # arguments
    Atom argcount = util::log2(truthtable.size());
    if (!is1stle2nd(argcount, stack.size(), varfound, itemonstack))
        return false;

    TTindex arg = 0;
    // arguments packed in binary
    for ( ; argcount > 0; --argcount)
    {
        (arg *= 2) += stack.back();
        stack.pop_back();
    }
//std::cout << "arguments = " << arg << std::endl;
    stack.push_back(truthtable[arg]);
    return true;
}

// Evaluate *iter at arg. Return UNKNOWN if not okay.
int Propctors::calcbool
    (Definitions const & defs, Definition const & def, TTindex arg)
{
    Proofsteps const & lhs = def.lhs, & rhs = def.rhs;
    Bvector stack;
// std::cout << rhs;
    FOR (Proofstep const step, rhs)
    {
        // Iterator to the variable in the LHS
        Stepiter const itervar = std::find(lhs.begin(), lhs.end() - 1, step);
        if (itervar != lhs.end() - 1)
        {
            // Variable exists in the LHS.
            stack.push_back(arg >> (itervar - lhs.begin()) & 1);
// std::cout << step << ": variable, value = " << stack.back() << std::endl;
            continue;
        }
// std::cout << step << ": connective" << std::endl;
        // Iterator to its data
        const_iterator iterctor = find(key_type(step));
        if (iterctor == end())
        {
            // Data missing or corrupt. Compute from definition.
            Defiter const iterdf = defs.find(key_type(step));
            if (iterdf != defs.end())
                iterctor = adddef(defs, *iterdf);
            // Definition not found, or truthtable can't be computed
            if (iterctor == end())
                return UNKNOWN;
        }

        if (!calc_stack(iterctor->second.truthtable, stack))
            return UNKNOWN;
    }

    if (stack.size() != 1)
    {
        std::cerr << "Syntax proof " << rhs << stackszerr << std::endl;
        return UNKNOWN;
    }

    return stack[0];
}

// Add lit <-> atom (in the positive sense) to CNF.
static void addlitatomequiv(CNFClauses & cnf, Literal lit, Atom atom)
{
    CNFClauses::size_type const n = cnf.size();
    cnf.resize(n + 2);
//std::cout << "Single literal CNF (" << lit << '<->' << atom << ") added\n";
    cnf[n].resize(2),           cnf[n + 1].resize(2);
    cnf[n][0] = lit,            cnf[n][1] = 2 * atom + 1;
    cnf[n + 1][0] = lit ^ 1,    cnf[n + 1][1] = 2 * atom;
//std::cout << cnf;
}

// Add CNF clauses from reverse Polish notation.
// # of auxiliary atoms start from natom.
// Return true if okay. First auxiliary atom = hyps.size()
bool Propctors::addclause
    (Proofsteps const & RPN, Hypiters const & hyps,
     CNFClauses & cnf, Atom & natom) const
{
    return addclause(RPN, ast(RPN), hyps, cnf, natom);
}
bool Propctors::addclause
    (Proofsteps const & RPN, AST const & ast, Hypiters const & hyps,
     CNFClauses & cnf, Atom & natom) const
{
    if (unexpected(ast.empty(), "empty proof tree when adding CNF of", RPN))
        return throw, false;

    std::vector<Literal> literals(RPN.size());
    for (Proofsize i = 0; i < RPN.size(); ++i)
    {
        const char * step = RPN[i];
//std::cout << "Step " << step << ":\t";
        if (RPN[i].ishyp())
        {
            Hypsize const hypindex = util::find(hyps, step) - hyps.begin();
            if (hypindex < hyps.size())
            {
                literals[i] = (hypindex) * 2;
//std::cout << "argument " << hypindex << std::endl;
                if (RPN.size() != 1)
                    continue;
                // Expression with a single variable
                addlitatomequiv(cnf, literals[i], natom++);
//std::cout << "New CNF:\n" << cnf;
                return true;
            }
            std::cerr << "In " << RPN;
            return !unexpected(true, "variable", step);
        }
//std::cout << "operator ";
        // CNF
        const_iterator const itercnf = find(step);
        if (unexpected(itercnf == end(), "connective", step))
            return false;
        // Its arguments
        std::vector<Literal> args(ast[i].size());
        for (ASTnode::size_type j = 0; j < args.size(); ++j)
            args[j] = literals[ast[i][j]];
        // Add the CNF.
        cnf.append(itercnf->second.cnf, natom, args.data(), args.size());
        literals[i] = natom++ * 2;
//std::cout << literals[i] / 2 << std::endl;
    }
//std::cout << "New CNF:\n" << cnf;
    return true;
}

// Translate the hypotheses of a propositional assertion to the CNF of an SAT.
Hypscnf Propctors::hypscnf(Assertion const & ass, Atom & natom,
                           Bvector const & hypstotrim) const
{
    natom = ass.hypcount(); // One atom for each floating hypotheses

    Hypscnf result;
    CNFClauses & cnf = result.first;
    result.second.resize(ass.hypcount());
// std::cout << "Adding clauses for ";
    for (Hypsize i = 0; i < ass.hypcount(); ++i)
    {
        if (!ass.hypfloats(i) && !(i < hypstotrim.size() && hypstotrim[i]))
        {
// std::cout << ass.hypexp(i);
            if (!addclause
                (ass.hypRPN(i), ass.hypAST(i), ass.hypiters, cnf, natom))
                return Hypscnf();
            // Assume the hypothesis.
            cnf.closeoff((natom - 1) * 2);
        }
        result.second[i] = cnf.size();
    }
    return result;
}

// Translate a propositional assertion to the CNF of an SAT instance.
CNFClauses Propctors::cnf
    (Assertion const & ass, Proofsteps const & conclusion,
     Bvector const & hypstotrim) const
{
    // Add hypotheses.
    Atom natom = 0;
    CNFClauses cnf(hypscnf(ass, natom, hypstotrim).first);
    // Add conclusion.
    if (!addclause(conclusion, ass.hypiters, cnf, natom))
        return CNFClauses();
    // Negate conclusion.
    cnf.closeoff((natom - 1) * 2 + 1);

    return cnf;
}

// Return true if an expression is valid given a propositional assertion.
bool Propctors::checkpropsat(Assertion const & ass,
                             Proofsteps const & conclusion) const
{
    CNFClauses const & clauses(cnf(ass, conclusion));

    if (!clauses.sat())
        return true;

    if (&conclusion == &ass.expRPN)
        std::cerr << "CNF:\n" << clauses << "counter-satisfiable" << std::endl;
    return false;
}

// Return true if all propositional assertions are sound.
bool Propctors::checkpropsat(Assertions const & assertions,
                             struct Typecodes const & typecodes) const
{
    FOR (Assertions::const_reference rass, assertions)
    {
        Assertion const & assertion(rass.second);
        if (assertion.expression.empty())
            return false;
        if (typecodes.isprimitive(assertion.exptypecode()) != FALSE)
            continue; // Skip syntax axioms.
        if (!(assertion.type & Asstype::PROPOSITIONAL))
            continue; // Skip non propositional assertions.

        if (!checkpropsat(assertion, assertion.expRPN))
        {
            printass(rass);
            std::cerr << "Logic error!" << std::endl;
            return false;
        }
    }

    return true;
}

// Return true if the root of exp is propositional,
// i.e., if all hypotheses and conclusion are floating and begins with "wff".
static Splitretval splitroot(Steprange exp)
{
    Proofstep const root = exp.root();
    if (!root.isthm() || !root.pass) return KEEPRANGE;
    return truthtablesize(root.pass->second) ? SPLITREC : KEEPRANGE;
}

// Return the propositional skeleton of an RPN.
Proofsteps propskeleton
    (Proofsteps const & RPN, AST const & ast, class Bank & bank)
{
    if (RPN.empty() || RPN.size() != ast.size())
        return Proofsteps();

    SteprangeAST const exp(RPN, ast);

    // Preallocate for efficiency
    Proofsteps result;
    result.reserve(RPN.size());

    return skeleton(exp, splitroot, bank, result) != UNKNOWN ?
        result: Proofsteps();
}
