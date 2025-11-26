// #include <iostream> // I/O library
#include "DPLL.h"

using namespace std;

/**
 * Marks that the next element in the backtrack stack is a decided literal.
 */
#define DECISION_MARK 0

/**
 * The amount that is added to the activity of a literal each time it is involved
 * in a conflict.
 */
#define ACTIVITY_INCREMENT 1.0

/**
 * Default activity-based heuristic parameter. The activity of the literals will be
 * halved every 1000 conflicts.
 *
 * @see getNextDecisionLiteral() and updateActivityForConflictingClause(...)
 */
#define ACT_INC_UPDATE_RATE 1000

/**
 * General type for counting
 */
typedef std::size_t uint;

/**
 * General type for activity
 */
typedef double activity;

/**
 * The number of variables of the satisfiability problem.
 */
uint numVariables;

/**
 * The number of clauses of the formula of the satisfiability problem.
 */
uint numClauses;

/**
 * The list of clauses of the problem.
 */
sCNF scnf;

/**
 * The occurrence list of positive appearances for each value in the clause set.
 */
vector<vector<sCNFClause* > > positiveClauses;

/**
 * The occurrence list of negative appearances for each value in the clause set.
 */
vector<vector<sCNFClause* > > negativeClauses;

/**
 * The current model (interpretation) of the problem.
 */
vector<int> model;

/**
 * The stack that tracks the current execution state (the backtrack stack).
 */
vector<sLiteral> modelStack;

/**
 * An index indicating which is the next literal from the stack to be propagated.
 */
uint indexOfNextLiteralToPropagate;

/**
 * The current decision level of the DPLL algorithm.
 */
uint decisionLevel;

/**
 * The activity (number of conflicts in which appears) for each positive literal.
 */
vector<activity> positiveLiteralActivity;

/**
 * The activity (number of conflicts in which appears) for each negative literal.
 */
vector<activity> negativeLiteralActivity;

/**
 * The total number of conflicts found during the DPLL execution.
 */
uint conflicts;

/**
 * Returns the variable that this literal represents.
 */
inline Atom var(sLiteral literal) { return std::abs(literal); }

/**
 * Reads the CNF and initializes
 * any remaining necessary data structures and variables.
 */
void parseInput(CNFClauses const & cnf) {
    numVariables = cnf.atomcount();
    numClauses = cnf.size();

	// Initialize positive and negative appearances
	if (numVariables >= positiveClauses.size())
        positiveClauses.resize(numVariables + 1);
	if (numVariables >= negativeClauses.size())
        negativeClauses.resize(numVariables + 1);
	for (Atom i = 1; i <= numVariables; ++i) {
	    positiveClauses[i].clear();
        negativeClauses[i].clear();
        positiveClauses[i].reserve(numClauses);
        negativeClauses[i].reserve(numClauses);
	}

	// Read clauses
    if (numClauses > scnf.size()) {
        scnf.resize(numClauses);
    }
	for (sCNF::size_type clause = 0; clause < numClauses; ++clause) {
        sCNFClause::size_type size = cnf[clause].size();
        scnf[clause].resize(size);
		for (sCNFClause::size_type i = 0; i < size; ++i) {
            sLiteral literal = scnf[clause][i] = sliteral(cnf[clause][i]);
			// add to the list of positive-negative literals
			vector<sCNFClause* >& clauses = var(literal)
                [literal>0 ? positiveClauses.data() : negativeClauses.data()];
            clauses.push_back(&scnf[clause]);
		}
	}
	// std::cout << "Clauses read" << std::endl;
	// Initialize the remaining necessary variables
		// model and backtrack stack
	model.assign(numVariables + 1, UNKNOWN);
	modelStack.clear();
	indexOfNextLiteralToPropagate = 0;
	decisionLevel = 0;
		// heuristic
	positiveLiteralActivity.assign(numVariables + 1, 0.0);
	negativeLiteralActivity.assign(numVariables + 1, 0.0);
	conflicts = 0;
}

/**
 * Returns the current value of the given literal, evaluated in the current
 * interpretation (model).
 *
 * @param literal the literal which value is requested
 */
int currentValueForLiteral(sLiteral literal) {
    return literal >= 0 ? model[literal] :
        model[-literal] == UNKNOWN ? UNKNOWN : 1 - model[-literal];
}

/**
 * The current interpretation (model) is updated with the appropriate value
 * that will make the passed literal become true in the clause where it appeared.
 * The model stack is also increased, pushing the new value on top.
 *
 * @param literal the literal that will become true after the model update
 */
void setLiteralToTrue(sLiteral literal) {
	modelStack.push_back(literal);
	model[var(literal)] = literal > 0;
}

/**
 * Updates the activity counter of the given literal.
 *
 * @param literal the literal which activity is to be updated
 */
void updateActivityForLiteral(sLiteral literal) {
	//update the activity of the literal (we are not distinguishing between positive
	// and negative literals here)
	(literal > 0 ? positiveLiteralActivity : negativeLiteralActivity)
	[var(literal)] += ACTIVITY_INCREMENT;
}

/**
 * Updates the activity counters for all the literals in the given clause. The activity
 * increment that is applied is also updated, in order to give more importance to recent activity.
 *
 * @param clause the clause which was involved in the most recent conflict
 */
void updateActivityForConflictingClause(const sCNFClause& clause) {
	//update the activity increment if necessary (every X conflicts)
	++conflicts;
	if ((conflicts % ACT_INC_UPDATE_RATE) == 0) {
		//decaying sum
		for (Atom i = 1; i <= numVariables; ++i) {
			positiveLiteralActivity[i] /= 2.0;
			negativeLiteralActivity[i] /= 2.0;
		}
	}

	//update activity for each literal
	for (sCNFClause::size_type i = 0; i < clause.size(); ++i) {
		updateActivityForLiteral(clause[i]);
	}
}

