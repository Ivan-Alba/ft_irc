#include "ClientMessageHandler.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "IRCReplies.hpp"
#include "Utils.hpp"
#include "config.hpp"

#include <iostream>
#include <sstream>
#include <cstdio>

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
	commandMap["PRIVMSG"]	= &ClientMessageHandler::handlePrivMsg;
	commandMap["JOIN"]		= &ClientMessageHandler::handleJoin;
	commandMap["PART"]		= &ClientMessageHandler::handlePart;
	commandMap["QUIT"]		= &ClientMessageHandler::handleQuit;
	commandMap["KICK"]		= &ClientMessageHandler::handleKick;
	commandMap["INVITE"]	= &ClientMessageHandler::handleInvite;
	commandMap["TOPIC"]		= &ClientMessageHandler::handleTopic;
	commandMap["MODE"]		= &ClientMessageHandler::handleMode;
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
		else if (tokens[0] != "CAP" && tokens[0] != "WHO")
		{
			server.sendNumeric(
				&client, ERR_UNKNOWNCOMMAND, tokens[0] + " :Unknown command");
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
		server.sendNumeric(&client, ERR_NEEDMOREPARAMS, "PASS :Not enough parameters");
	}

	if (tokens[1] == server.getPassword())
	{
		client.setPasswordAccepted(true);
		server.authenticateClient(&client);
	}
	else
	{
		server.sendNumeric(&client, ERR_PASSWDMISMATCH, "PASS :Wrong password");
		server.disconnectClient(&client, "Wrong password");
	}
}

void	ClientMessageHandler::handleNick(
			Server &server, Client &client, const std::vector<std::string> &tokens)
{
	if (!client.getNickname().empty())
		return ;

	if (tokens.size() == 1)
	{
		server.sendNumeric(&client, ERR_NONICKNAMEGIVEN, ":No nickname given");
	}
	else if (tokens[1].at(0) == '#')
	{
		server.sendNumeric(
			&client, ERR_ERRONEUSNICKNAME,  tokens[1] + " :Erroneus nickname");
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
		server.sendNumeric(&client, ERR_NEEDMOREPARAMS, "USER :Not enough parameters");
	}
	else
	{
		client.setUsername(tokens[1]);

		if (tokens.size() >= 3)
			client.setHostname(tokens[2]);
		else
			client.setHostname("*");
		server.authenticateClient(&client);
	}
}

void	ClientMessageHandler::handlePrivMsg(
			Server &server, Client &client, const std::vector<std::string> &tokens)
{
	if (!client.isAuthenticated())
	{
		server.sendNumeric(&client, ERR_NOTREGISTERED, ":You have not registered");
		return ;
	}

	if (tokens.size() != 3)
	{
		server.sendNumeric(
			&client, ERR_NEEDMOREPARAMS, "PRIVMSG :Not enough parameters");
		return;
	}
	
	if (tokens[1][0] == '#')
	{
		std::map<std::string, Channel*> channels = server.getChannels();
		std::map<std::string, Channel*>::iterator it = channels.find(tokens[1]);

		if (it != channels.end())
		{
			std::map<std::string, const Client*> users = (it->second)->getUsers();
			for (std::map<std::string, const Client*>::iterator i = users.begin();
				i != users.end(); ++i)
			{
				if (i->second != &client)
					server.sendPrivMsg(&client, tokens[1] ,i->second, tokens[2]);
			}
		}
		else
		{
			server.sendNumeric(
				&client, ERR_NOSUCHCHANNEL, tokens[1] + " :No such channel");
		}
	}
	else
	{
		std::map<std::string, Client*> clients = server.getClientsByNick();
		std::map<std::string, Client*>::iterator it = clients.find(tokens[1]);

		if (it != clients.end())
		{
			if (it->second != &client)
				server.sendPrivMsg(&client, it->second->getNickname() ,it->second, tokens[2]);
		}
		else
		{
			server.sendNumeric(
				&client, ERR_NOSUCHNICK, tokens[1] + " :No such nick");
		}
	}
}

