#include "ClientMessageHandler.hpp"
#include "Client.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "IRCReplies.hpp"
#include "Utils.hpp"
#include "config.hpp"
#include "Bot.hpp"

#include <iostream>
#include <sstream>
#include <cstdio>
#include <climits>

std::map<std::string, CommandHandler>	ClientMessageHandler::commandMap;

ClientMessageHandler::ModeContext::ModeContext() 
    : server(NULL), channel(NULL), client(NULL), tokens(NULL), paramIndex(0) {}

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

// ------------- PASS -----------//
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

// ------------- NICK -----------//
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

// ------------- USER -----------//
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

// ------------- PRIVMSG -----------//
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
			Channel 	*channel = it->second;

			const std::map<std::string, const Client*>& users = channel->getUsers();

			if (users.find(client.getNickname()) == users.end())
			{
				server.sendNumeric(&client, ERR_NOTONCHANNEL,
					channel->getName() + " :You're not on that channel");
				return ;
			}

			for (std::map<std::string, const Client*>::const_iterator i = users.begin();
				i != users.end(); ++i)
			{
				if (i->second != &client)
					server.sendPrivMsg(&client, tokens[1] ,i->second, tokens[2]);
			}

			// Send advice to Bot
			if (server.getBot())
			{
				const Client* botId = server.getBot()->getIdentityBot();
				if (users.find(botId->getNickname()) != users.end())
				{
					server.getBot()->onChannelMessage(it->second, &client, tokens[2]);
				}
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
			// If it's the Bot, make a response
            if (server.getBot() && it->second == server.getBot()->getIdentityBot())
			{
                server.getBot()->onDirectMessage(&client, tokens[2]);
            }
		}
		else
		{
			server.sendNumeric(
				&client, ERR_NOSUCHNICK, tokens[1] + " :No such nick");
		}
	}
}

// ------------- JOIN -----------//
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
			
			if (channel->getUserLimit() > -1
				&& channel->getUsers().size() >= static_cast<size_t>(channel->getUserLimit()))
			{
				server.sendNumeric(&client, ERR_CHANNELISFULL,
									channelsToJoin[i] + " :Cannot join channel (+l)");
				continue ;
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

			// Bot notification
			if (server.getBot())
			{
				server.getBot()->onUserJoinedChannel(channel, &client);
			}
		}
		else
		{
			server.sendNumeric(
				&client, ERR_NOSUCHCHANNEL, channelsToJoin[i] + " :No such channel");
		}
	}
}

// ------------- PART -----------//
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

// ------------- KICK -----------//
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

// ------------- INVITE -----------//
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
		Channel	*channel = it->second;
	
		const std::map<std::string, const Client*>& users = channel->getUsers();
		const std::map<std::string, const Client*>::const_iterator ui
			= users.find(client.getNickname());
		const std::set<const Client*> operators= channel->getOperators();
		
		if (ui == users.end())
		{
			server.sendNumeric(&client, ERR_NOTONCHANNEL,
				tokens[1] + " :You're not on that channel");
			return ;
		}
	
		if (channel->isInviteOnly() && operators.find(&client) == operators.end())
		{
			server.sendNumeric(&client, ERR_CHANOPRIVSNEEDED, client.getNickname()
								+ " " + tokens[2] + " :You're not channel operator");
			return ;
		}
		
		std::map<std::string, Client*> clientsByNick = server.getClientsByNick();
		std::map<std::string, Client*>::iterator ci = clientsByNick.find(tokens[1]);
		if (ci == clientsByNick.end())
		{
			server.sendNumeric(&client, ERR_NOSUCHNICK,
				tokens[1] + " :No such nick");
			return ;
		}

		if (users.find(tokens[1]) != users.end())
		{
			server.sendNumeric(&client, ERR_USERONCHANNEL,
				tokens[1] + " " + tokens[2] + " :Is already on channel");
			
			return ;
		}

		// If Bot is invited, make automatic join
		if (server.getBot() && ci->second == server.getBot()->getIdentityBot())
		{
			server.getBot()->join(tokens[2]);
			server.sendNumeric(&client, RPL_INVITING,
				client.getNickname() + " " + tokens[1] + " " + tokens[2]);
			return;
		}

		std::string inviteMsg = ":" + client.getNickname() + "!"
			+ client.getUsername() + "@" + client.getHostname() + " INVITE "
			+ tokens[1] + " " + tokens[2] + "\r\n";
			
		server.sendRaw(ci->second, inviteMsg);

		server.sendNumeric(&client, RPL_INVITING, client.getNickname() + " "
			+ tokens[1] + " " + tokens[2]);
	
		if (channel->isInviteOnly())
			channel->addInvited(ci->second);
	}
	else
	{
		server.sendNumeric(
			&client, ERR_NOSUCHCHANNEL, tokens[2] + " :No such channel");
	}
}

