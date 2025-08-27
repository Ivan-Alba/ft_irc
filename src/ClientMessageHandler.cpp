#include "ClientMessageHandler.hpp"

std::map<std::string, CommandHandler>	ClientMessageHandler::commandMap;

void	ClientMessageHandler::handleMessage(Server &server, Client &client)
{
	std::string	&buffer;
	size_t		pos;

	buffer = client.getBuffer();

	while ((pos = buffer.find("\r\n")) != std::string::npos)
	{
		std::string line = buffer.substr(0, pos);
		buffer.erase(0, pos + 2);

		if (line.empty())
			continue;

		std::vector<std::string> tokens = tokenize(line);
		
		processCommands(server, client, tokens);
	}
}

void	ClientMessageHandler::initCommandMap()
{
	commandMap["PASS"]		= &ClientMessageHandler::handlePass;
	commandMap["NICK"]		= &ClientMessageHandler::handleNick;
	commandMap["USER"]		= &ClientMessageHandler::handleUser;
	commandMap["PRIVMSG"]	= &ClientMessageHandler::handlePrivMsg;
	commandMap["JOIN"]		= &ClientMessageHandler::handleJoin;
	commandMap["PART"]		= &ClientMessageHandler::handlePart;
	commandMap["QUIT"]		= &ClientMessageHandler::handleQuit;
	commandMap["KICK"]		= &ClientMessageHandler::handleKick;
	commandMap["INVITE"]	= &ClientMessageHandler::handleInvite;
	commandMap["TOPIC"]		= &ClientMessageHandler::handleTopic;
	commandMap["MODE"]		= &ClientMessageHandler::handleMode;
}

void	ClientMessageHandler::processCommand(Server &server, Client &client,
			const std::vector<std::string>> &tokens)
{
	if (commandMap.empty())
	{
		initCommandMap();
	}

	if (!tokens.empty())
	{
		std::map<std::string, CommandHandler>::iterator it;
		it = commandMap.find(tokens[0]);

		if (it != commandMap.end())
		{
			it->second(server, client, tokens);
		}
		else
		{
			server.sendError(&client, "Unknown command: " + tokens[0]);
		}
	}
}

// TODO ALL COMMAND FUNCTIONS
void	ClientMessageHandler::handlePass(
			Server &server, Client &client, const std::vector<std::string> &token)
{
	if (client->isPasswordAccepted())
		return ;

	if (token.size() == 1)
	{
		// SEND ERROR 461
	}

	if (token[1] == server.getPassword())
	{
		client->setPasswordAccepted(true);
		client->checkAuthenticated();
	}
	else
	{
		// SEND ERROR 464
		server.disconnectClient(client, "Wrong password");
	}
}



// TODO tonekize()
std::vector<std::string>	ClientMessageHandler::tokenize(std::string &line)
{

}