void	ClientMessageHandler::handleJoin(
			Server &server, Client &client, const std::vector<std::string> &tokens)
{
	if (!client.isAuthenticated())
	{
		server.sendNumeric(&client, ERR_NOTREGISTERED, ":You have not registered");
		return ;
	}

	if (tokens.size() < 2)
	{
		server.sendNumeric(&client, ERR_NEEDMOREPARAMS, "JOIN :Not enough parameters");
		return;
	}
	
	//Split tokens[1] for multiple channels on param
	std::vector<std::string>	channelsToJoin;
	std::vector<std::string>	keys;

	channelsToJoin = Utils::split(tokens[1], ',');
	if (tokens.size() > 2)
		keys = Utils::split(tokens[2], ',');

	std::map<std::string, Channel*> channels = server.getChannels();
	
	for (size_t i = 0; i < channelsToJoin.size(); ++i)
	{
		std::map<std::string, Channel*>::iterator it = channels.find(channelsToJoin[i]);

		if (it != channels.end())
		{
			Channel 	*channel = it->second;
			std::string userList;

			const std::map<std::string, const Client*>& users 	= channel->getUsers();
			const std::set<const Client*>& operators 			= channel->getOperators();
			const std::set<const Client*>& invited 				= channel->getInvited();

			if (users.find(client.getNickname()) != users.end())
				continue ;

			if (channel->isInviteOnly() && invited.find(&client) == invited.end())
			{
				server.sendNumeric(&client, ERR_INVITEONLYCHAN,
									channelsToJoin[i] + " :Cannot join channel (+i)");
				continue ;
			}

			if (!channel->getKey().empty())
			{
				if (keys.size() < i + 1 || keys[i] != channel->getKey())
				{
					server.sendNumeric(
						&client, ERR_BADCHANNELKEY, tokens[1] + " :Password required");
					continue ;
				}
			}

			for (std::map<std::string, const Client*>::const_iterator ui = users.begin();
				ui != users.end(); ++ui)
			{
				if (!userList.empty())
					userList += " ";
				if (operators.find(ui->second) != operators.end())
					userList += "@";
				userList += ui->second->getNickname();
			}

			server.sendNameReply(&client, channelsToJoin[i], userList);
			server.sendEndOfNames(&client, channelsToJoin[i]);

			channel->addUser(&client);

			std::string joinMsg = ":" + client.getNickname() + "!"
				+ client.getUsername() + "@" + client.getHostname() + " JOIN "
				+ channelsToJoin[i] + "\r\n";

			const std::map<std::string, const Client*>& newUsers = channel->getUsers();
			for (std::map<std::string, const Client*>::const_iterator ui
				= newUsers.begin(); ui != newUsers.end(); ++ui)
			{
				server.sendRaw(ui->second, joinMsg);
			}
		}
		else
		{
			server.sendNumeric(
				&client, ERR_NOSUCHCHANNEL, channelsToJoin[i] + " :No such channel");
		}
	}
}

void	ClientMessageHandler::handlePart(
			Server &server, Client &client, const std::vector<std::string> &tokens)
{
	if (!client.isAuthenticated())
	{
		server.sendNumeric(&client, ERR_NOTREGISTERED, ":You have not registered");
		return ;
	}

	if (tokens.size() < 2)
	{
		server.sendNumeric(&client, ERR_NEEDMOREPARAMS, "PART :Not enough parameters");
		return;
	}
	
	std::vector<std::string>		channelsToLeave = Utils::split(tokens[1], ',');
	std::map<std::string, Channel*>	channels = server.getChannels();
	
	for (size_t i = 0; i < channelsToLeave.size(); ++i)
	{
		std::map<std::string, Channel*>::iterator it = channels.find(channelsToLeave[i]);

		if (it != channels.end())
		{
			Channel 	*channel = it->second;

			const std::map<std::string, const Client*>& users = channel->getUsers();

			if (users.find(client.getNickname()) == users.end())
			{
				server.sendNumeric(&client, ERR_NOTONCHANNEL,
					channelsToLeave[i] + " :You're not on that channel");
				continue ;
			}

			std::string	msg("");
			if (tokens.size() >= 3)
				msg = " :" + tokens[2];

			std::string leaveMsg = ":" + client.getNickname() + "!"
				+ client.getUsername() + "@" + client.getHostname() + " PART "
				+ channelsToLeave[i] + msg + "\r\n";
			
			for (std::map<std::string, const Client*>::const_iterator ui
				= users.begin(); ui != users.end(); ++ui)
			{
				server.sendRaw(ui->second, leaveMsg);
			}
			channel->removeUser(client.getNickname());
			channel->removeOperator(&client);
		}
		else
		{
			server.sendNumeric(
				&client, ERR_NOSUCHCHANNEL, channelsToLeave[i] + " :No such channel");
		}
	}
}

