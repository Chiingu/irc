#include "Server.hpp"
#include "Utils.hpp"

std::string Server::channelModes(Channel *channel) const
{
	std::string modes = "+";
	std::string arguments;

	if (channel->inviteOnly)
		modes += "i";
	if (channel->topicRestricted)
		modes += "t";
	if (!channel->key.empty())
	{
		modes += "k";
		arguments += " " + channel->key;
	}
	if (channel->userLimit > 0)
	{
		modes += "l";
		arguments += " " + numberString(channel->userLimit);
	}
	return (modes + arguments);
}

bool Server::modeNeedsArgument(char mode, bool adding) const
{
	return (mode == 'o' || (mode == 'k' && adding)
		|| (mode == 'l' && adding));
}

void Server::commandMode(Client *client, const Message &message)
{
	if (message.params.empty())
	{
		numeric(client, 461, "MODE :Not enough parameters");
		return ;
	}
	Channel *channel = findChannel(message.params[0]);
	if (channel == NULL)
	{
		if (message.params[0] == client->nickname)
			numeric(client, 221, "+");
		else
			numeric(client, 403, message.params[0] + " :No such channel");
	}
	else if (message.params.size() == 1)
		numeric(client, 324, channel->name + " " + channelModes(channel));
	else if (!channel->has(client))
		numeric(client, 442, channel->name + " :You're not on that channel");
	else if (!channel->isOperator(client))
		numeric(client, 482, channel->name + " :You're not channel operator");
	else
		applyModes(client, channel, message);
}

void Server::applyModes(Client *client, Channel *channel,
	const Message &message)
{
	bool adding = true;
	size_t argumentIndex = 2;
	std::string applied;
	std::string arguments;
	char lastSign = '\0';

	for (size_t i = 0; i < message.params[1].size(); ++i)
	{
		char mode = message.params[1][i];
		if (mode == '+' || mode == '-')
		{
			adding = (mode == '+');
			continue ;
		}
		if (std::string("itkol").find(mode) == std::string::npos)
		{
			numeric(client, 472, std::string(1, mode) + " :is unknown mode char to me");
			continue ;
		}
		std::string argument;
		if (modeNeedsArgument(mode, adding))
		{
			if (argumentIndex >= message.params.size())
			{
				numeric(client, 461, "MODE :Not enough parameters");
				continue ;
			}
			argument = message.params[argumentIndex++];
		}
		if (applyMode(client, channel, mode, adding, argument))
		{
			char sign = adding ? '+' : '-';
			if (lastSign != sign)
				applied += sign;
			lastSign = sign;
			applied += mode;
			if (!argument.empty())
				arguments += " " + argument;
		}
	}
	if (!applied.empty())
		broadcast(channel, client->prefix() + " MODE " + channel->name + " "
			+ applied + arguments, NULL);
}

bool Server::applyMode(Client *client, Channel *channel, char mode, bool adding,
	const std::string &argument)
{
	if (mode == 'i')
		channel->inviteOnly = adding;
	else if (mode == 't')
		channel->topicRestricted = adding;
	else if (mode == 'k')
		channel->key = adding ? argument : "";
	else if (mode == 'l')
	{
		size_t limit = 0;
		if (adding && !parsePositive(argument, limit))
			return (false);
		channel->userLimit = adding ? limit : 0;
	}
	else if (mode == 'o')
	{
		Client *target = findNickname(argument);
		if (target == NULL || !channel->has(target))
		{
			numeric(client, 441, argument + " " + channel->name
				+ " :They aren't on that channel");
			return (false);
		}
		if (adding)
			channel->operators.insert(target);
		else
			channel->operators.erase(target);
	}
	return (true);
}