/**
 * Performs the Boolean Constraint Propagation algorithm, traversing the set of clauses
 * for each literal that is to be propagated (in the model stack) and checking if a
 * conflict has been found or if a clause is unit.
 *
 * @return true if a conflict was found while performing the propagation; false otherwise
 */
bool propagateGivesConflict() {
	while (indexOfNextLiteralToPropagate < modelStack.size()) {
		//retrieve the literal to be propagated and move forward to the next.
		sLiteral literalToPropagate = modelStack[indexOfNextLiteralToPropagate++];

		//traverse only positive/negative appearances
		const vector<sCNFClause* >& clausesToPropagate = literalToPropagate>0 ?
				negativeClauses[var(literalToPropagate)] :
				positiveClauses[var(literalToPropagate)];

		//traverse the clauses
		for (uint i = 0; i < clausesToPropagate.size(); ++i) {
			//retrieve the next clause
			const sCNFClause& clause = *clausesToPropagate[i];

			//necessary variables initialization
			bool isSomeLiteralTrue = false;
			int undefinedLiterals = 0;
			sLiteral lastUndefinedLiteral = 0;

			//traverse the clause
			for (uint k = 0; !isSomeLiteralTrue && k < clause.size(); ++k) {
				int value = currentValueForLiteral(clause[k]);
				if (value == TRUE) {
					isSomeLiteralTrue = true;
				}
				else if (value == UNKNOWN) {
					++undefinedLiterals;
					lastUndefinedLiteral = clause[k];
				}
			}
			if (!isSomeLiteralTrue && undefinedLiterals == 0) {
				// A conflict has been found! All literals are false!
				updateActivityForConflictingClause(clause);
				return true;
			}
			else if (!isSomeLiteralTrue && undefinedLiterals == 1) {
				// The 'lastUndefinedLiteral' is "propagated", because it is the only one
				//  remaining in its clause that is undefined and all of the other literals
				//  of the clause are false
				// With the propagation, this literal is added to the stack, too!
				setLiteralToTrue(lastUndefinedLiteral);
			}
		}
	}
	return false;
}

/**
 * Resets the model and model stack to the last decision level.
 */
void backtrack() {
	sLiteral literal = 0;
	while (modelStack.back() != DECISION_MARK) { // 0 is the  mark
		literal = modelStack.back();
		model[var(literal)] = UNKNOWN;
		modelStack.pop_back();
	}
	// at this point, literal is the last decision
	modelStack.pop_back(); // remove the  mark
	--decisionLevel;
	indexOfNextLiteralToPropagate = modelStack.size();
	setLiteralToTrue(-literal);  // reverse last decision
}


/**
 * Selects the next undefined variable from the current interpretation (model)
 * that will be decided and propagated. The heuristic used is based on variable
 * activity, in order to quickly find any possible contradictions.
 *
 * @return the next variable to be decided within the DPLL procedure or 0 if no
 * variable is currently undefined
 */
sLiteral getNextDecisionLiteral() {
	activity maximumActivity = 0.0;
	sLiteral mostActiveVariable = 0; // in case no variable is undefined, it will not be modified
	for (Atom i = 1; i <= numVariables; ++i) {
		// check only undefined variables
		if (model[i] != UNKNOWN)
            continue;
        // search for the most active literal
        if (positiveLiteralActivity[i] >= maximumActivity) {
            maximumActivity = positiveLiteralActivity[i];
            mostActiveVariable = static_cast<sLiteral>(i);
        }
        if (negativeLiteralActivity[i] >= maximumActivity) {
            maximumActivity = negativeLiteralActivity[i];
            mostActiveVariable = -static_cast<sLiteral>(i);
        }
	}
	//return the most active variable or, if none is undefined, 0
	return mostActiveVariable;
}

/**
 * Executes the DPLL (Davis–Putnam–Logemann–Loveland) algorithm, performing a full search
 * for a model (interpretation) which satisfies the formula given as a CNF clause set.
 */
bool DPLL() {
	// DPLL algorithm
	while (true) {
		while (propagateGivesConflict()) {
			if (decisionLevel == 0) {
				//there are no more possible decisions (variable assignments),
				// which means that the problem is unsatisfiable
				return false;
			}
			backtrack();
		}
		sLiteral decisionLit = getNextDecisionLiteral();
		if (decisionLit == 0) {
			return true;
		}
		// start new decision level:
		modelStack.push_back(DECISION_MARK);  // push mark indicating new
		++indexOfNextLiteralToPropagate;
		++decisionLevel;
		setLiteralToTrue(decisionLit); // now push decisionLit on top of the mark
	}
}

/**
 * Checks for any unit clause and sets the appropriate values in the
 * model accordingly. If a contradiction is found among these unit clauses,
 * early failure is triggered.
 */
bool checkUnitClauses() {
	for (sCNF::size_type i = 0; i < numClauses; ++i) {
        if (scnf[i].empty())
            return false;
		if (scnf[i].size() == 1) {
			sLiteral literal = scnf[i][0];
			int value = currentValueForLiteral(literal);
			if (value == FALSE) {
				// This condition will only occur if at least a couple of unit clauses
				//  exist with opposite literals (p and ¬p, for example). When the first
				//  unit clause is used to set a value in the model (see the next condition),
				//  the second unit clause with the same variable will cause a contradiction
				//  to be found, i.e., the problem is unsatisfiable!
				return false;
			}
			else if (value == UNKNOWN) {
				setLiteralToTrue(literal);
			}
		}
	}

	return true;
}
