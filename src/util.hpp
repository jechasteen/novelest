#include <regex>

static int word_count(std::string str)
{
    std::regex rx("(\\w+)(;|,)*");

    return std::distance(
        std::sregex_iterator(str.begin(), str.end(), rx),
        std::sregex_iterator());
}