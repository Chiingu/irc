#include "Server.hpp"
#include "Utils.hpp"

void Server::commandJoin(Client *client, const Message &message)
{
	if (message.params.empty())
	{
		numeric(client, 461, "JOIN :Not enough parameters");
		return ;
	}
	if (message.params[0] == "0")
	{
		std::vector<std::string> joined;
		for (ChannelMap::iterator it = _channels.begin();
			it != _channels.end(); ++it)
		{
			if (it->second->has(client))
				joined.push_back(it->second->name);
		}
		for (size_t i = 0; i < joined.size(); ++i)
			partOne(client, joined[i], "Leaving all channels");
		return ;
	}
	std::vector<std::string> channels = splitComma(message.params[0]);
	std::vector<std::string> keys;
	if (message.params.size() > 1)
		keys = splitComma(message.params[1]);
	for (size_t i = 0; i < channels.size(); ++i)
		joinOne(client, channels[i], i < keys.size() ? keys[i] : "");
}

void Server::joinOne(Client *client, const std::string &name,
	const std::string &providedKey)
{
	if (!validChannelName(name))
	{
		numeric(client, 403, name + " :No such channel");
		return ;
	}
	Channel *channel = findChannel(name);
	bool created = (channel == NULL);
	if (created)
		channel = new Channel(name);
	if (!created && channel->has(client))
		return ;
	std::string channelKey = ircLower(name);
	if (!created && channel->inviteOnly
		&& client->invitations.find(channelKey) == client->invitations.end())
		numeric(client, 473, name + " :Cannot join channel (+i)");
	else if (!created && !channel->key.empty() && channel->key != providedKey)
		numeric(client, 475, name + " :Cannot join channel (+k)");
	else if (!created && channel->userLimit > 0
		&& channel->members.size() >= channel->userLimit)
		numeric(client, 471, name + " :Cannot join channel (+l)");
	else
	{
		if (created)
			_channels[channelKey] = channel;
		channel->members.insert(client);
		if (created)
			channel->operators.insert(client);
		client->invitations.erase(channelKey);
		broadcast(channel, client->prefix() + " JOIN :" + channel->name, NULL);
		if (channel->topic.empty())
			numeric(client, 331, channel->name + " :No topic is set");
		else
			numeric(client, 332, channel->name + " :" + channel->topic);
		sendNames(client, channel);
	}
	if (created && channel->members.empty())
		delete channel;
}

void Server::commandPart(Client *client, const Message &message)
{
	if (message.params.empty())
	{
		numeric(client, 461, "PART :Not enough parameters");
		return ;
	}
	std::string reason = message.params.size() > 1
		? message.params[1] : client->nickname;
	std::vector<std::string> names = splitComma(message.params[0]);
	for (size_t i = 0; i < names.size(); ++i)
		partOne(client, names[i], reason);
}

void Server::partOne(Client *client, const std::string &name,
	const std::string &reason)
{
	Channel *channel = findChannel(name);
	if (channel == NULL)
	{
		numeric(client, 403, name + " :No such channel");
		return ;
	}
	if (!channel->has(client))
	{
		numeric(client, 442, channel->name + " :You're not on that channel");
		return ;
	}
	broadcast(channel, client->prefix() + " PART " + channel->name
		+ " :" + reason, NULL);
	channel->members.erase(client);
	channel->operators.erase(client);
	if (channel->members.empty())
	{
		_channels.erase(ircLower(channel->name));
		delete channel;
	}
	else if (channel->operators.empty())
	{
		Client *promoted = *channel->members.begin();
		channel->operators.insert(promoted);
		broadcast(channel, ":irc.42.local MODE " + channel->name
			+ " +o " + promoted->nickname, NULL);
	}
}

void Server::commandKick(Client *client, const Message &message)
{
	if (message.params.size() < 2)
	{
		numeric(client, 461, "KICK :Not enough parameters");
		return ;
	}
	Channel *channel = findChannel(message.params[0]);
	Client *target = findNickname(message.params[1]);
	if (channel == NULL)
		numeric(client, 403, message.params[0] + " :No such channel");
	else if (!channel->has(client))
		numeric(client, 442, channel->name + " :You're not on that channel");
	else if (!channel->isOperator(client))
		numeric(client, 482, channel->name + " :You're not channel operator");
	else if (target == NULL || !channel->has(target))
		numeric(client, 441, message.params[1] + " " + channel->name
			+ " :They aren't on that channel");
	else
	{
		std::string reason = message.params.size() > 2
			? message.params[2] : client->nickname;
		broadcast(channel, client->prefix() + " KICK " + channel->name + " "
			+ target->nickname + " :" + reason, NULL);
		channel->members.erase(target);
		channel->operators.erase(target);
		if (channel->operators.empty() && !channel->members.empty())
			channel->operators.insert(*channel->members.begin());
		if (channel->members.empty())
		{
			_channels.erase(ircLower(channel->name));
			delete channel;
		}
	}
}

void Server::commandInvite(Client *client, const Message &message)
{
	if (message.params.size() < 2)
	{
		numeric(client, 461, "INVITE :Not enough parameters");
		return ;
	}
	Client *target = findNickname(message.params[0]);
	Channel *channel = findChannel(message.params[1]);
	if (target == NULL)
		numeric(client, 401, message.params[0] + " :No such nick/channel");
	else if (channel == NULL)
		numeric(client, 403, message.params[1] + " :No such channel");
	else if (!channel->has(client))
		numeric(client, 442, channel->name + " :You're not on that channel");
	else if (!channel->isOperator(client))
		numeric(client, 482, channel->name + " :You're not channel operator");
	else if (channel->has(target))
		numeric(client, 443, target->nickname + " " + channel->name
			+ " :is already on channel");
	else
	{
		target->invitations.insert(ircLower(channel->name));
		numeric(client, 341, target->nickname + " " + channel->name);
		queue(target, client->prefix() + " INVITE " + target->nickname
			+ " :" + channel->name);
	}
}

void Server::commandTopic(Client *client, const Message &message)
{
	if (message.params.empty())
	{
		numeric(client, 461, "TOPIC :Not enough parameters");
		return ;
	}
	Channel *channel = findChannel(message.params[0]);
	if (channel == NULL)
		numeric(client, 403, message.params[0] + " :No such channel");
	else if (!channel->has(client))
		numeric(client, 442, channel->name + " :You're not on that channel");
	else if (message.params.size() == 1)
		numeric(client, channel->topic.empty() ? 331 : 332, channel->name
			+ (channel->topic.empty() ? " :No topic is set" : " :" + channel->topic));
	else if (channel->topicRestricted && !channel->isOperator(client))
		numeric(client, 482, channel->name + " :You're not channel operator");
	else
	{
		channel->topic = message.params[1];
		broadcast(channel, client->prefix() + " TOPIC " + channel->name
			+ " :" + channel->topic, NULL);
	}
}
