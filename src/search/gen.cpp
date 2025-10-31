#include "gen.h"
#include "../io.h"
#include "../syntaxiom.h"
#include "../util/for.h"
#include "../util/iter.h"

// Return the typecodes of the arguments of a syntax axiom.
static Argtypes argtypes(Proofsteps const & assRPN)
{
    if (assRPN.empty())
        return Argtypes();

    Argtypes result(assRPN.size() - 1);
    for (Proofsize i = 0; i < assRPN.size() - 1; ++i)
    {
        if (assRPN[i].type != Proofstep::HYP)
            return Argtypes();
        Hypothesis const & hyp = assRPN[i].phyp->second;
        if (hyp.expression.empty())
            return Argtypes();
        result[i] = (hyp.expression[0]);
    }

    return result;
}

// Return the sum of the sizes of substituted arguments.
static Proofsize argssize
    (Argtypes const & argtypes, Genresult const & result,
     Genstack const & stack)
{
    Proofsize count = 0;
    for (Genstack::size_type i = 0; i < stack.size(); ++i)
        count += result.at(argtypes[i])[stack[i]].size();
    return count;
}

// Return a lower bound of the number of potential substitutions.
Proofsize Gen::substcount(Argtypes const & argtypes, Genstack const & stack) const
{
    Proofsize count = 1;
    for (Genstack::size_type i = 0; i < stack.size(); ++i)
    {
        // Type of the substitution
        strview type = argtypes[i];
        // Index of the substitution
        Terms::size_type index = stack[i];
        count *= termcounts[type][genresult[type][index].size()];
    }
    return count;
}

// Advance the stack. Return true if stack is advanced.
bool Gen::next(Argtypes const & argtypes, Proofsize size, Genstack & stack) const
{
    Proofsize const argcount = argtypes.size();
    while (!stack.empty())
    {
        // Check if size of the last substitution is maximal.
        if (argssize(argtypes, genresult, stack)
            < size - 1 - argcount + stack.size())
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

static Proofsteps writeRPN
    (Argtypes const & argtypes, Genresult const & result,
     Genstack const & stack, Proofstep root)
{
    Proofsteps rpn;
    // Preallocate for efficiency.
    rpn.reserve(argssize(argtypes, result, stack));

    for (Genstack::size_type i(0); i < stack.size(); ++i)
        rpn += result.at(argtypes[i])[stack[i]];
    rpn.push_back(root);

    return rpn;
}

// Adds a generated term.
struct Termadder : Adder
{
    Terms & terms;
    Proofstep root;
    Termadder(Terms & terms, Proofstep root) : terms(terms), root(root) {}
    virtual void operator()(Argtypes const & types, Genresult const & result,
                            Genstack const & stack)
    { terms.push_back(writeRPN(types, result, stack, root)); }
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
            terms.push_back(Proofsteps(1, var.first.phyp));

    // Generate all 1-step syntax axioms.
    FOR (Syntaxioms::const_reference syntaxiom, syntaxioms)
    {
        Assertion const & ass(syntaxiom.second.assiter->second);
        if (ass.expRPN.size() == 1 && ass.expression[0] == type)
            terms.push_back(ass.expRPN);
    }

    return terms;
}

// Generate all terms with RPN up to a given size.
// Stop and return false when max count is exceeded.
bool Gen::generateupto(strview type, Proofsize size) const
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
        return true; // already generated

    if (!generateupto(type, size - 1))
        return false;// argument generation failed

    FOR (Syntaxioms::const_reference syntaxiom, syntaxioms)
    {
        Assertion const & ass = syntaxiom.second.assiter->second;
        if (ass.expRPN.size() <= 1 || ass.expRPN.size() > size ||
            ass.expression[0] != type)
            continue; // Syntax axiom mismatch

        Argtypes const & types = argtypes(ass.expRPN);
        if (types.empty())
            continue; // Bad syntax axiom

        // Callback functor to add terms
        Termadder adder(terms, ass.expRPN.back());
        // Main loop of term generation
        if (!dogenerate(types, size, adder))
            return false; // Argument generation failed
    }
    // Record the # of terms.
    countbysize.insert(countbysize.end(),
                       size + 1 - countbysize.size(), terms.size());
    return true;
}

// Generate all terms for all arguments with RPN up to a given size.
// Stop and return false when max count is exceeded.
bool Gen::dogenerate(Argtypes const & argtypes, Proofsize size, Adder & adder) const
{
    // Stack of terms to be tried
    Genstack stack;
    // Preallocate for efficiency.
    Proofsize const argcount = argtypes.size();
    stack.reserve(argcount);
    do
    {
        if (stack.size() < argcount) // Not all args seen
        {
            strview type = argtypes[stack.size()];
            Proofsize argsize = size - argcount + stack.size()
                - argssize(argtypes, genresult, stack);
            if (!generateupto(type, argsize)
                || genresult[type].empty())
                return false; // Argument generation failed

            if (stack.size() < argcount - 1) // At least 2 args unseen
                stack.push_back(0);
            else
            {
                // Size of the only unseen arg
                Proofsize lastsize = size - 1 - argssize(argtypes, genresult, stack);
                // 1st substitution with that size
                stack.push_back(termcounts[type][lastsize - 1]);
                // # substitutions to be generated
                // if (substcount(argtypes, stack) > m_maxcount/16)
                //     return false;  // Too many substitutions
            }
            continue;
        }
        // All arguments seen. Write RPN of term.
        adder(argtypes, genresult, stack);
        // Try the next substitution.
        if (!next(argtypes, size, stack))
            break;
    } while (!stack.empty());
    
    return true;
}

// Generate all terms whose RPN is of a given size.
// Terms generate
//     (Varusage const & varusage, Syntaxioms const & syntaxioms,
//      strview type, Proofsize size)
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
//                strview type, Proofsize size)
// {
//     Syntaxioms filtered;
//     FOR (Syntaxioms::const_reference syntaxiom, syntaxioms)
//         if (syntaxiom.second.assiter->second.number <= assertion.number)
//             filtered.insert(syntaxiom);

//     return generate(assertion.varusage, filtered, type, size);
// };
