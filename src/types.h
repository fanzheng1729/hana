#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <cstddef>      // for std::size_t and NULL
#include <map>
#include <set>
#include <utility>
#include <vector>
#include "util/fun.h"   // for util::mem_fn and util::not_fn
#include "util/strview.h"

// Boolean vector
typedef std::vector<bool> Bvector;

// A constant or a variable with ID and pointer to defining hypothesis
struct Symbol3;
// Expression is a sequence of math tokens.
typedef std::vector<Symbol3> Expression;
// Iterator to an expression
typedef Expression::const_iterator Expiter;
// Begin and end iterators to a subexpression
typedef std::pair<Expiter, Expiter> Subexp;
// A sequence of subexpressions
typedef std::vector<Subexp> Subexps;

typedef std::size_t Proofnumber;
// Vector of proof numbers
typedef std::vector<Proofnumber> Proofnumbers;

// Step in a regular or compressed proof
struct Proofstep;
// A sequence of proof steps
typedef std::vector<Proofstep> Proofsteps;
// Pointers to the proofs to be included
typedef std::vector<Proofsteps const *> pProofs;
// # of step in the proof
typedef Proofsteps::size_type Proofsize;
// Iterator to a proof step
typedef Proofsteps::const_iterator Stepiter;

// Begin and end of a sequence of proof steps
struct Steprange : std::pair<Stepiter, Stepiter>
{
    Steprange(Stepiter begin = Stepiter(), Stepiter end = Stepiter())
        { first = begin, second = end; }
    Steprange(Proofsteps const & proofsteps) :
        std::pair<Stepiter, Stepiter>(proofsteps.begin(), proofsteps.end()) {}
    bool empty() const { return size() == 0; }
    void clear() { second = first; }
    Proofsize size() const { return second - first; }
    Proofstep const & root() const { return *(second - 1); }
    Proofstep const & operator[](Proofsize index) const { return first[index]; }
};
// Ranges of proof steps
typedef std::vector<Steprange> Stepranges;

// Node of an abstract syntax tree, listing the indices of all its hypotheses
typedef std::vector<Proofsize> ASTnode;
// Abstract syntax tree
typedef std::vector<ASTnode> AST;
// Iterator to an AST node
typedef AST::const_iterator ASTiter;

// Ranges governed by a Proofstep, map: range -> AST
typedef std::map<Steprange, AST, bool(*)(Steprange, Steprange)>
    GovernedStepranges;
// Map: Proofstep -> all ranges governed by the Proofstep
typedef std::map<Proofstep, GovernedStepranges, std::less<const char *> >
    GovernedSteprangesbystep;

// List of indentations
typedef std::vector<Proofsize> Indentations;

// Statistics
typedef Proofsize Freqcount, Weight;

struct Hypothesis
{
    Expression expression;
    bool floats;
    Proofsteps RPN;
    AST  ast;
    Hypothesis(Expression const & exp = Expression(), bool floating = false) :
        expression(exp), floats(floating) {}
};
// Map: label -> hypothesis
typedef std::map<strview, Hypothesis> Hypotheses;
// Pointer to (label, hypothesis)
typedef Hypotheses::const_pointer pHyp;
// Iterator to a hypothesis
typedef Hypotheses::const_iterator Hypiter;
// Return true if the name of the hypothesis pointed to is label.
inline bool operator==(Hypiter iter, strview label) {return iter->first==label;}
// A sequence of hypothesis iterators
typedef std::vector<Hypiter> Hypiters;
// # of hypotheses
typedef Hypiters::size_type Hypsize;
// Vector of hypothesis indices
typedef std::vector<Hypsize> Hypsizes;

// A constant or a variable with ID
struct Symbol2 : strview
{
    // ID of variable, 0 for constants
    typedef std::size_t ID;
    ID id;
    operator ID() const { return id; }
    Symbol2(strview str = "", ID n = 0) : strview(str), id(n) {}
};
// A constant or a variable with ID and pointer to defining hypothesis
struct Symbol3 : Symbol2
{
    // Iterator to the floating hypothesis for variable, NULL for constant
    Hypiter iter;
    strview typecode() const { return iter->second.expression[0]; }
    Symbol3(strview str = "", ID n = 0) : Symbol2(str, n) {}
    Symbol3(strview str, ID n, Hypiter it) : Symbol2(str, n), iter(it) {}
};

// Functor returning the id of an object, and 0 for anything else
static const struct
{
    template<class T>
    Symbol2::ID operator()(T t) { return util::mem_fn(&T::id)(t); }
} id;

// Set of symbols
typedef std::set<Symbol2> Symbol2s;
typedef std::set<Symbol3> Symbol3s;
// Map: var -> is used in (hypotheses..., expression)
typedef std::map<Symbol3, Bvector> Varusage;
inline bool operator<(Varusage::const_reference x, Varusage::const_reference y)
    { return x.first.id < y.first.id; }
