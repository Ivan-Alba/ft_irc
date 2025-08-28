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
		else if (tokens[0] != "CAP")
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
	std::string target = tokens[1];
	if (target[0] == '#') // Handle Channel Modes
	{
		std::map<std::string, Channel*> channels = server.getChannels();

		std::map<std::string, Channel*>::iterator it = channels.find(target);

		if (it == channels.end())
		{
			server.sendNumeric(&client, ERR_NOSUCHCHANNEL, target + " :No such channel");
			return ;
		}

		Channel * channel = it->second;

		// If channel exist ask for mode
		if (tokens.size() == 2)
		{
			std::string mode = "+";
			if (channel->isInviteOnly()) mode += "i";
			if (channel->isTopicBlocked()) mode += "t";
			if (!channel->getKey().empty()) mode += "k";
			if (channel->getUserLimit() > 0) mode += "l";
			server.sendNumeric(&client, RPL_CHANNELMODEIS, target + " " + mode);
			return ;
		}

		// Check if the cliente is a channel operator
		if (channel-> getOperators().find(&client) == channel->getOperators().end())
		{
			server.sendNumeric(&client, ERR_CHANOPRIVSNEEDED, target + " :You're not channel operator");
			return ;
		}

		// Mode changes
		std::string mode_flag = tokens[2];
		size_t param_index = 3; // Index for optional parameters (k, o, l)

		for (size_t i=0; i < mode_flag.length(); ++i)
		{
			char flag = mode_flag[i];
			bool sign = flag == '+';

			if (flag == '+' || flag == '-')
				continue ;
            
			switch (flag)
			{
				case 'i':
					channel->setInviteOnly(sign);
					//channel->clearInvited(); // This line is for -i, it has to clear the invited list
					break ;
				case 't':
					channel->setTopicBlocked(sign);
					break ;
				case 'k':
				{   
					if (sign)
					{
						if (param_index < tokens.size())
							channel->setKey(tokens[param_index]);
						else
							server.sendNumeric(&client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters for +k");
					}
					else
						channel->setKey("");
					param_index++;
					break ;
				}
				case 'o':
				{
					if (param_index < tokens.size())
						{
						std::string targetNick = tokens[param_index];
						std::map<std::string, Client*> clients = server.getClientsByNick();
						Client* targetClient = NULL;
						std::map<std::string, Client*>::iterator client_it = clients.find(targetNick);

						if (client_it != clients.end())
							targetClient = client_it->second;
                        
						if (targetClient && channel->getUsers().find(targetNick) != channel->getUsers().end())
						{
							if (sign)
								channel->addOperator(targetClient);
							else
								channel->removeOperator(targetClient);
						}
						else
							server.sendNumeric(&client, ERR_USERNOTINCHANNEL, targetNick + " " + target + " :They aren't on that channel");
					}
					else
						server.sendNumeric(&client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters for +o");
					param_index++;
					break ;
				}
				case 'l':
				{
					if (sign)
					{
						if (param_index < tokens.size())
						{
							int limit;
							if (sscanf(tokens[param_index].c_str(), "%d", &limit) == 1)
							{
								if (limit > 0)
									channel->setUserLimit(limit);
							}
							else
							{
								server.sendNumeric(&client, ERR_NEEDMOREPARAMS, "MODE :Invalid limit parameter");
							}
						}
						else
							server.sendNumeric(&client, ERR_NEEDMOREPARAMS, "MODE :Not enough parameters");
					}
					else
						channel->setUserLimit(0); // 0 means no limits
					param_index++;
					break ;
				}
				default:
					server.sendNumeric(&client, ERR_UNKNOWNMODE, std::string(1, flag) + " :is unknown mode char to me for " + target);
					break ;
			}
		}
		std::string modeMsg = ":" + client.getNickname() + "!" + client.getUsername() + "@" + client.getHostname() + " MODE " + target + " " + tokens[2];

		std::map<std::string, const Client*> users = channel->getUsers();
		for (std::map<std::string, const Client*>::iterator it = users.begin();
			it != users.end(); ++it)
			server.sendRaw(it->second, modeMsg);
	}
	else // User Modes (only for self)
	{
		if (target != client.getNickname())
		{
			server.sendNumeric(&client, ERR_USERSDONTMATCH, ":Cant change modes for other users");
			return ;
		}

		if (tokens.size() == 2)
		{
			std::string mode = "+";
			if (client.getIsInvisible())
				mode += "i";
			server.sendNumeric(&client, RPL_UMODEIS, mode);
			return ;
		}

		std::string mode_flag = tokens[2];
		if (mode_flag.length() < 2 || (mode_flag[0] != '+' && mode_flag[0] != '-'))
		{
			server.sendNumeric(&client, ERR_UMODEUNKOWNFLAG, "Unknown MODE flag");
			return ;
		}
		bool sign = (mode_flag[0] == '+');

		for (size_t i = 1; i < mode_flag.length(); ++i)
		{
			char flag = mode_flag[i];
			
			switch (flag)
			{
				case 'i':
					client.setIsInvisible(sign);
					server.sendRaw(&client, ":" + client.getNickname() + " MODE " + client.getNickname() + " " + mode_flag);
					break ;
				default:
					server.sendNumeric(&client, ERR_UMODEUNKOWNFLAG, "Unknown MODE flag");
					break ;
			}
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
