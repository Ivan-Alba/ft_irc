#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <string>

namespace Utils
{
	std::vector<std::string>	split(const std::string &str, char delimiter);
	std::vector<std::string>	splitBySpace(const std::string &input);
	std::string					trim(const std::string &str);
}

#endif
