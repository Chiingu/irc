#include "Channel.hpp"
#include "Client.hpp"

Channel::Channel(const std::string &channelName)
	: name(channelName), inviteOnly(false), topicRestricted(true), userLimit(0)
{
}

bool Channel::has(Client *client) const
{
	return (members.find(client) != members.end());
}

bool Channel::isOperator(Client *client) const
{
	return (operators.find(client) != operators.end());
}
