#include "Server.hpp"
#include "Utils.hpp"

void Server::sendNames(Client *client, Channel *channel)
{
	std::string names;

	for (std::set<Client *>::iterator it = channel->members.begin();
		it != channel->members.end(); ++it)
	{
		std::string item = (channel->isOperator(*it) ? "@" : "")
			+ (*it)->nickname;
		if (names.size() + item.size() > 350)
		{
			numeric(client, 353, "= " + channel->name + " :" + names);
			names.clear();
		}
		if (!names.empty())
			names += " ";
		names += item;
	}
	numeric(client, 353, "= " + channel->name + " :" + names);
	numeric(client, 366, channel->name + " :End of /NAMES list");
}

void Server::commandNames(Client *client, const Message &message)
{
	if (message.params.empty())
	{
		for (ChannelMap::iterator it = _channels.begin();
			it != _channels.end(); ++it)
			sendNames(client, it->second);
		return ;
	}
	std::vector<std::string> names = splitComma(message.params[0]);
	for (size_t i = 0; i < names.size(); ++i)
	{
		Channel *channel = findChannel(names[i]);
		if (channel != NULL)
			sendNames(client, channel);
		else
			numeric(client, 366, names[i] + " :End of /NAMES list");
	}
}

void Server::commandList(Client *client)
{
	numeric(client, 321, "Channel :Users Name");
	for (ChannelMap::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		Channel *channel = it->second;
		numeric(client, 322, channel->name + " "
			+ numberString(channel->members.size()) + " :" + channel->topic);
	}
	numeric(client, 323, ":End of /LIST");
}

void Server::commandWho(Client *client, const Message &message)
{
	std::string mask = message.params.empty() ? "*" : message.params[0];
	Channel *channel = findChannel(mask);

	if (channel != NULL)
	{
		for (std::set<Client *>::iterator it = channel->members.begin();
			it != channel->members.end(); ++it)
		{
			Client *user = *it;
			numeric(client, 352, channel->name + " " + user->username + " "
				+ user->host + " irc.42.local " + user->nickname + " H"
				+ (channel->isOperator(user) ? "@" : "") + " :0 " + user->realname);
		}
	}
	numeric(client, 315, mask + " :End of /WHO list");
}

void Server::commandWhois(Client *client, const Message &message)
{
	if (message.params.empty())
	{
		numeric(client, 431, ":No nickname given");
		return ;
	}
	if (ircLower(message.params[0]) == "42bot")
	{
		numeric(client, 311, "42bot bot irc.42.local * :ft_irc service bot");
		numeric(client, 312, "42bot irc.42.local :42 IRC Server");
		numeric(client, 318, "42bot :End of /WHOIS list");
		return ;
	}
	Client *target = findNickname(message.params[0]);
	if (target == NULL)
	{
		numeric(client, 401, message.params[0] + " :No such nick/channel");
		return ;
	}
	numeric(client, 311, target->nickname + " " + target->username + " "
		+ target->host + " * :" + target->realname);
	numeric(client, 312, target->nickname + " irc.42.local :42 IRC Server");
	std::string joined;
	for (ChannelMap::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		if (it->second->has(target))
			joined += " " + std::string(it->second->isOperator(target) ? "@" : "")
				+ it->second->name;
	}
	if (!joined.empty())
		numeric(client, 319, target->nickname + " :" + joined.substr(1));
	numeric(client, 318, target->nickname + " :End of /WHOIS list");
}

void Server::commandMotd(Client *client)
{
	numeric(client, 375, ":- irc.42.local Message of the day -");
	numeric(client, 372, ":- Welcome to ft_irc. Ask 42bot for help.");
	numeric(client, 376, ":End of /MOTD command");
}
