#ifndef SECT_H_INCLUDED
#define SECT_H_INCLUDED

#include <cstddef>  // for std::size_t
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

// Section number
typedef std::vector<std::size_t> Sectionnumber;
// Section level count
typedef Sectionnumber::size_type Sectionlevel;

struct Comment;
// Data of a section
struct Section
{
    std::string title;
    // Pointer to comment
    Comment const * pcomment;
    // Position in tokens
    std::size_t tokenpos() const;
};

// Map: Section # -> position of begin of section in the token/assertion list
struct Sections : std::map<Sectionnumber, Section>
{
// Print the contents.
    void print(Sectionlevel const level = 0) const;
// Find the section whose title begins with str.
    const_iterator find(const char * str) const;
// Parse section header ($4.4.1/Headings).
    Sections(struct Comments const & comments);
};

std::ostream & operator<<(std::ostream & out, const Sectionnumber & sn);
std::ostream & operator<<(std::ostream & out, const Section & sect);

#endif // SECT_H_INCLUDED
