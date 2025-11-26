#ifndef DATABASE_H_INCLUDED
#define DATABASE_H_INCLUDED

#include "comment.h"
#include "def.h"
#include "propctor.h"
#include "relation.h"
#include "stat.h"
#include "syntaxiom.h"
#include "util/for.h"

class Database
{
    // Set of constants
    typedef std::set<strview> Constants;
    Constants m_constants;
    // Map: var -> id
    typedef std::map<strview, Symbol2::ID> VarIDmap;
    VarIDmap m_varIDmap;
    std::vector<strview> m_varvec;
    Hypotheses m_hypotheses;
    Assertions m_assertions;
    Assiters m_assiters;
    Syntaxioms m_syntaxioms;
    Commentinfo m_commentinfo;
    Relationmap m_relations;
    Definitions m_definitions;
    Propctors m_propctors;
public:
    Database() : m_assiters(1) { addvar(""); }
    bool read(Tokens & tokens, Comments const & comments,
              Tokens::size_type upto);
    void clear() { *this = Database(); }
    Symbol2::ID varid(strview str) const { return varIDmap().at(str); }
    VarIDmap const & varIDmap() const { return m_varIDmap; }
    std::vector<strview> const & varvec() const { return m_varvec; }
    Hypotheses const & hypotheses() const { return m_hypotheses; }
    Assertions const & assertions() const { return m_assertions; }
    Assiters const & assiters() const { return m_assiters; }
    Syntaxioms const & syntaxioms() const { return m_syntaxioms; }
    Syntaxioms primitivesyntaxioms() const
    {
        Syntaxioms axioms;
        FOR (Syntaxioms::const_reference axiom, syntaxioms())
            if (!definitions().count(axiom.first))
                axioms.insert(axiom);
        return axioms;
    }
    Commentinfo const & commentinfo() const { return m_commentinfo; }
    Ctordefns const & ctordefns() const { return commentinfo().ctordefns; }
    Typecodes const & typecodes() const { return commentinfo().typecodes; }
    Relationmap const & relations() const { return m_relations; }
    Relations const & relations(unsigned type) const
    {
        static const Relations empty;
        Relationmap::const_iterator iter(relations().find(type));
        return iter == relations().end() ? empty : iter->second;
    }
    Relations relations(unsigned mask, unsigned pattern) const
    {
        Relations rels;
        FOR (Relationmap::const_reference relation, relations())
            if ((relation.first & mask) == pattern)
                rels.insert(relation.second.begin(), relation.second.end());
        return rels;
    }
    Relations equalities() const
    { return relations(Relations::EQUIVALENCE, Relations::EQUIVALENCE); }
    Relations implications() const
    { return relations(Relations::AX1, Relations::AX1); }
    Relations negations() const
    { return relations(Relations::ID12, Relations::ID2); }
    Relations conjunctions() const
    { return relations(Relations::AND, Relations::AND); }
    Relations disjunctions() const
    { return relations(Relations::OR, Relations::OR); }
    Relations conjunctions3() const
    { return relations(Relations::A3AN, Relations::A3AN); }
    Relations disjunctions3() const
    { return relations(Relations::O3OR, Relations::O3OR); }
    Definitions const & definitions() const { return m_definitions; }
    Propctors const & propctors() const { return m_propctors; }
    bool hasconst(strview str) const { return m_constants.count(str); }
    bool addconst(strview str) { return m_constants.insert(str).second; }
    bool hasvar(strview str) const { return varIDmap().count(str); }
    void addvar(strview str)
    {
        if (m_varIDmap.insert(std::make_pair(str, varIDmap().size())).second)
            m_varvec.push_back(str);
    }
    bool hashyp(strview label) const { return hypotheses().count(label); }
    Hypiter addhyp(strview label, Expression const & exp, bool const floating)
    {
        Hypotheses::value_type value(label, Hypothesis(Expression(), floating));
        Hypotheses::iterator iter = m_hypotheses.insert(value).first;
        Expression & hypexp = iter->second.expression;
        hypexp = exp;
        if (floating) // Point the defined variable to the hypothesis
            hypexp[1].phyp = &*iter;
        return iter;
    }
    bool hasass(strview label) const { return assertions().count(label); }
    // Construct an Assertion from an Expression. That is, determine the
    // mandatory hypotheses and disjoint variable restrictions and the #.
    // The Assertion is inserted into the assertions collection.
    // Return the iterator to tha assertion.
    Assertions::iterator addass
        (strview label, Expression const & exp, struct Scopes const & scopes,
         Tokens::size_type tokenpos);
    // Types of tokens
    enum Tokentype {OKAY, CONST, VAR, LABEL};
    // Determine if a new constant, variable or label can be added
    // If type is CONST or VAR, the corresponding type is not checked
    Tokentype canaddcvlabel(strview token, int const type) const
    {
        if (type != CONST && hasconst(token))
            return CONST;
        if (type != VAR && hasvar(token))
            return VAR;
        if (hashyp(token) || hasass(token))
            return LABEL;
        return OKAY;
    }
    bool addtypecode(strview typecode, strview astypecode="", bool isbound=0)
    {
        std::pair<strview, bool> const mapped(astypecode, isbound);
        std::pair<strview, std::pair<strview, bool> > value(typecode, mapped);
        return m_commentinfo.typecodes.insert(value).second;
    }
// Add the revPolish notation to the whole database. Return true iff okay.
    bool addRPN()
    {
        m_syntaxioms = Syntaxioms(assertions(), *this);
        if (!syntaxioms().addRPN(m_assertions, typecodes()))
            return false;
        // Find the equivalence relations and their justifications among syntax axioms.
        Relations relations(assertions());
        FOR (Relations::const_reference relation, relations)
            m_relations[relation.second.type()].insert(relation);
        return true;
    }
// Test syntax parser. Return 1 iff okay.
    bool checkRPN() const;
// Mark duplicate assertions. Return its number.
    Assertions::size_type markduplicate();
// Find definitions in assertions.
    void loaddefinitions()
    {
        m_definitions = Definitions(assertions(), commentinfo(), equalities());
    }
// Return true if all the definitions are syntactically okay.
    bool checkdefinitions() const
    {
        return ::checkdefinitions(definitions(), typecodes());
    }
// Load data related to propositional calculus.
    void loadpropctors()
    {
        static const bool tts[][1 << 3] =
        {
            {1, 0, 0, 1},   // equalities
            {1, 0, 1, 1},   // implications
            {1, 0},         // negations
            {0, 0, 0, 1},   // conjunctions
            {0, 1, 1, 1},   // disjunctions
            {0, 0, 0, 0, 0, 0, 0, 1},   // 3conjunctions
            {0, 1, 1, 1, 1, 1, 1, 1},   // 3disjunctions
        };
        static const unsigned maskpatterns[][2] =
        {
            {Relations::EQUIVALENCE, Relations::EQUIVALENCE},
            {Relations::AX1, Relations::AX1},
            {Relations::ID12,Relations::ID2},
            {Relations::AND, Relations::AND},
            {Relations::OR,  Relations::OR},
            {Relations::A3AN,Relations::A3AN},
            {Relations::O3OR,Relations::O3OR},
        };
        for (int i = 0; i < sizeof(tts)/sizeof(*tts); ++i)
            m_propctors.addbatch
            (relations(maskpatterns[i][0], maskpatterns[i][1]), tts[i]);
        m_propctors.adddefs(definitions());
    }
// Mark propositional assertions. Return its number.
    Assertions::size_type markpropassertions()
    {
        Assertions::size_type count = 0;
        FOR (Assertions::reference r, m_assertions)
        {
            Assertion & ass = r.second;
            bool isprop = largestsymboldefnumber(ass,propctors(),Syntaxioms(),1);
            ass.type |= isprop * Asstype::PROPOSITIONAL;
            if (isprop && !ass.expression.empty() &&
                !typecodes().isprimitive(ass.expression[0]))
                ++count;
        }
        return count;
    }
// Return true if all propositional assertions are sound.
    bool checkpropassertion() const
    {
        return propctors().checkpropsat(assertions(), typecodes());
    }
};

#endif // DATABASE_H_INCLUDED
