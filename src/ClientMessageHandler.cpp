#include "ClientMessageHandler.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "config.hpp"

#include <iostream>
#include <sstream>

std::map<std::string, CommandHandler>	ClientMessageHandler::commandMap;

void	ClientMessageHandler::handleMessage(Server &server, Client &client)
{
	std::string	&buffer = client.getBuffer();
	size_t		pos;

	while ((pos = buffer.find("\r\n")) != std::string::npos)
	{
		std::string line = buffer.substr(0, pos);
		buffer.erase(0, pos + 2);

		if (line.empty())
			continue;

		std::vector<std::string> tokens = tokenize(line);	
		processCommand(server, client, tokens);
	}
}

void	ClientMessageHandler::initCommandMap()
{
	commandMap["PASS"]		= &ClientMessageHandler::handlePass;
	commandMap["NICK"]		= &ClientMessageHandler::handleNick;
	commandMap["USER"]		= &ClientMessageHandler::handleUser;
	//commandMap["PRIVMSG"]	= &ClientMessageHandler::handlePrivMsg;
	//commandMap["JOIN"]		= &ClientMessageHandler::handleJoin;
	//commandMap["PART"]		= &ClientMessageHandler::handlePart;
	commandMap["QUIT"]		= &ClientMessageHandler::handleQuit;
	//commandMap["KICK"]		= &ClientMessageHandler::handleKick;
	//commandMap["INVITE"]	= &ClientMessageHandler::handleInvite;
	//commandMap["TOPIC"]		= &ClientMessageHandler::handleTopic;
	//commandMap["MODE"]		= &ClientMessageHandler::handleMode;
	commandMap["PING"]		= &ClientMessageHandler::handlePing;
}

void	ClientMessageHandler::processCommand(Server &server, Client &client,
			const std::vector<std::string> &tokens)
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
		else if (tokens[0] == "CAP")
		{
			server.sendError(&client, "Unknown command: " + tokens[0]);
		}
	}
}

// TODO ALL COMMAND FUNCTIONS
void	ClientMessageHandler::handlePass(
			Server &server, Client &client, const std::vector<std::string> &tokens)
{
	if (client.isPasswordAccepted())
		return ;

	if (tokens.size() == 1)
	{
		server.sendNumeric(&client, 461, "PASS :Not enough parameters");
	}

	if (tokens[1] == server.getPassword())
	{
		client.setPasswordAccepted(true);
		server.authenticateClient(&client);
	}
	else
	{
		server.sendNumeric(&client, 464, "PASS :Wrong password");
		server.disconnectClient(&client, "Wrong password");
	}
}

void	ClientMessageHandler::handleQuit(
			Server &server, Client &client, const std::vector<std::string> &tokens)
{
	server.disconnectClient(&client, "Goodbye");
	(void)tokens;
}

void	ClientMessageHandler::handlePing(
			Server &server, Client &client, const std::vector<std::string> &tokens)
{
	server.sendRaw(&client, std::string("PONG :" + tokens[1]));
}

void	ClientMessageHandler::handleNick(
			Server &server, Client &client, const std::vector<std::string> &tokens)
{
	if (!client.getNickname().empty())
		return ;

	if (tokens.size() == 1)
	{
		server.sendNumeric(&client, 431, "NICK :No nickname given");
	}
	else
	{
		std::map<std::string, Client*>	clientsByNick;
		
		clientsByNick = server.getClientsByNick();
		for (std::map<std::string, Client*>::iterator it
				= clientsByNick.begin();
				it != clientsByNick.end(); ++it)
		{
			if (it->second->getNickname() == client.getNickname())
			{
				server.sendError(&client, "Nickname already in use");
				return ;
			}
		}

		client.setNickname(tokens[1]);
		server.authenticateClient(&client);
	}
}

void	ClientMessageHandler::handleUser(
			Server &server, Client &client, const std::vector<std::string> &tokens)
{
	if (!client.getUsername().empty())
		return ;

	if (tokens.size() == 1)
	{
		server.sendNumeric(&client, 461, "USER :Not enough parameters");
	}
	else
	{
		client.setUsername(tokens[1]);
		server.authenticateClient(&client);
	}
}

// TODO tonekize()

// Testing tokenizer, printing tokens
void	ClientMessageHandler::printTokens(const std::vector<std::string> &tokens)
{
	for (size_t i=0; i < tokens.size(); ++i)
	{
		std::cout << "Token " << i << ": '" << tokens[i] << "'" << std::endl;
	}
}

// Trim to delete spaces after and before msg
std::string	ClientMessageHandler::trim(const std::string &str)
{
	std::string	trimmed = "";

	size_t start = str.find_first_not_of(" \t\r\n");

	if (start == std::string::npos)
		return (trimmed);

	size_t end = str.find_last_not_of(" \t\r\n");

	trimmed = str.substr(start, end - start + 1);

	return (trimmed);
}

std::vector<std::string>	ClientMessageHandler::tokenize(std::string &line)
{
	std::vector<std::string> tokens;

	// Looking for :
	size_t	pos = line.find(":");

	// If : exist, all the content after is a token including spaces (trailing paramaters)
	if (pos != std::string::npos)
	{
    	// Left part from : must be separated by spaces
		std::string			left = line.substr(0, pos);
		std::istringstream	iss(left);
		std::string			token;

		while (iss >> token)
		{
			tokens.push_back(token);
		}

		//Right part from : , but without begining spaces
		std::string trailing_params = line.substr(pos + 1);
		// if there is spaces in the (Ex: PRIVMSG #canal :    hello world  ) delete all the spaces with a trim function
		tokens.push_back(trim(trailing_params));
	}
	else
	{
		// If there is not trailing parameters
		std::istringstream iss(line);
		std::string word;
		while (iss >> word)
		{
			tokens.push_back(word);
		}
	}

	//TODO DEBUG
	printTokens(tokens);

	return (tokens);
}
