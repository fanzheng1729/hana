#include "../ass.h"
// #include "../database.h"
#include "../disjvars.h"
#include "../io.h"
#include "printer.h"
#include "../util/filter.h"
#include "../util/msg.h"
#include "verify.h"

// Substitution vector
typedef std::vector<std::pair<const Symbol3 *, const Symbol3 *> > Substitutions;

// Extract proof steps from a compressed proof.
Proofsteps compressed
    (Proofsteps const & labels, Proofnumbers const & proofnumbers)
{
    Proofsteps result(proofnumbers.size()); // Preallocate for efficiency
    Proofsize const labelcount = labels.size();

    for (Proofnumbers::size_type step = 0; step < proofnumbers.size(); ++step)
    {
        Proofnumber const number = proofnumbers[step];
        result[step] = number == 0 ? Proofstep(Proofstep::SAVE) :
                        number <= labelcount ? labels[number - 1] :
                            Proofstep(number - labelcount - 1);
    }

    return result;
}

// Extract proof steps from a regular proof.
Proofsteps regular
    (Proof const & proof,
     Hypotheses const & hypotheses, Assertions const & assertions)
{
    // Preallocate for efficiency
    Proofsteps result(proof.size());
    std::transform(proof.begin(), proof.end(), result.begin(),
                   Proofstep::Builder(hypotheses, assertions));
    return util::filter(result)((const char *)0) ? Proofsteps() : result;
}

static bool printinproofof(strview label, bool okay = false)
{
    if (!okay)
        std::cerr << " in proof of theorem " << label << std::endl;
    return okay;
}

// Return true if there is enough item on the stack for hypothesis verification.
bool enoughitemonstack
    (std::size_t hypcount, std::size_t stacksize, strview label)
{
    static const char hypfound[] = " hypotheses found, ";
    bool const okay(is1stle2nd(hypcount, stacksize, hypfound, itemonstack));
    return printinproofof(label, okay);
}

static void printunificationfailure
    (strview label, strview thmlabel, Hypothesis const & hyp,
     Expression const & dest, Expression const & stackitem)
{
    std::cerr << "In step " << thmlabel; printinproofof(label);
    std::cerr << (hyp.floats ? "floating" : "essential") << " hypothesis ";
    std::cerr << hyp.expression << "expanded to\n" << dest;
    std::cerr << "does not match stack item\n" << stackitem;
}

static void printdisjvarserr
    (strview var1,Expression const & exp1,strview var2,Expression const & exp2,
     Disjvars const & DV)
{
    std::cerr << "The substitutions\n" << var1 << ":\t" << exp1;
    std::cerr << var2 << ":\t" << exp2;
    std::cerr << "violate disjoint variable hypothesis\n";
    std::cerr << "Disjoint variable hypotheses of the assertion:\n" << DV;
}

// Check disjoint variable hypothesis in verifying an assertion reference.
static bool checkDV
    (Substitutions const & subst, Disjvars const & thmDV, Assertion const & ass)
{
    FOR (Disjvars::const_reference vars, thmDV)
    {
        Substitutions::value_type exp1(subst[vars.first]);
        Substitutions::value_type exp2(subst[vars.second]);

        if (!checkDV(exp1, exp2, ass.disjvars, ass.varusage))
        {
            printdisjvarserr(vars.first, Expression(exp1.first, exp1.second),
                             vars.second, Expression(exp2.first, exp2.second),
                             ass.disjvars);
            return false;
        }
    }

    return true;
}

static Expression::size_type substitutionsize
    (Expression const & src, Substitutions const & substitutions)
{
    Expression::size_type size = 0;
    FOR (Symbol3 symbol, src)
        if (Symbol3::ID const id = symbol.id)
            size += substitutions[id].second - substitutions[id].first;
        else
            ++size;
    return size;
}

static void makesubstitution
    (Expression const & src, Expression & dest,
     Substitutions const & substitutions)
{
    if (substitutions.empty())
        return dest.assign(src.begin(), src.end());
    // Preallocate for efficiency
    dest.reserve(substitutionsize(src, substitutions));
    // Make the substitution
    FOR (Symbol3 symbol, src)
        if (Symbol3::ID const id = symbol.id)
            dest += substitutions[id];  // variable with an id
        else
            dest.push_back(symbol);     // constant with no id
}

