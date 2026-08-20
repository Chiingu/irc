#ifndef SERVER_HPP
# define SERVER_HPP

# include "Channel.hpp"
# include "Client.hpp"
# include "Message.hpp"
# include <map>
# include <poll.h>
# include <set>
# include <string>
# include <vector>

class Server
{
public:
	Server(unsigned short port, const std::string &password);
	~Server(void);
	void run(void);

private:
	typedef std::map<int, Client *> ClientMap;
	typedef std::map<std::string, Channel *> ChannelMap;

	unsigned short _port;
	std::string _password;
	int _listener;
	ClientMap _clients;
	ChannelMap _channels;
	std::vector<struct pollfd> _pollfds;

	void createListener(void);
	void buildPollList(void);
	void acceptClient(void);
	void readClient(int fd);
	void writeClient(int fd);
	void removeClient(Client *client, const std::string &reason);
	void cleanClosingClients(void);
	void processInput(Client *client);
	bool dispatch(Client *client, const Message &message);

	void queue(Client *client, const std::string &line);
	void numeric(Client *client, int code, const std::string &text);
	void broadcast(Channel *channel, const std::string &line,
		Client *excluded);
	void broadcastNick(Client *client, const std::string &line);
	Client *findNickname(const std::string &nickname) const;
	Channel *findChannel(const std::string &name) const;
	bool requireRegistered(Client *client);

	void commandCap(Client *client, const Message &message);
	void commandPass(Client *client, const Message &message);
	void commandNick(Client *client, const Message &message);
	void commandUser(Client *client, const Message &message);
	void tryRegister(Client *client);
	void welcome(Client *client);

	void commandJoin(Client *client, const Message &message);
	void joinOne(Client *client, const std::string &name,
		const std::string &key);
	void commandPart(Client *client, const Message &message);
	void partOne(Client *client, const std::string &name,
		const std::string &reason);
	void commandKick(Client *client, const Message &message);
	void commandInvite(Client *client, const Message &message);
	void commandTopic(Client *client, const Message &message);
	void commandMode(Client *client, const Message &message);
	void applyModes(Client *client, Channel *channel,
		const Message &message);
	bool applyMode(Client *client, Channel *channel, char mode, bool adding,
		const std::string &argument);
	bool modeNeedsArgument(char mode, bool adding) const;
	std::string channelModes(Channel *channel) const;

	void commandPrivmsg(Client *client, const Message &message, bool notice);
	void messageTarget(Client *client, const std::string &target,
		const std::string &text, bool notice);
	void commandNames(Client *client, const Message &message);
	void sendNames(Client *client, Channel *channel);
	void commandList(Client *client);
	void commandWho(Client *client, const Message &message);
	void commandWhois(Client *client, const Message &message);
	void commandMotd(Client *client);
	void commandBot(Client *client, const std::string &target,
		const std::string &text);

	Server(const Server &other);
	Server &operator=(const Server &other);
};

#endif