void	ClientMessageHandler::handleKick(
			Server &server, Client &client, const std::vector<std::string> &tokens)
{
	if (!client.isAuthenticated())
	{
		server.sendNumeric(&client, ERR_NOTREGISTERED, ":You have not registered");
		return ;
	}

	if (tokens.size() < 3)
	{
		server.sendNumeric(&client, ERR_NEEDMOREPARAMS, "PART :Not enough parameters");
		return ;
	}
	
	std::map<std::string, Channel*>	channels = server.getChannels();
	
	std::map<std::string, Channel*>::iterator it = channels.find(tokens[1]);

	if (it != channels.end())
	{
		Channel 						*channel = it->second;
		const std::set<const Client*>	operators = channel->getOperators();

		if (operators.find(&client) == operators.end())
		{
			server.sendNumeric(&client, ERR_CHANOPRIVSNEEDED, client.getNickname()
								+ " " + tokens[1] + " :You're not channel operator");
			return ;
		}

		const std::map<std::string, const Client*>& users = channel->getUsers();
		if (users.find(tokens[2]) == users.end())
		{
			server.sendNumeric(&client, ERR_USERNOTINCHANNEL, tokens[2] + " "
								+ tokens[1] + " :They aren't on that channel");
			return ;
		}

		std::string	msg("");
		if (tokens.size() >= 4)
			msg = " :" + tokens[3];

		std::string kickMsg = ":" + client.getNickname() + "!"
			+ client.getUsername() + "@" + client.getHostname() + " KICK "
			+ tokens[1] + " " + tokens[2] + msg + "\r\n";
			
		for (std::map<std::string, const Client*>::const_iterator ui
			= users.begin(); ui != users.end(); ++ui)
		{
			server.sendRaw(ui->second, kickMsg);
		}
		channel->removeUser(tokens[2]);
		channel->removeOperator(users.find(tokens[2])->second);
	}
	else
	{
		server.sendNumeric(
			&client, ERR_NOSUCHCHANNEL, tokens[1] + " :No such channel");
	}
}

void	ClientMessageHandler::handleInvite(
			Server &server, Client &client, const std::vector<std::string> &tokens)
{
	if (!client.isAuthenticated())
	{
		server.sendNumeric(&client, ERR_NOTREGISTERED, ":You have not registered");
		return ;
	}

	if (tokens.size() < 3)
	{
		server.sendNumeric(&client, ERR_NEEDMOREPARAMS, "PART :Not enough parameters");
		return ;
	}
	
	std::map<std::string, Channel*>	channels = server.getChannels();
	
	std::map<std::string, Channel*>::iterator it = channels.find(tokens[2]);

	if (it != channels.end())
	{
		Channel 						*channel = it->second;
		const std::set<const Client*>	operators = channel->getOperators();

		if (channel->isInviteOnly() && operators.find(&client) == operators.end())
		{
			server.sendNumeric(&client, ERR_CHANOPRIVSNEEDED, client.getNickname()
								+ " " + tokens[2] + " :You're not channel operator");
			return ;
		}

		const std::map<std::string, const Client*>& users = channel->getUsers();
		std::map<std::string, const Client*>::const_iterator ui = users.find(tokens[1]);

		if (ui == users.end())
		{
			server.sendNumeric(&client, ERR_USERNOTINCHANNEL, tokens[1] + " "
								+ tokens[2] + " :They aren't on that channel");
			return ;
		}

		std::string inviteMsg = ":" + client.getNickname() + "!"
			+ client.getUsername() + "@" + client.getHostname() + " INVITE "
			+ tokens[1] + " " + tokens[2] + "\r\n";
			
		server.sendRaw(ui->second, inviteMsg);
	
		if (channel->isInviteOnly())
			channel->addInvited(ui->second);
	}
	else
	{
		server.sendNumeric(
			&client, ERR_NOSUCHCHANNEL, tokens[2] + " :No such channel");
	}
}

