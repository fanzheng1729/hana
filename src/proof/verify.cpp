#include "../ass.h"
// #include "../database.h"
#include "../disjvars.h"
#include "../io.h"
#include "../util/arith.h"
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

void printunificationfailure
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
     Disjvars const & disjvars)
{
    std::cerr << "The substitutions\n" << var1 << ":\t" << exp1;
    std::cerr << var2 << ":\t" << exp2;
    std::cerr << "violate disjoint variable hypothesis in:\n" << disjvars;
}

// Check disjoint variable hypothesis in verifying an assertion reference.
static bool checkDV
    (Substitutions const & subst, Disjvars const & thmDV, Assertion const & ass)
{
    FOR (Disjvars::const_reference vars, thmDV)
    {
        Substitutions::value_type exp1(subst[vars.first]);
        Substitutions::value_type exp2(subst[vars.second]);

        if (!checkdisjvars(exp1.first, exp1.second, exp2.first, exp2.second,
                           ass.disjvars, &ass.varusage))
        {
            printdisjvarserr(vars.first, Expression(exp1.first, exp1.second),
                             vars.second, Expression(exp2.first, exp2.second),
                             ass.disjvars);
            return false;
        }
    }

    return true;
}

// Subroutine for proof verification. Verify a proof step referencing an
// assertion (i.e., not a hypothesis).
static bool verifystep
    (Assptr pass, Assptr pthm, std::vector<Expression> & stack,
     Substitutions & substitutions)
{
    strview label = pass ? pass->first : "";
    Assertion const & thm = pthm->second;

    // Find the necessary substitutions.
    prealloc(substitutions, thm.varusage);
    std::vector<Expression>::size_type const base = findsubstitutions
        (label, pthm->first, pthm->second.hypiters, stack, substitutions);
    if (base == stack.size())
        return false;
//std::cout << "Substitutions" << std::endl << substitutions;

    // Verify disjoint variable conditions.
    if (pass)
        if (!checkDV(substitutions, thm.disjvars, pass->second))
        {
            std::cerr << "In step " << pthm->first;
            return printinproofof(label);
        }

    // Insert new statement onto stack.
    makesubstitution(thm.expression, stack.back(), substitutions,
        util::mem_fn(&Symbol3::id));
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
Expression verify(Proofsteps const & proof, Printer & printer, Assptr pass)
{
    strview label = pass ? pass->first : "";
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

// Verify a regular proof. The "proof" argument should be a non-empty sequence
// of valid labels. Return the statement the "proof" proves.
// Return the empty expression if the "proof" is invalid.
// Expression verifyregular
//     (strview label, class Database const & database,
//      Proof const & proof, Hypotheses const & hypotheses)
// {
//     Assertions const & assertions = database.assertions();
//     Assiter const iter = assertions.find(label);
//     Assptr const pass = iter == assertions.end() ? NULL : &*iter;

//     Proofsteps steps(regular(proof, hypotheses, assertions));
//     if (steps.empty())
//     {
//         std::cout << " in regular proof of " << label << std::endl;
//         return Expression();
//     }

//     return verify(steps, pass);
// }

// Return if "conclusion" == "expression" to be proved.
bool provesrightthing
    (strview label, Expression const & conclusion, Expression const & expression)
{
    if (conclusion == expression)
        return true;

    std::cerr << "Proof of theorem " << label << " proves wrong statement:\n"
              << conclusion << "instead of:\n" << expression;
    return false;
}
