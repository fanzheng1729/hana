#include <algorithm>// for std::copy
#include <iterator> // for std::ostream_iterator
#include <iostream>
#include "comment.h"
#include "sect.h"
#include "util/for.h"

std::size_t Section::tokenpos() const { return pcomment->tokenpos; }

std::ostream & operator<<(std::ostream & out, const Sectionnumber & sn)
{
    std::copy(sn.begin(), sn.end(), std::ostream_iterator<std::size_t>(out,"."));
    return out;
}

std::ostream & operator<<(std::ostream & out, const Section & sect)
{
    return out << "\t(" << sect.tokenpos() << ") " << sect.title << std::endl;
}

// Print the contents.
void Sections::print(Sectionlevel const level) const
{
    FOR (const_reference section, *this)
        if (level == 0 || section.first.size() <= level)
            std::cout << section.first << section.second;
}

// Find the section whose title begins with str.
Sections::const_iterator Sections::find(const char * str) const
{
    const_iterator iter(begin());
    for ( ; iter != end(); ++iter)
        if (strview(iter->second.title).remove_prefix(str))
            break;
    return iter;
}

// If mark is the marker of a section, return its level.
// Otherwise return 0 ($4.4.1/Headings).
static Sectionlevel sectionlevel(std::string const & mark)
{
    // Start of heading marker
    static const char * const marks[] = {"####", "#*#*", "=-=-", "-.-."};
    // Max # of levels of sections
    static const Sectionlevel maxlevel(sizeof(marks) / sizeof(const char *));
    // Level of heading marker
    Sectionlevel level;
    for (level = 0; level < maxlevel; ++level)
        if (strview(mark).remove_prefix(marks[level]))
            break;
    // ####: level = 0, return 1, ..., -.: level = 3, return 4
    // other: level = 4, return 0.
    return level < maxlevel ? level + 1 : 0;
}

static std::string trim(std::string const & str)
{
    std::string::size_type const begin(str.find_first_not_of(mmws));

    return begin == std::string::npos ? "" :
        str.substr(begin, str.find_last_not_of(mmws) - begin + 1);
}

// Add a new section to sections. Return its reference.
static Section & addsection(Sections & sections, Sectionlevel const level)
{
    // Fetch the section number from sections already added.
    Sectionnumber number;
    if (!sections.empty())
        number = std::max_element(sections.begin(), sections.end(),
                                  sections.value_comp())->first;
    // Advance section number to the next section at stated level.
    number.resize(level);
    ++number.back();
    return sections[number];
}

// Parse section header. Returns true iff there is a header ($4.4.1/Headings).
static bool parseheader(Comment const & comment, Sections & sections)
{
    std::string const & text(comment.text);
    // The first char should be \n.
    if (text.empty() || text[0] != '\n')
        return false;
    // Get marker.
    std::string::size_type end(text.find('\n', 1));
    std::string const mark(text.substr(1, end - 1));
    Sectionlevel const level(sectionlevel(mark));
    if (level == 0 || end >= text.size())
        return false;
    // Get title.
    std::string::size_type const begin(end + 1);
    end = text.find('\n', begin);
    std::string title(trim(text.substr(begin, end - begin)));
    // Check markers match.
    if (text.compare(end + 1, mark.size(), mark) != 0)
        return false;
    // Add new section.
    Section & section(addsection(sections, level));
    section.title = title;
    section.pcomment = &comment;
    return true;
}

// Parse section header ($4.4.1/Headings).
Sections::Sections(struct Comments const & comments)
{
    FOR (Comments::const_reference comment, comments)
        parseheader(comment, *this);
}
