#include "Server.hpp"
#include "Utils.hpp"

void Server::commandPrivmsg(Client *client, const Message &message, bool notice)
{
	if (message.params.empty())
	{
		if (!notice)
			numeric(client, 411, ":No recipient given (PRIVMSG)");
		return ;
	}
	if (message.params.size() < 2 || message.params[1].empty())
	{
		if (!notice)
			numeric(client, 412, ":No text to send");
		return ;
	}
	std::vector<std::string> targets = splitComma(message.params[0]);
	for (size_t i = 0; i < targets.size(); ++i)
		messageTarget(client, targets[i], message.params[1], notice);
}

void Server::messageTarget(Client *client, const std::string &target,
	const std::string &text, bool notice)
{
	std::string verb = notice ? " NOTICE " : " PRIVMSG ";
	Channel *channel = findChannel(target);
	if (channel != NULL)
	{
		if (!channel->has(client))
		{
			if (!notice)
				numeric(client, 404, target + " :Cannot send to channel");
			return ;
		}
		broadcast(channel, client->prefix() + verb + channel->name + " :" + text,
			client);
		if (!notice && !text.empty() && text[0] == '!')
			commandBot(client, channel->name, text);
		return ;
	}
	if (ircLower(target) == "42bot" && !notice)
	{
		commandBot(client, client->nickname, text);
		return ;
	}
	Client *recipient = findNickname(target);
	if (recipient != NULL)
		queue(recipient, client->prefix() + verb + recipient->nickname + " :" + text);
	else if (!notice)
		numeric(client, 401, target + " :No such nick/channel");
}