// ------------- TOPIC -----------//
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

// ------------- QUIT -----------//
void	ClientMessageHandler::handleQuit(
			Server &server, Client &client, const std::vector<std::string> &tokens)
{
	server.disconnectClient(&client, "Goodbye");
	(void)tokens;
}

// ------------- PING -----------//
void	ClientMessageHandler::handlePing(
			Server &server, Client &client, const std::vector<std::string> &tokens)
{
	server.sendRaw(&client, std::string("PONG :" + tokens[1]));
}

// ------------- MODE -----------//
void ClientMessageHandler::handleMode(
	Server &server, Client &client, const std::vector<std::string> &tokens)
{
	if (!client.isAuthenticated())
	{
		server.sendNumeric(&client, ERR_NOTREGISTERED, ":You have not registered");
		return ;
	}

	if (tokens.size() < 2)
	{
		server.sendNumeric(&client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
		return ;
	}

	if (tokens[1][0] != '#')
	{
		return ;
	}

	std::map<std::string, Channel*> channels = server.getChannels();
	std::map<std::string, Channel*>::iterator it = channels.find(tokens[1]);

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
	bool	topicBlocked	= channel->isTopicBlocked();
	bool	keyChannel		= !channel->getKey().empty();
	bool	userLimit		= channel->getUserLimit() > 0;

	// If channel exist ask for mode
	if (tokens.size() == 2)
	{
		// TODO separar en función
		std::string	mode = "+";
		std::string	params;
		if (inviteOnly) mode += "i";
		if (topicBlocked) mode += "t";
		if (keyChannel)
		{
			mode += "k";
			params += " " + channel->getKey();
		}
		if (userLimit)
		{
			mode += "l";
			params += " " + Utils::toString(channel->getUserLimit());
		}

		std::string msg = ":" + serverConfig::serverName + " 324 "
                  + client.getNickname() + " "
                  + tokens[1] + " "
                  + mode;
		if (!params.empty())
			msg += " " + params;
		msg += "\r\n";

		server.sendRaw(&client, msg);

		// -----------------------------

		return ;
	}

	// Check if the cliente is a channel operator
	if (channel->getOperators().find(&client) == channel->getOperators().end())
	{
		server.sendNumeric(&client, ERR_CHANOPRIVSNEEDED,
			tokens[1] + " :You're not channel operator");
		
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
	ClientMessageHandler::ModeContext modeCtx;
	modeCtx.server = &server;
	modeCtx.channel = channel;
	modeCtx.client = &client;
	modeCtx.tokens = &tokens;
	modeCtx.paramIndex = 3;
	
	for (size_t i = 0; i < execCmd.size(); ++i)
	{
		if (execCmd[i] == '-' || execCmd[i] == '+')
		{
			currentSign = execCmd[i];
			continue ;
		}
		changeMode(execCmd[i], currentSign, modeCtx);	
	}

	// TODO Print last message (only changes from original state)


}

void	ClientMessageHandler::changeMode(char mode, char symbol, ModeContext &modeCtx)
{
	switch (mode)
	{
		case 'i':
		{
			if (symbol == '+' && !modeCtx.channel->isInviteOnly())
			{
				modeCtx.channel->setInviteOnly(true);
				modeCtx.server->notifyModeChange(modeCtx.channel, modeCtx.client, "+i");
			}
			else if (symbol == '-' && modeCtx.channel->isInviteOnly())
			{
				modeCtx.channel->setInviteOnly(false);
				modeCtx.server->notifyModeChange(modeCtx.channel, modeCtx.client, "-i");
			}
			break;
		}

		case 'k':
		{
			if (modeCtx.tokens->size() <= modeCtx.paramIndex)
			{
				modeCtx.server->sendNumeric(modeCtx.client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
				return ;
			}

			if (symbol == '+' && modeCtx.channel->getKey().empty())
			{
				modeCtx.channel->setKey((*modeCtx.tokens)[modeCtx.paramIndex]);
				modeCtx.server->notifyModeChange(
					modeCtx.channel, modeCtx.client, "+k", (*modeCtx.tokens)[modeCtx.paramIndex]);
			}
			else if (symbol == '+')
			{
				modeCtx.server->sendNumeric(modeCtx.client, ERR_KEYSET,
					modeCtx.client->getNickname() + " " + modeCtx.channel->getName()
					+ " :Channel key already set");
			}
			else if (symbol == '-' && !modeCtx.channel->getKey().empty())
			{
				if (modeCtx.channel->getKey() == (*modeCtx.tokens)[modeCtx.paramIndex])
				{
					modeCtx.channel->setKey("");
					modeCtx.server->notifyModeChange(modeCtx.channel, modeCtx.client, "-k");
				}
			}
			++(modeCtx.paramIndex);
			
			break;
		}
		
		case 't':
		{
			if (symbol == '+' && !modeCtx.channel->isTopicBlocked())
			{
				modeCtx.channel->setTopicBlocked(true);
				modeCtx.server->notifyModeChange(modeCtx.channel, modeCtx.client, "+t");
			}
			else if (symbol == '-' && modeCtx.channel->isTopicBlocked())
			{
				modeCtx.channel->setTopicBlocked(false);
				modeCtx.server->notifyModeChange(modeCtx.channel, modeCtx.client, "-t");
			}
			break;
		}
		case 'o':
		{
			if ((*modeCtx.tokens).size() <= modeCtx.paramIndex)
			{
				modeCtx.server->sendNumeric(modeCtx.client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
				return ;
			}

			std::string	userName = (*modeCtx.tokens)[modeCtx.paramIndex];
			std::map<std::string, const Client*> users = modeCtx.channel->getUsers();
			std::map<std::string, const Client*>::iterator ui = users.find(userName);
			
			if (ui == users.end())
			{
				modeCtx.server->sendNumeric(modeCtx.client, ERR_USERNOTINCHANNEL, userName
						+ " " + modeCtx.channel->getName() + " :They aren't on that channel");
				++(modeCtx.paramIndex);

				return ;
			}

			const Client *user = ui->second;
			std::set<const Client*> currentOp		= modeCtx.channel->getOperators();
			std::set<const Client*>::iterator oi	= currentOp.find(user);
			
			if (symbol == '+' && oi == currentOp.end())
			{
				modeCtx.channel->addOperator(user);
				modeCtx.server->notifyModeChange(modeCtx.channel, modeCtx.client, "+o", userName);
			}
			else if (symbol == '-' && oi != currentOp.end())

			{
				modeCtx.channel->removeOperator(user);
				modeCtx.server->notifyModeChange(modeCtx.channel, modeCtx.client, "-o", userName);
			}

			++(modeCtx.paramIndex);

			break;
		}

		case 'l':
		{
			if ((*modeCtx.tokens).size() <= modeCtx.paramIndex)
			{
				modeCtx.server->sendNumeric(modeCtx.client, ERR_NEEDMOREPARAMS,
									"MODE :Not enough parameters");
				return ;
			}

			const std::string &extra = (*modeCtx.tokens)[modeCtx.paramIndex];

			if (symbol == '+')
			{
				int	newLimit = parseUserLimit((*modeCtx.tokens)[modeCtx.paramIndex]);
				if (newLimit != -1)
				{
					modeCtx.channel->setUserLimit(newLimit);
					modeCtx.server->notifyModeChange(modeCtx.channel, modeCtx.client,
												"+l", extra);
				
					++(modeCtx.paramIndex);
				}
			}
			else
			{
				modeCtx.channel->setUserLimit(-1);
				modeCtx.server->notifyModeChange(modeCtx.channel, modeCtx.client, "-l");
			}
	
			break;
		}

	}
}

int	ClientMessageHandler::parseUserLimit(const std::string &param)
{
	if (param.empty())
		return (-1);

	std::istringstream	iss(param);
	long				value;
	iss >> value;

	if (iss.fail() || !iss.eof())
		return (-1);

	if (value <= 0 || value > INT_MAX)
        return (-1);

	return (static_cast<int>(value));
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
