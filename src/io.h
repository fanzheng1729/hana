#ifndef IO_H_INCLUDED
#define IO_H_INCLUDED

#include <algorithm>// for std::copy
#include <cctype>   // for std::tolower
#include <cstddef>  // for std::size_t
#include <iostream>
#include <iterator> // for std::ostream_iterator
#include <map>
#include <set>
#include <string>
#include <vector>

struct strview;
std::ostream & operator<<(std::ostream & out, strview str);

template<class T>
std::ostream & operator<<(std::ostream & out, const std::vector<T> & v)
{
    std::copy(v.begin(), v.end(), std::ostream_iterator<T>(out, " "));
    return out << std::endl;
}

inline std::ostream & operator<<
    (std::ostream & out, const std::vector<bool> & v)
{
    std::copy(v.begin(), v.end(), std::ostream_iterator<bool>(out));
    return out << std::endl;
}

struct CNFClauses;
std::ostream & operator<<(std::ostream & out, const CNFClauses & cnf);

struct Proofstep;
typedef std::vector<Proofstep> Proofsteps;
std::ostream & operator<<(std::ostream & out, Proofsteps const & steps);

template<class Key, class T>
std::ostream & operator<<(std::ostream & out, const std::map<Key, T> & map)
{
    for (typename std::map<Key, T>::const_iterator
         iter(map.begin()); iter != map.end(); ++iter)
        out << iter->first << ' ';
    return out << std::endl;
}
template<class Key>
std::ostream & operator<<(std::ostream & out, const std::set<Key> & set)
{
    std::copy(set.begin(), set.end(), std::ostream_iterator<Key>(out, " "));
    return out << std::endl;
}

template
<class SyntaxDAG, class Ranks = typename SyntaxDAG::RanksDAG>
std::ostream & operator<<(std::ostream & out, const SyntaxDAG & syntaxDAG)
{
    typedef typename SyntaxDAG::RankDAGiter It;

    Ranks const & dag = syntaxDAG.ranksDAG();
    for (It iter1 = dag.begin(); iter1 != dag.end(); ++iter1)
    {
        out << *iter1 << " -> ";
        for (It iter2 = dag.begin(); iter2 != dag.end(); ++iter2)
            if (dag.reachable(iter1, iter2))
                out << *iter2 << ' ';
        out << std::endl;
    }
    return out;
}

// Return true if n <= lim. Otherwise print a message and return false.
bool is1stle2nd(std::size_t const n, std::size_t const lim,
                const char * const s1, const char * const s2);

template<class T>
bool unexpected(bool const condition, const char * const type, const T & value)
{
    if (condition)
        std::cerr << "Unexpected " << type << ": " << value << std::endl;

    return condition;
}

// Print the number and the label of an assertion.
void printass(std::size_t number, strview label, std::size_t count = 0);
template<class Assref>
void printass(Assref const & assref, std::size_t count = 0)
{
    printass(assref.second.number, assref.first, count);
}

// Print a percentage
template<class NUM, class DENOM>
void printpercent
    (NUM num, const char * slash, DENOM denom, const char * equals,
     const char * ending = NULL)
{
    std::cout << num << slash << denom << equals;
    if (ending)
        std::cout << static_cast<double>(num)*100/denom << ending;
}

// Ask a question and get its answer.
inline std::string ask(const char * question)
{
    std::string answer;
    std::cout   << question;
    std::cin    >> answer;
    return answer;
}

// Ask a y/n question and get its answer.
inline bool askyn(const char * question)
{
    std::string const & answer(ask(question));
    if (answer.empty()) return false;
    unsigned char const c = answer[0];
    return std::tolower(c) == 'y';
}

#endif // IO_H_INCLUDED