void	ClientMessageHandler::handleTopic(
			Server &server, Client &client, const std::vector<std::string> &tokens)
{
	if (!client.isAuthenticated())
	{
		server.sendNumeric(&client, ERR_NOTREGISTERED, ":You have not registered");
		return ;
	}

	if (tokens.size() < 2)
	{
		server.sendNumeric(&client, ERR_NEEDMOREPARAMS, "PART :Not enough parameters");
		return ;
	}
	
	std::map<std::string, Channel*>	channels = server.getChannels();	
	std::map<std::string, Channel*>::iterator it = channels.find(tokens[1]);

	if (it != channels.end())
	{
		Channel *channel									= it->second;
		const std::set<const Client*> operators				= channel->getOperators();
		const std::map<std::string, const Client*>& users	= channel->getUsers();

		std::map<std::string, const Client*>::const_iterator ui 
			= users.find(client.getNickname());

		if (ui == users.end())
		{
			server.sendNumeric(&client, ERR_NOTONCHANNEL,
				tokens[1] + " :You're not on that channel");
			return ;
		}

		if (tokens.size() > 2 && channel->isTopicBlocked()
			&& operators.find(&client) == operators.end())
		{
			server.sendNumeric(&client, ERR_CHANOPRIVSNEEDED, client.getNickname()
								+ " " + tokens[1] + " :You're not channel operator");
			return ;
		}

		if (tokens.size() == 2)
		{
			//Mostrar topic
			if (!channel->getTopic().empty())
			{
				server.sendNumeric(&client, RPL_TOPIC, tokens[1]
					+ " :" + channel->getTopic());
			}
			else
				server.sendNumeric(&client, RPL_NOTOPIC, tokens[1] + " :No topic is set");
		}
		else
		{
			channel->setTopic(tokens[2]);

			std::string topicMsg = ":" + client.getNickname() + "!"
				+ client.getUsername() + "@" + client.getHostname() + " TOPIC "
				+ tokens[1] + " :" + tokens[2] + "\r\n";
			
			for (ui = users.begin(); ui != users.end(); ++ui)
			{
				server.sendRaw(ui->second, topicMsg);
			}
		}
	}
	else
	{
		server.sendNumeric(
			&client, ERR_NOSUCHCHANNEL, tokens[1] + " :No such channel");
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

void ClientMessageHandler::handleMode(
	Server &server, Client &client, const std::vector<std::string> &tokens)
{
	// Check if client is authenticated
	if (!client.isAuthenticated())
	{
		server.sendNumeric(&client, ERR_NOTREGISTERED, ":You have not registered");
		return ;
	}

	// Check minimum parameters
	if (tokens.size() < 2)
	{
		server.sendNumeric(&client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
		return ;
	}

	// Check if target is channel
	if (tokens[1][0] != '#') // Handle Channel Modes
	{
		return ;
	}

	std::map<std::string, Channel*> channels = server.getChannels();
	std::map<std::string, Channel*>::iterator it = channels.find(target);

	if (it == channels.end())
	{
		server.sendNumeric(&client, ERR_NOSUCHCHANNEL, tokens[1] + " :No such channel");
		return ;
	}

	Channel *channel									= it->second;
	const std::set<const Client*> operators				= channel->getOperators();
	const std::map<std::string, const Client*>& users	= channel->getUsers();

	std::map<std::string, const Client*>::const_iterator ui 
		= users.find(client.getNickname());

	if (ui == users.end())
	{
		server.sendNumeric(&client, ERR_NOTONCHANNEL,
			tokens[1] + " :You're not on that channel");
			return ;
	}

	//Current channel modes
	bool	inviteOnly		= channel->isInviteOnly();
	bool	topicBlocked	= isTopicBlocked();
	bool	keyChannel		= !channel->getKey().empty();
	bool	userLimit		= channel->getUserLimit() > 0;

	// If channel exist ask for mode
	if (tokens.size() == 2)
	{
		std::string mode = "+";
		if (inviteOnly) mode += "i";
		if (topicBlocked) mode += "t";
		if (keyChannel) mode += "k";
		if (userLimit) mode += "l";
		
		server.sendNumeric(&client, RPL_CHANNELMODEIS, tokens[1] + " " + mode);
		
		return ;
	}

	// Check if the cliente is a channel operator
	if (channel-> getOperators().find(&client) == channel->getOperators().end())
	{
		server.sendNumeric(&client, ERR_CHANOPRIVSNEEDED,
			target + " :You're not channel operator");
		
		return ;
	}

	//Generate a valid chain "+-+it--k+ol-"
	std::string execCmd;
	std::string flagsAccepted = "itkol";
	char currentSign = 0;
	for (size_t i = 0; i < tokens[2].size(); ++i)
	{
		char c = tokens[2][i];
		if (c == '+' || c == '-')
		{
			currentSign = c;
			execCmd += c;
			continue ;
		}

		if (currentSign == 0)
			continue;

		if (flagsAccepted.find(tokens[2][i]) != std::string::npos)
		{
			execCmd += c;
		}
	}

	//Execute cmd chain
	modeContext.server = server;
	modeContext.channel = *channel;
	modeContext.client = client
	modeContext.tokens = tokens;
	modeContext.paramIndex = 3;
	
	for (size_t i = 0; i < exeCmd.size(), ++i)
	{
		if (execCmd[i] == '-' || execCmd == '+')
		{
			currentSing = execCmd[i];
			continue ;
		}
		changeMode(execCmd[i], currentSign, modeContext);	
	}

	//Print last message (only changes from original state)

}

void	ClientMessageHandler::changeMode(char mode, char symbol, ModeContext &modeCtx)
{
	switch (mode)
	{
		case 'i':
		{
			if (symbol == '+' && !modeCtx.channel.isInviteOnly())
			{
				modeCtx.channel.setInviteOnly(true);
				modeCtx.server.notifyModeChange(&modeCtx.channel, &modeCtx.client, "+i");
			}
			else if (symbol == '-' && modeCtx.channel.isInviteOnly())
			{
				modeCtx.channel.setInviteOnly(false);
				modeCtx.server.notifyModeChange(&modeCtx.channel, &modeCtx.client, "-i");
			}
			break;
		}

		case 'k':
		{
			if (tokens.size() <= modeCtx.paramIndex)
			{
				server.sendNumeric(&client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
				return ;
			}

			if (symbol == '+' && modeCtx.channel.getKey().empty())
			{
				modeCtx.channel.setKey(tokens[modeCtx.paramIndex]);
				modeCtx.server.notifyModeChange(&modeCtx.channel, &modeCtx.client, "+k");
			}
			else if (symbol == '+')
			{
				server.sendNumeric(&client, ERR_KEYSET, " :Channel key already set");
			}
			else if (symbol == '-' && !modeCtx.channel.getKey().empty())
			{
				if (modeCtx.channel.getKey() == tokens[modeCtx.paramIndex])
				{
					modeCtx.channel.setKey("");
					modeCtx.server.notifyModeChange(&modeCtx.channel, &modeCtx.client, "-k");
				}
			}
			++(modeCtx.paramIndex);
			
			break;
		}
		
		case 't':
		{
			if (symbol == '+' && !modeCtx.channel.isTopicBlocked())
			{
				modeCtx.channel.setTopicBlocked(true);
				modeCtx.server.notifyModeChange(&modeCtx.channel, &modeCtx.client, "+t");
			}
			else if (symbol == '-' && modeCtx.channel.isTopicBlocked())
			{
				modeCtx.channel.setTopicBlocked(false);
				modeCtx.server.notifyModeChange(&modeCtx.channel, &modeCtx.client, "-t");
			}
			break;
		}
		case 'o':
		{
			if (tokens.size() <= modeCtx.paramIndex)
			{
				server.sendNumeric(&client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
				return ;
			}

			// TODO

			break;
		}

		case 'l':
		{
			if (tokens.size() <= modeCtx.paramIndex)
			{
				server.sendNumeric(&client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
				return ;
			}

			std::string	&extra = modeCtx.tokens[modeCtx.paramIndex++];

			if (symbol == '+')
			{
				// TODO falta casteo de str a int
				modeCtx.channel.setUserLimit(tokens[modeCtx.paramIndex]);
				modeCtx.server.notifyModeChange(&modeCtx.channel, &modeCtx.client, "+l");
			}
			else
			{
				// TODO falta gestionar límite en JOIN
				modeCtx.channel.setUSerLimit(-1);
				modeCtx.server.notifyModeChange(&modeCtx.channel, &modeCtx.client, "-l");
			}

			
			break;
		}

	}
}

// Testing tokenizer, printing tokens
void	ClientMessageHandler::printTokens(const std::vector<std::string> &tokens)
{
	for (size_t i = 0; i < tokens.size(); ++i)
	{
		std::cout << "Token " << i << ": '" << tokens[i] << "'" << std::endl;
	}
}

std::vector<std::string>	ClientMessageHandler::tokenize(std::string &line)
{
	std::vector<std::string> tokens;

	size_t	pos = line.find(":");

	if (pos != std::string::npos)
	{
		std::string			left = line.substr(0, pos);
		std::string			right = line.substr(pos + 1);
		
		tokens = Utils::splitBySpace(left);

		tokens.push_back(Utils::trim(right));
	}
	else
	{
		tokens = Utils::splitBySpace(line);	
	}

	//TODO DEBUG
	printTokens(tokens);

	return (tokens);
}
