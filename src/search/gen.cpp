#include "gen.h"
#include "../io.h"
#include "../syntaxiom.h"
#include "../util/FMA.h"
#include "../util/for.h"

// Return the typecodes of the arguments of a syntax axiom.
static Argtypes argtypes(RPN const & syntaxiom)
{
    if (syntaxiom.empty())
        return Argtypes();

    Argtypes result(syntaxiom.size() - 1);
    for (RPNsize i = 0; i < syntaxiom.size() - 1; ++i)
    {
        RPNstep const step = syntaxiom[i];
        if (!step.id())
            return Argtypes();
        result[i] = step.phyp->second.expression[0];
    }

    return result;
}

// Return the sum of the sizes of substituted arguments.
static RPNsize argssize
    (Argtypes const & argtypes, Genresult const & result,
     Genstack const & stack)
{
    RPNsize size = 0;
    for (Genstack::size_type i = 0; i < stack.size(); ++i)
        size += result.at(argtypes[i])[stack[i]].size();
    return size;
}

// Return a lower bound of the number of potential substitutions.
RPNsize Gen::substcount(Argtypes const & argtypes, Genstack const & stack) const
{
    RPNsize count = 1;
    for (Genstack::size_type i = 0; i < stack.size(); ++i)
    {
        // Type of the substitution
        strview type = argtypes[i];
        // Multiplier
        RPNsize mul = termcounts[type][genresult[type][stack[i]].size()];
        if (!util::FMA(count, mul, 0))
            return static_cast<RPNsize>(-1);
    }
    return count;
}

// Advance the stack and return true if it can be advanced.
// Clear the stack and return false if it cannot be advanced.
bool Gen::next(Argtypes const & argtypes, RPNsize size, Genstack & stack) const
{
    while (!stack.empty())
    {
        // Check if size of the last substitution is maximal.
        if (argssize(argtypes, genresult, stack)
            < size - 1 - argtypes.size() + stack.size())
            break;
        // Type of the last substitution
        strview type = argtypes[stack.size() - 1];
        // Index of the last substitution
        Terms::size_type index = stack.back();
        // Check if indexe of the last substitution is maximal.
        if (index < termcounts[type][genresult[type][index].size()] - 1)
            break;
        // It is maximal. Backtrack to the previous substitution.
        stack.pop_back();
    }
    return !stack.empty() && ++stack.back();
}

static RPN writeRPN
    (Argtypes const & argtypes, Genresult const & result,
     Genstack const & stack, RPNstep const root)
{
    RPN term;
    // Preallocate for efficiency.
    term.reserve(argssize(argtypes, result, stack));

    for (Genstack::size_type i = 0; i < stack.size(); ++i)
        term += result.at(argtypes[i])[stack[i]];
    term.push_back(root);

    return term;
}

// Adds a generated term.
struct Termadder : Adder
{
    Terms & terms;
    RPNstep const root;
    Termadder(Terms & terms, RPNstep const root) :
        terms(terms), root(root) {}
    // Add a move. Return true if the move closed the goal.
    virtual bool operator()(Argtypes const & types, Genresult const & result,
                            Genstack const & stack)
    {
        terms.push_back(writeRPN(types, result, stack, root));
        return false;
    }
};

// Generate all terms of size 1.
Terms Gen::generateupto1(strview type) const
{
    Terms terms;
    // Preallocate for efficiency.
    terms.reserve(m_varusage.size());

    // Generate all variables of the type.
    FOR (Varusage::const_reference var, m_varusage)
        if (var.first.typecode() == type)
            terms.push_back(RPN(1, var.first.iter));

    // Generate all 1-step syntax axioms.
    FOR (Syntaxioms::const_reference syntaxiom, syntaxioms)
    {
        Assertion const & ass(syntaxiom.second.pass->second);
        if (ass.expRPN.size() == 1 && ass.exptypecode() == type)
            terms.push_back(ass.expRPN);
    }

    return terms;
}

// Generate all terms with RPN up to a given size.
// Stop and return false when max count is exceeded.
void Gen::generateupto(strview type, RPNsize size) const
{
    Terms & terms = genresult[type];
    Termcounts::mapped_type & countbysize = termcounts[type];
    // Preallocate for efficiency.
    countbysize.reserve(size + 1);

    if (countbysize.empty())
    {
        terms = generateupto1(type);
        // Record # of size 1 terms.
        countbysize.resize(2);
        countbysize[1] = terms.size();
    }

    if (countbysize.size() >= size + 1)
        return; // already generated

    generateupto(type, size - 1);

    FOR (Syntaxioms::const_reference syntaxiom, syntaxioms)
    {
        Assertion const & ass = syntaxiom.second.pass->second;
        if (ass.expRPN.size() <= 1 || ass.expRPN.size() > size ||
            ass.exptypecode() != type)
            continue; // Syntax axiom mismatch

        Argtypes const & types = argtypes(ass.expRPN);
        if (types.empty())
            continue; // Bad syntax axiom

        // Callback functor to add terms
        Termadder adder(terms, ass.expRPN.back());
        // Main loop of term generation
        dogenerate(types, size, adder);
    }
    // Record the # of terms.
    countbysize.insert(countbysize.end(),
                       size + 1 - countbysize.size(), terms.size());
}

// Generate all terms for all arguments with RPN up to a given size.
// Skip when max count is exceeded.
// Return true if a move closed the goal.
bool Gen::dogenerate(Argtypes const & argtypes, RPNsize size, Adder & adder) const
{
    // Stack of terms to be tried
    Genstack stack;
    // Preallocate for efficiency.
    RPNsize const nargs = argtypes.size();
    stack.reserve(nargs);
    do
    {
        if (stack.size() < nargs) // Not all args seen
        {
            strview type = argtypes[stack.size()];
            RPNsize argsize = size - nargs + stack.size();
            argsize -= argssize(argtypes, genresult, stack);
            generateupto(type, argsize);
            if (genresult[type].empty())
                return false; // Argument generation failed

            if (stack.size() < nargs - 1) // At least 2 args unseen
                stack.push_back(0);
            else
            {
                // Size of the only unseen arg
                RPNsize lastsize = size - 1 - argssize(argtypes, genresult, stack);
                // 1st substitution with that size
                stack.push_back(termcounts[type][lastsize - 1]);
            }
        }
        else
        {
            // All arguments seen. Write RPN of term.
            if (substcount(argtypes, stack) <= m_maxmoves)
                if (adder(argtypes, genresult, stack))
                    return true;
            // Try the next substitution.
            next(argtypes, size, stack);
        }
    } while (!stack.empty());
    
    return true;
}

// Generate all terms whose RPN is of a given size.
// Terms generate
//     (Varusage const & varusage, Syntaxioms const & syntaxioms,
//      strview type, RPNsize size)
// {
//     if (size == 0)
//         return Terms();

//     Genresult result;
//     Termcounts counts;
//     generateupto(varusage, syntaxioms, type, size, result, counts);

//     return Terms(result[type].begin() + counts[type][size - 1],
//                  result[type].end());
// }

// Terms generate(struct Assertion const & assertion,
//                Syntaxioms const & syntaxioms,
//                strview type, RPNsize size)
// {
//     Syntaxioms filtered;
//     FOR (Syntaxioms::const_reference syntaxiom, syntaxioms)
//         if (syntaxiom.second.assiter->second.number <= assnum())
//             filtered.insert(syntaxiom);

//     return generate(assertion.varusage, filtered, type, size);
// };
