#include "Server.hpp"
#include "Utils.hpp"
#include <set>
#include <unistd.h>

bool Server::requireRegistered(Client *client)
{
	if (client->registered)
		return (true);
	numeric(client, 451, ":You have not registered");
	return (false);
}

void Server::commandCap(Client *client, const Message &message)
{
	if (message.params.empty())
		return ;
	std::string subcommand = ircUpper(message.params[0]);
	if (subcommand == "LS")
	{
		client->capNegotiating = true;
		queue(client, ":irc.42.local CAP * LS :multi-prefix");
	}
	else if (subcommand == "REQ")
	{
		std::string requested;
		if (message.params.size() > 1)
			requested = message.params[1];
		queue(client, ":irc.42.local CAP * ACK :" + requested);
	}
	else if (subcommand == "END")
	{
		client->capNegotiating = false;
		tryRegister(client);
	}
}

void Server::commandPass(Client *client, const Message &message)
{
	if (client->registered)
	{
		numeric(client, 462, ":You may not reregister");
		return ;
	}
	if (message.params.empty())
	{
		numeric(client, 461, "PASS :Not enough parameters");
		return ;
	}
	client->passwordAccepted = (message.params[0] == _password);
	if (!client->passwordAccepted)
		numeric(client, 464, ":Password incorrect");
	tryRegister(client);
}

void Server::commandNick(Client *client, const Message &message)
{
	if (message.params.empty())
	{
		numeric(client, 431, ":No nickname given");
		return ;
	}
	const std::string &nickname = message.params[0];
	Client *used = findNickname(nickname);
	if (!validNickname(nickname))
		numeric(client, 432, nickname + " :Erroneous nickname");
	else if (ircLower(nickname) == "42bot" || (used != NULL && used != client))
		numeric(client, 433, nickname + " :Nickname is already in use");
	else
	{
		if (client->registered && client->nickname != nickname)
			broadcastNick(client, client->prefix() + " NICK :" + nickname);
		client->nickname = nickname;
		client->hasNickname = true;
		tryRegister(client);
	}
}

void Server::commandUser(Client *client, const Message &message)
{
	if (client->registered || client->hasUsername)
	{
		numeric(client, 462, ":You may not reregister");
		return ;
	}
	if (message.params.size() < 4)
	{
		numeric(client, 461, "USER :Not enough parameters");
		return ;
	}
	client->username = message.params[0].substr(0, 32);
	client->realname = message.params[3].substr(0, 128);
	client->hasUsername = true;
	tryRegister(client);
}

void Server::tryRegister(Client *client)
{
	if (client->registered || client->capNegotiating)
		return ;
	if (!client->passwordAccepted || !client->hasNickname
		|| !client->hasUsername)
		return ;
	client->registered = true;
	welcome(client);
}

void Server::welcome(Client *client)
{
	numeric(client, 1, ":Welcome to the 42 IRC Network " + client->prefix());
	numeric(client, 2, ":Your host is irc.42.local, running ft_irc-1.0");
	numeric(client, 3, ":This server supports RFC-style IRC messaging");
	numeric(client, 4, "irc.42.local ft_irc-1.0 itkol itkol");
	numeric(client, 5, "CHANTYPES=#& PREFIX=(o)@ CHANMODES=,k,l,it "
		"NICKLEN=30 :are supported by this server");
	numeric(client, 251, ":There are " + numberString(_clients.size())
		+ " users and 1 service on 1 server");
	commandMotd(client);
}

void Server::broadcastNick(Client *client, const std::string &line)
{
	std::set<Client *> recipients;
	recipients.insert(client);
	for (ChannelMap::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		if (!it->second->has(client))
			continue ;
		recipients.insert(it->second->members.begin(), it->second->members.end());
	}
	for (std::set<Client *>::iterator it = recipients.begin();
		it != recipients.end(); ++it)
		queue(*it, line);
}

void Server::removeClient(Client *client, const std::string &reason)
{
	std::set<Client *> recipients;
	std::string quitLine = client->prefix() + " QUIT :" + reason;
	for (ChannelMap::iterator it = _channels.begin(); it != _channels.end(); ++it)
	{
		if (it->second->has(client))
			recipients.insert(it->second->members.begin(), it->second->members.end());
	}
	recipients.erase(client);
	for (std::set<Client *>::iterator it = recipients.begin();
		it != recipients.end(); ++it)
		queue(*it, quitLine);
	for (ChannelMap::iterator it = _channels.begin(); it != _channels.end(); )
	{
		Channel *channel = it++->second;
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
	_clients.erase(client->fd);
	close(client->fd);
	delete client;
}

void Server::cleanClosingClients(void)
{
	for (ClientMap::iterator it = _clients.begin(); it != _clients.end(); )
	{
		Client *client = it++->second;
		if (client->closing)
			removeClient(client, "Connection lost");
	}
}

bool Server::dispatch(Client *client, const Message &message)
{
	const std::string &command = message.command;
	if (command == "CAP")
		commandCap(client, message);
	else if (command == "PASS")
		commandPass(client, message);
	else if (command == "NICK")
		commandNick(client, message);
	else if (command == "USER")
		commandUser(client, message);
	else if (command == "PING")
		queue(client, ":irc.42.local PONG irc.42.local :"
			+ (message.params.empty() ? "irc.42.local" : message.params[0]));
	else if (command == "PONG")
		return (true);
	else if (command == "QUIT" || command == "ERROR")
	{
		std::string reason = message.params.empty()
			? "Client Quit" : message.params[0];
		removeClient(client, reason);
		return (false);
	}
	else if (!requireRegistered(client))
		return (true);
	else if (command == "JOIN")
		commandJoin(client, message);
	else if (command == "PART")
		commandPart(client, message);
	else if (command == "PRIVMSG" || command == "NOTICE")
		commandPrivmsg(client, message, command == "NOTICE");
	else if (command == "KICK")
		commandKick(client, message);
	else if (command == "INVITE")
		commandInvite(client, message);
	else if (command == "TOPIC")
		commandTopic(client, message);
	else if (command == "MODE")
		commandMode(client, message);
	else if (command == "NAMES")
		commandNames(client, message);
	else if (command == "LIST")
		commandList(client);
	else if (command == "WHO")
		commandWho(client, message);
	else if (command == "WHOIS")
		commandWhois(client, message);
	else if (command == "MOTD")
		commandMotd(client);
	else if (command == "LUSERS")
		numeric(client, 251, ":There are " + numberString(_clients.size())
			+ " users and 1 service on 1 server");
	else if (command == "VERSION")
		numeric(client, 351, "ft_irc-1.0 irc.42.local :C++98 poll server");
	else if (command == "AWAY")
		numeric(client, message.params.empty() ? 305 : 306,
			message.params.empty() ? ":You are no longer marked as away"
			: ":You have been marked as being away");
	else
		numeric(client, 421, command + " :Unknown command");
	return (true);
}
