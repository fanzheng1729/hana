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

// Proof is a sequence of labels.
typedef std::vector<std::string> Proof;
// Iterator to a proof
typedef Proof::const_iterator Prfiter;
// Begin and end of a subproof
typedef std::pair<Prfiter, Prfiter> Subprf;
// A sequence of subproofs
typedef std::vector<Subprf> Subprfs;

// Step in a regular or compressed proof
struct Proofstep;
// A sequence of proof steps
typedef std::vector<Proofstep> Proofsteps;
// # of step in the proof
typedef Proofsteps::size_type Proofsize;
// Iterator to a proof step
typedef Proofsteps::const_iterator Stepiter;
// Begin and end of a range of steps
typedef std::pair<Stepiter, Stepiter> Steprange;
// Ranges of steps to be substituted
typedef std::vector<Steprange> Stepranges;
// Pointers to the proofs to be included
typedef std::vector<Proofsteps const *> pProofs;

// Node of an abstract syntax tree, listing the indices of all its hypotheses
typedef std::vector<Proofsize> ASTnode;
// Abstract syntax tree
typedef std::vector<ASTnode> AST;
// Iterator to an AST node
typedef AST::const_iterator ASTiter;

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
typedef Hypotheses::const_pointer Hypptr;
// Iterator to a hypothesis
typedef Hypotheses::const_iterator Hypiter;
// Return true if the name of the hypothesis pointed to is label.
inline bool operator==(Hypiter iter, strview label) {return iter->first==label;}
// A sequence of hypothesis iterators
typedef std::vector<Hypiter> Hypiters;
// # of hypotheses
typedef Hypiters::size_type Hypsize;

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
    // Pointer to the floating hypothesis for variable, NULL for constant
    Hypptr phyp;
    strview typecode() const { return phyp->second.expression[0]; }
    Symbol3(strview str = "", ID n = 0, Hypptr p = NULL) :
        Symbol2(str, n), phyp(p) {}
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
typedef Assertions::const_pointer Assptr;
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
        NONE, HYP, ASS, LOAD, SAVE
    } type;
    union
    {
        Hypptr phyp;
        Assptr pass;
        Index index;
    };
    Proofstep(Type t = NONE) : type(t == SAVE ? t : NONE) {}
    Proofstep(Hypptr p) : type(p ? HYP : NONE), phyp(p) {}
    Proofstep(Assptr p) : type(p ? ASS : NONE), pass(p) {}
    Proofstep(Hypiter iter) : type(HYP), phyp(&*iter) {}
    Proofstep(Index i) : type(LOAD), index(i) {}
// Return hypothesis or assertion pointer of the proof step.
    const void * ptr() const
    {
        if (type == HYP)
            return phyp;
        else if (type == ASS)
            return pass;
        return NULL;
    }
// Return symbol of the variable.
    Symbol3 symbol() const;
// Return id of the variable.
    Symbol2::ID id() const { return symbol().id; }
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
inline bool operator<(Proofstep x, Proofstep y)
    { return std::less<const void *>()(x.ptr(), y.ptr()); }

// (Range of steps, begin of subAST)
struct SteprangeAST: std::pair<Steprange, ASTiter>
{
    using std::pair<Steprange, ASTiter>::pair;
    // Return the subrange corresponding to child i.
    SteprangeAST child(ASTnode::size_type index) const
    {
        ASTiter const ASTend = first.second - first.first + second;
        ASTnode const & ASTback = *(ASTend - 1);
        ASTiter const newAST = index == 0 ? second :
            ASTend - 1 - (ASTback.back() - ASTback[index - 1]);
        Stepiter newRPN = newAST - second + first.first;
        Stepiter newend = first.second - 1 - (ASTback.back() - ASTback[index]);
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
