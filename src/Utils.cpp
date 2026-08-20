#include "Utils.hpp"
#include <cctype>
#include <climits>
#include <sstream>

std::string ircUpper(const std::string &value)
{
	std::string result(value);

	for (size_t i = 0; i < result.size(); ++i)
		result[i] = static_cast<char>(std::toupper(
			static_cast<unsigned char>(result[i])));
	return (result);
}

std::string ircLower(const std::string &value)
{
	std::string result(value);

	for (size_t i = 0; i < result.size(); ++i)
		result[i] = static_cast<char>(std::tolower(
			static_cast<unsigned char>(result[i])));
	return (result);
}

std::vector<std::string> splitComma(const std::string &value)
{
	std::vector<std::string> result;
	size_t start = 0;
	size_t comma;

	do
	{
		comma = value.find(',', start);
		result.push_back(value.substr(start, comma - start));
		start = comma + 1;
	}
	while (comma != std::string::npos);
	return (result);
}

std::string numberString(unsigned long value)
{
	std::ostringstream stream;

	stream << value;
	return (stream.str());
}

bool parsePositive(const std::string &value, size_t &result)
{
	unsigned long parsed = 0;

	if (value.empty())
		return (false);
	for (size_t i = 0; i < value.size(); ++i)
	{
		if (!std::isdigit(static_cast<unsigned char>(value[i])))
			return (false);
		parsed = parsed * 10 + static_cast<unsigned long>(value[i] - '0');
		if (parsed > INT_MAX)
			return (false);
	}
	result = static_cast<size_t>(parsed);
	return (result > 0);
}

bool validNickname(const std::string &nickname)
{
	const std::string firstSpecial = "[]\\`^{}_|";
	const std::string special = "-" + firstSpecial;

	if (nickname.empty() || nickname.size() > 30)
		return (false);
	if (!std::isalpha(static_cast<unsigned char>(nickname[0]))
		&& firstSpecial.find(nickname[0]) == std::string::npos)
		return (false);
	for (size_t i = 1; i < nickname.size(); ++i)
	{
		if (!std::isalnum(static_cast<unsigned char>(nickname[i]))
			&& special.find(nickname[i]) == std::string::npos)
			return (false);
	}
	return (true);
}

bool validChannelName(const std::string &name)
{
	if (name.size() < 2 || name.size() > 50)
		return (false);
	if (name[0] != '#' && name[0] != '&')
		return (false);
	return (name.find_first_of(" ,\a\r\n") == std::string::npos);
}
