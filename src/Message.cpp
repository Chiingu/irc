#include "Message.hpp"
#include "Utils.hpp"

Message parseMessage(const std::string &line)
{
	Message message;
	size_t position = 0;
	size_t end;

	if (!line.empty() && line[0] == ':')
	{
		position = line.find(' ');
		if (position == std::string::npos)
			return (message);
		++position;
	}
	while (position < line.size() && line[position] == ' ')
		++position;
	end = line.find(' ', position);
	message.command = ircUpper(line.substr(position, end - position));
	position = end;
	while (position != std::string::npos && position < line.size())
	{
		while (position < line.size() && line[position] == ' ')
			++position;
		if (position >= line.size())
			break ;
		if (line[position] == ':')
		{
			message.params.push_back(line.substr(position + 1));
			break ;
		}
		end = line.find(' ', position);
		message.params.push_back(line.substr(position, end - position));
		position = end;
	}
	return (message);
}
