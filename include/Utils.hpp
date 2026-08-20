#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>
# include <vector>

std::string ircUpper(const std::string &value);
std::string ircLower(const std::string &value);
std::vector<std::string> splitComma(const std::string &value);
std::string numberString(unsigned long value);
bool parsePositive(const std::string &value, size_t &result);
bool validNickname(const std::string &nickname);
bool validChannelName(const std::string &name);

#endif
