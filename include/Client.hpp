#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <set>
# include <string>

class Client
{
public:
	Client(int socketFd, const std::string &address);
	std::string prefix(void) const;

	int fd;
	std::string host;
	std::string nickname;
	std::string username;
	std::string realname;
	std::string input;
	std::string output;
	std::set<std::string> invitations;
	bool passwordAccepted;
	bool hasNickname;
	bool hasUsername;
	bool registered;
	bool capNegotiating;
	bool closing;
};

#endif