// Find the substitution. Increase the size of the stack by 1.
// Return index of the base of the substitution in the stack.
// Return the size of the stack if not okay.
static typename std::vector<Expression>::size_type findsubstitutions
    (strview label, strview thmlabel, Hypiters const & hypiters,
     std::vector<Expression> & stack, Substitutions & substitutions)
{
    Hypsize const hypcount = hypiters.size();
    if (!enoughitemonstack(hypcount, stack.size(), label))
        return stack.size();

    // Space for new statement onto stack
    stack.resize(stack.size() + 1);
    
    Hypsize const base = stack.size() - 1 - hypcount;

    // Determine substitutions and check that we can unify
    for (Hypsize i = 0; i < hypcount; ++i)
    {
        Hypothesis const & hypothesis = hypiters[i]->second;
        if (hypothesis.floats)
        {
            // Floating hypothesis of the referenced assertion
            if (hypothesis.expression[0] != stack[base + i][0])
            {
                printunificationfailure(label, thmlabel, hypothesis,
                                        hypothesis.expression, stack[base + i]);
                return stack.size();
            }
            Symbol2::ID const id = hypothesis.expression[1];
            substitutions.resize(std::max(id + 1, substitutions.size()));
            substitutions[id]
            = std::make_pair(&stack[base + i][1], &stack[base + i].back() + 1);
        }
        else
        {
            // Essential hypothesis
            Expression dest;
            makesubstitution(hypothesis.expression, dest, substitutions);
            if (dest != stack[base + i])
            {
                printunificationfailure(label, thmlabel, hypothesis,
                                        dest, stack[base + i]);
                return stack.size();
            }
        }
    }

    return base;
}

// Subroutine for proof verification. Verify a proof step referencing an
// assertion (i.e., not a hypothesis).
static bool verifystep
    (pAss pass, pAss pthm, std::vector<Expression> & stack,
     Substitutions & substitutions)
{
    strview label = pass ? pass->first : strview();
    Assertion const & thm = pthm->second;

    // Find the necessary substitutions.
    substitutions.clear();
    substitutions.resize(thm.maxvarid() + 1);
    std::vector<Expression>::size_type const base = findsubstitutions
        (label, pthm->first, pthm->second.hypiters, stack, substitutions);
    if (base == stack.size())
        return false;
//std::cout << "Substitutions" << std::endl << substitutions;

    // Verify disjoint variable conditions.
    if (pass && !checkDV(substitutions, thm.disjvars, pass->second))
    {
        std::cerr << "In step " << pthm->first;
        return printinproofof(label);
    }

    // Insert new statement onto stack.
    makesubstitution(thm.expression, stack.back(), substitutions);
    // Remove hypotheses from stack.
    stack.erase(stack.begin() + base, stack.end() - 1);

    return true;
}

// Return true if the index of a load step is within the bound.
static bool enoughsavedsteps
    (Proofstep::Index index, Proofstep::Index savedsteps, strview label)
{
    bool okay = is1stle2nd(index + 1, savedsteps, "steps needed", "steps saved");
    return printinproofof(label, okay);
}

// Subroutine for proof verification. Verify proof steps.
Expression verify(Proofsteps const & proof, Printer & printer, pAss pass)
{
    strview label = pass ? pass->first : strview();
//std::cout << "Verifying " << label << std::endl;
    std::vector<Expression> stack, savedsteps;

    Substitutions substitutions;

    FOR (Proofstep const & step, proof)
    {
        switch (step.type)
        {
        case Proofstep::HYP:
//std::cout << "Pushing hypothesis: " << step.phyp->first << '\n';
            stack.push_back(step.phyp->second.expression);
            break;
        case Proofstep::THM:
//std::cout << "Applying assertion: " << step.pass->first << '\n';
            if (!verifystep(pass, step.pass, stack, substitutions))
                return Expression();
            break;
        case Proofstep::LOAD:
//std::cout << "Loading saved step " << step.index << std::endl;
            if (!enoughsavedsteps(step.index, savedsteps.size(), label))
                return Expression();
            stack.push_back(savedsteps[step.index]);
            break;
        case Proofstep::SAVE:
//std::cout << "Saving step " << savedsteps.size() << std::endl;
            if (stack.empty())
            {
                std::cerr << "No step to save";
                printinproofof(label);
                return Expression();
            }
            savedsteps.push_back(stack.back());
            break;
        default:
            std::cerr << "Invalid step";
            printinproofof(label);
            return Expression();
        }
        if (!printer.addstep(step, &step - &proof[0], stack.back()))
            return Expression();
    }

    if (stack.size() != 1)
    {
        std::cerr << "Proof of theorem " << label << stackszerr << std::endl;
        return Expression();
    }

    return Expression(stack[0].begin(), stack[0].end());
}
Expression verify(Proofsteps const & proof, pAss pass)
{
    Printer printer;
    return verify(proof, printer, pass);
}

// Verify a regular proof. The "proof" argument should be a non-empty sequence
// of valid labels. Return the statement the "proof" proves.
// Return the empty expression if the "proof" is invalid.
// Expression verifyregular
//     (strview label, class Database const & database,
//      Proof const & proof, Hypotheses const & hypotheses)
// {
//     Assertions const & assertions = database.assertions();
//     Assiter const iter = assertions.find(label);
//     pAss const pass = iter == assertions.end() ? NULL : &*iter;

//     Proofsteps steps(regular(proof, hypotheses, assertions));
//     if (steps.empty())
//     {
//         std::cout << " in regular proof of " << label << std::endl;
//         return Expression();
//     }

//     return verify(steps, pass);
// }

// Return if "conclusion" == "expression" to be proved.
bool checkconclusion
    (strview label, Expression const & conclusion, Expression const & expression)
{
    if (conclusion == expression)
        return true;

    std::cerr << "Proof of theorem " << label << " proves wrong statement:\n"
              << conclusion << "instead of:\n" << expression;
    return false;
}
