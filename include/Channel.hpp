#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <set>
# include <string>
# include <cstddef>

class Client;

class Channel
{
public:
	Channel(const std::string &channelName);
	bool has(Client *client) const;
	bool isOperator(Client *client) const;

	std::string name;
	std::string topic;
	std::string key;
	std::set<Client *> members;
	std::set<Client *> operators;
	bool inviteOnly;
	bool topicRestricted;
	size_t userLimit;
};

#endif
