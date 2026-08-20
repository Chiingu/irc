#include "Server.hpp"
#include "Utils.hpp"
#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

static volatile sig_atomic_t g_serverRunning = 1;
static volatile sig_atomic_t g_listener = -1;

static void stopServer(int signalNumber)
{
	(void)signalNumber;
	g_serverRunning = 0;
	if (g_listener >= 0)
	{
		int listener = static_cast<int>(g_listener);
		g_listener = -1;
		close(listener);
	}
}

Server::Server(unsigned short port, const std::string &password)
	: _port(port), _password(password), _listener(-1)
{
	createListener();
	g_listener = _listener;
	std::signal(SIGINT, stopServer);
	std::signal(SIGTERM, stopServer);
	std::signal(SIGPIPE, SIG_IGN);
}

Server::~Server(void)
{
	for (ClientMap::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		close(it->first);
		delete it->second;
	}
	for (ChannelMap::iterator it = _channels.begin(); it != _channels.end(); ++it)
		delete it->second;
	if (_listener >= 0)
		close(_listener);
	g_listener = -1;
}

void Server::createListener(void)
{
	struct sockaddr_in address;
	int enabled = 1;

	_listener = socket(AF_INET, SOCK_STREAM, 0);
	if (_listener < 0)
		throw (std::runtime_error("cannot create listening socket"));
	setsockopt(_listener, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));
	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(_port);
	if (fcntl(_listener, F_SETFL, O_NONBLOCK) < 0
		|| bind(_listener, reinterpret_cast<struct sockaddr *>(&address),
			sizeof(address)) < 0 || listen(_listener, 128) < 0)
	{
		close(_listener);
		_listener = -1;
		throw (std::runtime_error("cannot bind or listen on requested port"));
	}
}

void Server::buildPollList(void)
{
	struct pollfd descriptor;

	_pollfds.clear();
	descriptor.fd = _listener;
	descriptor.events = POLLIN;
	descriptor.revents = 0;
	_pollfds.push_back(descriptor);
	for (ClientMap::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		descriptor.fd = it->first;
		descriptor.events = POLLIN;
		if (!it->second->output.empty())
			descriptor.events |= POLLOUT;
		descriptor.revents = 0;
		_pollfds.push_back(descriptor);
	}
}

void Server::run(void)
{
	while (g_serverRunning)
	{
		cleanClosingClients();
		buildPollList();
		int ready = poll(&_pollfds[0], _pollfds.size(), -1);
		if (ready <= 0)
			continue ;
		for (size_t i = 0; i < _pollfds.size(); ++i)
		{
			if (_pollfds[i].fd == _listener && (_pollfds[i].revents & POLLIN))
				acceptClient();
			else if (_clients.find(_pollfds[i].fd) != _clients.end())
			{
				if (_pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
					_clients[_pollfds[i].fd]->closing = true;
				else if (_pollfds[i].revents & POLLIN)
					readClient(_pollfds[i].fd);
				if (_clients.find(_pollfds[i].fd) != _clients.end()
					&& (_pollfds[i].revents & POLLOUT))
					writeClient(_pollfds[i].fd);
			}
		}
	}
}