// Set: (x, y) = $d x y
typedef std::set<std::pair<Symbol2, Symbol2> > Disjvars;

// An axiom or a theorem
struct Assertion;
// Map: label -> assertion
typedef std::map<strview, Assertion> Assertions;
// Pointer to (label, assertion)
typedef Assertions::const_pointer pAss;
// Iterator to an assertion
typedef Assertions::const_iterator Assiter;
// Vector of iterators to assertions
typedef std::vector<Assiter> Assiters;
// Types of assertions
enum Asstype
{
    AXIOM = 1,
    TRIVIAL = 2,
    DUPLICATE = 4,
    USELESS = TRIVIAL | DUPLICATE,
    NOUSE = 8,
    NONEWPROOF = 16,
    PROPOSITIONAL = 32
};

// Step in a regular or compressed proof
struct Proofstep
{
    typedef std::vector<Expression>::size_type Index;
    enum Type
    {
        NONE, HYP, THM, LOAD, SAVE
    } type;
    union
    {
        pHyp phyp;
        pAss pass;
        Index index;
    };
    Proofstep(Type t = NONE) : type(t == SAVE ? t : NONE) {}
    Proofstep(pHyp p) : type(p ? HYP : NONE), phyp(p) {}
    Proofstep(pAss p) : type(p ? THM : NONE), pass(p) {}
    Proofstep(Hypiter iter) : type(HYP), phyp(&*iter) {}
    Proofstep(Index i) : type(LOAD), index(i) {}
    bool empty() const { return type == NONE; }
    bool ishyp() const { return type == HYP; }
    bool isthm() const { return type == THM; }
// Return hypothesis or assertion pointer of the proof step.
    const void * ptr() const
    {
        if (ishyp()) return phyp;
        else if (isthm()) return pass;
        return NULL;
    }
// Return typecode. Return "" if not HYP nor THM.
    strview typecode() const;
// Return symbol of the variable.
    Symbol3 var() const;
// Return id of the variable.
    Symbol2::ID id() const { return var().id; }
// Return name of the proof step.
    operator const char *() const;
    struct Builder
    {
        Builder(Hypotheses const & hyps, Assertions const & assertions)
                : m_hyps(hyps), m_assertions(assertions) {}
// label -> proof step, using hypotheses and assertions.
        Proofstep operator()(strview label) const;
    private:
        Hypotheses const & m_hyps;
        Assertions const & m_assertions;
    };  // struct Proofstep::Builder
};  // struct Proofstep
inline bool operator==(Proofstep x, Proofstep y)
    { return x.ptr() == y.ptr(); }
inline bool operator<(Proofstep x, Proofstep y)
    { return std::less<const void *>()(x.ptr(), y.ptr()); }

// (Range of steps, begin of subAST)
struct SteprangeAST: std::pair<Steprange, ASTiter>
{
    SteprangeAST(Steprange range, ASTiter iter) :
        std::pair<Steprange, ASTiter>(range, iter) {}
    SteprangeAST(Steprange range, AST const & ast) :
        std::pair<Steprange, ASTiter>(range, ast.begin()) { check(ast); }
    SteprangeAST(Proofsteps const & proofsteps, AST const & ast) :
        std::pair<Steprange, ASTiter>(proofsteps, ast.begin()) { check(ast); }
    Steprange RPN() const { return first; }
    ASTiter   ast() const { return second;}
    bool empty() const { return RPN().empty(); }
    void clear() { first.clear(); }
    Proofsize size() const { return RPN().size(); }
    void check(AST const & ast) { if (size() != ast.size()) clear(); }
    Proofstep const & RPNroot() const { return RPN().root(); }
    ASTnode const & ASTroot() const { return ast()[size() - 1]; }
    // Child i's subrange
    SteprangeAST child(ASTnode::size_type index) const
    {
        ASTiter const ASTend = ast() + size();
        ASTnode const & ASTback = ASTend[-1];
        ASTiter const newAST = index == 0 ? ast() :
            ASTend - 1 - (ASTback.back() - ASTback[index - 1]);
        Stepiter newRPN = newAST - ast() + RPN().first;
        Stepiter newend = RPN().second - 1 - (ASTback.back() - ASTback[index]);
        return SteprangeAST(Steprange(newRPN, newend), newAST);
    }
};

struct Relations;
// Type of relations
typedef unsigned Reltype;
// Map: type -> relations of that type
typedef std::map<Reltype, Relations> Relationmap;

// Append an expression to another.
template<class T, class U>
std::vector<T> & operator+=(std::vector<T> & a, std::vector<U> const & b)
{
    a.reserve(b.end() - b.begin() + a.size());
    a.insert(a.end(), b.begin(), b.end());
    return a;
}

#endif // TYPES_H_INCLUDED
