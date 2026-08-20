#include "Server.hpp"
#include "Utils.hpp"

static std::string botAnswer(const std::string &text)
{
	std::string command = ircLower(text);

	if (command == "help" || command == "!help")
		return ("Commands: !help, !ping, !about, !rules, !dice");
	if (command == "ping" || command == "!ping")
		return ("PONG - the server event loop is responsive.");
	if (command == "about" || command == "!about")
		return ("I am 42bot, the built-in ft_irc service bot.");
	if (command == "rules" || command == "!rules")
		return ("Be excellent to each other. Keep channels useful and respectful.");
	if (command == "dice" || command == "!dice")
		return ("You rolled " + numberString((text.size() * 7) % 6 + 1) + ".");
	return ("Unknown bot command. Send !help for the command list.");
}

void Server::commandBot(Client *client, const std::string &target,
	const std::string &text)
{
	std::string response = ":42bot!bot@irc.42.local PRIVMSG " + target
		+ " :" + botAnswer(text);
	Channel *channel = findChannel(target);

	if (channel != NULL)
		broadcast(channel, response, NULL);
	else
		queue(client, response);
}
