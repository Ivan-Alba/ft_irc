#include "Utils.hpp"
#include <sstream>

namespace Utils
{
	std::vector<std::string>	split(const std::string &str, char delimiter)
	{
		std::vector<std::string> tokens;
		std::istringstream ss(str);
		std::string token;

		while (std::getline(ss, token, delimiter))
		{
			tokens.push_back(token);
		}

		return (tokens);
	}

	std::vector<std::string>	splitBySpace(const std::string &input)
    {
		std::istringstream			ss(input);
		std::vector<std::string>	tokens;
		std::string					token;

		while (ss >> token) // usa std::isspace como delimitador
			tokens.push_back(token);

		return (tokens);
    }

	std::string trim(const std::string &str)
	{
		if (str.empty())
			return ("");

		size_t	start = 0;
        size_t	end = str.size() - 1;

		while (start <= end && std::isspace(static_cast<unsigned char>(str[start])))
			start++;

		while (end >= start && std::isspace(static_cast<unsigned char>(str[end])))
			end--;

		return (str.substr(start, end - start + 1));
    }
}
