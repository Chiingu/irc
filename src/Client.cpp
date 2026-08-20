#include "Client.hpp"

Client::Client(int socketFd, const std::string &address)
	: fd(socketFd), host(address), passwordAccepted(false),
	  hasNickname(false), hasUsername(false), registered(false),
	  capNegotiating(false), closing(false)
{
}

std::string Client::prefix(void) const
{
	return (":" + nickname + "!" + username + "@" + host);
}
