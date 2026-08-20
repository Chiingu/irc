#include "Server.hpp"
#include "Utils.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <iomanip>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <unistd.h>

void Server::acceptClient(void)
{
	struct sockaddr_in address;
	socklen_t size = sizeof(address);
	int fd = accept(_listener,
		reinterpret_cast<struct sockaddr *>(&address), &size);

	if (fd < 0)
		return ;
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
	{
		close(fd);
		return ;
	}
	std::string host(inet_ntoa(address.sin_addr));
	Client *client = NULL;
	try
	{
		client = new Client(fd, host);
		_clients[fd] = client;
	}
	catch (...)
	{
		_clients.erase(fd);
		delete client;
		close(fd);
	}
}

void Server::readClient(int fd)
{
	char buffer[4096];
	ssize_t count = recv(fd, buffer, sizeof(buffer), 0);
	ClientMap::iterator found = _clients.find(fd);

	if (found == _clients.end())
		return ;
	if (count == 0)
	{
		removeClient(found->second, "Connection closed");
		return ;
	}
	if (count < 0)
		return ;
	found->second->input.append(buffer, static_cast<size_t>(count));
	if (found->second->input.size() > 65536)
		found->second->closing = true;
	else
		processInput(found->second);
}

void Server::writeClient(int fd)
{
	ClientMap::iterator found = _clients.find(fd);
	ssize_t count;

	if (found == _clients.end() || found->second->output.empty())
		return ;
	count = send(fd, found->second->output.c_str(),
		found->second->output.size(), 0);
	if (count > 0)
		found->second->output.erase(0, static_cast<size_t>(count));
}

void Server::queue(Client *client, const std::string &line)
{
	static const size_t maximumQueue = 16 * 1024 * 1024;

	if (client == NULL || client->closing)
		return ;
	if (client->output.size() + line.size() + 2 > maximumQueue)
	{
		client->closing = true;
		return ;
	}
	client->output += line + "\r\n";
}

void Server::numeric(Client *client, int code, const std::string &text)
{
	std::ostringstream line;

	line << ":irc.42.local " << std::setw(3) << std::setfill('0') << code
		<< " " << (client->hasNickname ? client->nickname : "*")
		<< " " << text;
	queue(client, line.str());
}

void Server::processInput(Client *client)
{
	size_t newline;

	while ((newline = client->input.find('\n')) != std::string::npos)
	{
		std::string line = client->input.substr(0, newline);
		client->input.erase(0, newline + 1);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty())
			continue ;
		if (line.size() > 510)
		{
			numeric(client, 417, ":Input line was too long");
			continue ;
		}
		Message message = parseMessage(line);
		if (!message.command.empty() && !dispatch(client, message))
			return ;
	}
}

void Server::broadcast(Channel *channel, const std::string &line,
	Client *excluded)
{
	for (std::set<Client *>::iterator it = channel->members.begin();
		it != channel->members.end(); ++it)
	{
		if (*it != excluded)
			queue(*it, line);
	}
}

Client *Server::findNickname(const std::string &nickname) const
{
	std::string wanted = ircLower(nickname);

	for (ClientMap::const_iterator it = _clients.begin();
		it != _clients.end(); ++it)
	{
		if (it->second->hasNickname
			&& ircLower(it->second->nickname) == wanted)
			return (it->second);
	}
	return (NULL);
}

Channel *Server::findChannel(const std::string &name) const
{
	ChannelMap::const_iterator found = _channels.find(ircLower(name));

	if (found == _channels.end())
		return (NULL);
	return (found->second);
}
