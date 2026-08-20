#include "Server.hpp"
#include "Utils.hpp"
#include <exception>
#include <iostream>

static bool readPort(const char *value, unsigned short &port)
{
	size_t parsed;
	std::string text(value);

	if (!parsePositive(text, parsed) || parsed > 65535)
		return (false);
	port = static_cast<unsigned short>(parsed);
	return (true);
}

int main(int argc, char **argv)
{
	unsigned short port;

	if (argc != 3 || !readPort(argv[1], port) || argv[2][0] == '\0')
	{
		std::cerr << "Usage: ./ircserv <port> <password>" << std::endl;
		return (1);
	}
	try
	{
		Server server(port, argv[2]);
		server.run();
	}
	catch (const std::exception &error)
	{
		std::cerr << "ircserv: " << error.what() << std::endl;
		return (1);
	}
	return (0);
}
