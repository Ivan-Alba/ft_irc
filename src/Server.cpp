#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "ClientMessageHandler.hpp"
#include "IRCReplies.hpp"
#include "config.hpp"

#include <iostream>
#include <sstream>
#include <cstring>
#include <ctime>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>

//Constructor
Server::Server(int port, const std::string &password) : port(port), password(password)
{
	// Create the server socket
	// int socket(int domain, int type, int protocol);
	// return: socket_fd OK / -1 ERROR
	listenFd = socket(serverConfig::domain, serverConfig::type, serverConfig::protocol);

	if (listenFd == -1)
	{
		throw std::runtime_error(
			std::string("socket() failed: ") + strerror(errno));
	}

	// Set socket options
	// int setsocktopt(int sockfd, int level, int optname,
	//					const void *optval, socklen_t optlen);
	// return: 0 OK / -1 ERROR

	if (setsockopt(listenFd, serverConfig::socketLevel, serverConfig::addrOpt,
			&serverConfig::addrValue, sizeof(serverConfig::addrValue)) == -1)
	{
		throw std::runtime_error(
			std::string("setsockopt() failed: ") + strerror(errno));
	}

	// Bind the socket to the specified IP address and port
	//int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
	// return: 0 OK / -1 ERROR
	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr)); //Clean struct

	addr.sin_family = serverConfig::domain;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = serverConfig::listenAddr;

	if (bind(listenFd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
	{
		throw std::runtime_error(
			std::string("bind() failed: ") + strerror(errno));
	}
	
	// Mark the socket as a passive socket to accept incoming connections
	// int listen(int sockfd, int backlog);
	// return: 0 OK / -1 ERROR

	if (listen(listenFd, serverConfig::backlog) == -1)
	{
		throw std::runtime_error(
			std::string("listen() failed: ") + strerror(errno));
	}

	// Set the listening socket to non-blocking mode
	// return: 0 OK / -1 ERROR
	if (fcntl(listenFd, serverConfig::fcntlCmd, serverConfig::fcntlFlag) == -1)
	{
		throw std::runtime_error(
			std::string("fcntl() fauked: ") + strerror(errno));
	}
}

//Destructor
Server::~Server()
{
	close(listenFd);

	for (std::map<int, Client*>::iterator it = clientsByFd.begin();
		it != clientsByFd.end(); ++it)
	{
    	delete it->second;
	}
	
	clientsByFd.clear();
	clientsByNick.clear();

	for (std::map<std::string, Channel*>::iterator it = channels.begin();
		it != channels.end(); ++it)
	{
    	delete it->second;
	}
	
	channels.clear();
}

void	Server::addChannel(const std::string &name, const std::string &topic)
{
	if (channels.find(name) == channels.end())
	{
		Channel *newChannel = new Channel(name, topic);
		channels[name] = newChannel;
	}
	else
		throw std::runtime_error("Channel already exists.");
}

// Getter
const std::string&	Server::getPassword() const
{
	return (this->password);
}

const std::map<std::string, Client*>&	Server::getClientsByNick() const
{
	return (this->clientsByNick);
}

const std::map<std::string, Channel*>&	Server::getChannels() const
{
	return (this->channels);
}

// Execution flow
void	Server::run()
{
	addChannel("#welcome", "Welcome channel");

	addPollFd(listenFd);

	while (true)
	{
		if (poll(&pollFds[0], pollFds.size(), serverConfig::pollTimeout) < 0)	
            throw std::runtime_error("poll() failed");
		
		size_t n = pollFds.size();
		for (size_t i = 0; i < n; ++i)
		{
			// READ (POLLIN)
			if (pollFds[i].revents & POLLIN)
			{
				if (pollFds[i].fd == listenFd)	// Server poll
				{
					try
					{
						acceptNewClient();
					}
					catch (const std::exception &e)
					{
						std::cerr << "Error: " << e.what() << std::endl;
					}		
				}
				else // Client poll
				{
					try
					{
						handleClientMessage(pollFds[i].fd);
					}
					catch (const ClientDisconnectedException &e) {}
				}
			}

			// WRITE (POLLOUT)
			if (pollFds[i].revents & POLLOUT)
			{
				Client* client = clientsByFd[pollFds[i].fd];
				if (!client->getBufferOut().empty())
				{
					sendPendingMessages(client);
					if (client->getBufferOut().empty())
					{
                        pollFds[i].events &= ~POLLOUT;
					}
				}
			}

			pollFds[i].revents = 0;
		}
	}
}

void	Server::acceptNewClient()
{
	int					clientFd;
	struct sockaddr_in	clientAddr;
	socklen_t			clientLen;

	clientLen = sizeof(clientAddr);

	clientFd = accept(listenFd, (struct sockaddr*)&clientAddr, &clientLen);
	
	if (clientFd == -1)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK) // Non-critic errors
		{
			throw std::runtime_error(
				std::string("Cannot accept client: ") + strerror(errno));
		}
	}
	else
	{
		int flags = fcntl(clientFd, F_GETFL, 0);
		fcntl(clientFd, F_SETFL, flags | O_NONBLOCK);

		logMessage("New client accepted");
		Client *newClient = new Client(clientFd);
		clientsByFd[clientFd] = newClient;
		std::ostringstream oss;
		oss << "Total clients: " << clientsByFd.size();
		logMessage(oss.str());

		addPollFd(clientFd);
	}
}

void	Server::handleClientMessage(int fd)
{
	std::map<int, Client*>::iterator	it;
	char								buf[BUFFER_SIZE];
	Client 								*client;

	it = clientsByFd.find(fd);
	if (it == clientsByFd.end())
		return ;
		
	client = it->second;

	int bytesRead = recv(client->getClientFd(), buf, BUFFER_SIZE - 1, 0);

	if (bytesRead > 0)
	{
		client->appendToBuffer(std::string(buf, bytesRead));
		// TODO DEBUG
    	std::cout << "DEBUG Client[" << fd << "]:"
			<< std::string(buf, bytesRead) << std::endl;

		std::cout << "Client[" << fd << "] buffer: " << client->getBuffer() << std::endl;
		ClientMessageHandler::handleMessage(*this, *client);
	}
	else if (bytesRead == 0
			|| (bytesRead < 0 && errno != EAGAIN && errno != EWOULDBLOCK))
	{
		disconnectClient(client, "Client connection lost");
	}
}

void	Server::disconnectClient(Client *client, const std::string& reason)
{
	std::ostringstream oss;
	oss << "Client[" << client->getClientFd() << "] disconnected.";
	logMessage(oss.str());

	std::string msg = "ERROR :disconnected: " + reason + "\r\n";
	send(client->getClientFd(), msg.c_str(), msg.size(), 0); 

	removePollFd(client->getClientFd());
	
	if (client->getClientFd() >= 0)
	{
		close(client->getClientFd());
		client->setClientFd(-1);
	}

    clientsByFd.erase(client->getClientFd());
    if (!client->getNickname().empty())
	{
		clientsByNick.erase(client->getNickname());
	}

	for (std::map<std::string, Channel*>::iterator it = channels.begin();
		it != channels.end(); ++it)
	{
		Channel* chan = it->second;
		if (chan)
		{
			chan->removeUser(client->getNickname());
			chan->removeOperator(client);
			chan->removeInvited(client);
		}
	}

	delete client;

	throw ClientDisconnectedException();
}

void	Server::authenticateClient(Client *client)
{
	if (client->isPasswordAccepted()
		&& !client->getNickname().empty() && !client->getUsername().empty())
	{
		client->setAuthenticated(true);

		clientsByNick[client->getNickname()] = client;

		sendNumeric(client, RPL_WELCOME, std::string("Welcome to " + serverConfig::serverName
			+ " " + client->getNickname()));
	}
}

void	Server::addPollFd(int fd)
{
	struct pollfd	newPoll;

	newPoll.fd = fd;
	newPoll.events = serverConfig::pollReadEvent;
	pollFds.push_back(newPoll);
}

void	Server::removePollFd(int fd)
{
	for (size_t i = 0; i < pollFds.size(); ++i)
	{
		if (pollFds[i].fd == fd)
		{
			pollFds[i] = pollFds.back();
			pollFds.pop_back();
			return ;
		}
	}
}

// Utilities
void	Server::sendRaw(const Client *client, const std::string &text)
{
	sendToClient(client->getClientFd(), text + "\r\n");
}

void	Server::sendNotice(const Client *client, const std::string &text)
{
	std::string	msg;
	std::string	target = "*";

	if (!client->getNickname().empty())
		target = client->getNickname();

	msg = ":" + serverConfig::serverName + " NOTICE " + target
		+ " :" + text + "\r\n";

	sendToClient(client->getClientFd(), msg);
}

void	Server::sendPrivMsg(const Client *from, const std::string &target,
							const Client* to, const std::string &text)
{
	if (!from || !to)
		return;

	std::string prefix = ":" + from->getNickname() + "!" + from->getUsername()
							+ "@" + from->getHostname();

	std::string fullMessage = prefix + " PRIVMSG " + target	+ " :" + text + "\r\n";

    sendToClient(to->getClientFd(), fullMessage);
}

void	Server::sendError(const Client *client, const std::string &text)
{
	std::string	msg;

	msg = "ERROR :" + text + "\r\n";

    sendToClient(client->getClientFd(), msg);
}

void	Server::sendNumeric(Client* client, int numeric, const std::string &message)
{
	std::ostringstream	oss;
	std::string			numericStr;
	std::string			fullMessage;
	std::string			target = "*";

	if (!client->getNickname().empty())
		target = client->getNickname();

	if (numeric < 10)
		oss << "00" << numeric;
	else if (numeric < 100)
		oss << "0" << numeric;
	else
		oss << numeric;

	numericStr = oss.str();

	fullMessage = ":" + serverConfig::serverName + " " 
						+ numericStr + " " 
						+ target + " :" 
						+ message + "\r\n";

	sendToClient(client->getClientFd(), fullMessage);
}

void	Server::sendNameReply(Client* client, const std::string &channel,
								const std::string &userList)
{
	std::string fullMessage = ":" + serverConfig::serverName
							+ " 353 " + client->getNickname()
							+ " = " + channel
							+ " :" + userList + "\r\n";

	sendToClient(client->getClientFd(), fullMessage);
}

void	Server::sendEndOfNames(Client* client, const std::string &channel)
{
	std::string fullMessage = ":" + serverConfig::serverName
							+ " 366 " + client->getNickname()
							+ " " + channel
							+ " :End of NAMES list\r\n";

	sendToClient(client->getClientFd(), fullMessage);
}

void	Server::sendToClient(int clientFd, const std::string &message)
{
	Client* client = clientsByFd.at(clientFd);

	// Pending messages
	if (!client->getBufferOut().empty())
	{
		client->appendToBufferOut(message);
		markPollFdWritable(clientFd);
		return;
	}

	ssize_t bytesSent = send(clientFd, message.c_str(), message.size(), 0);

	if (bytesSent == -1) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			client->appendToBufferOut(message);
			markPollFdWritable(clientFd);
		}
		else
		{
			disconnectClient(client, "Cannot send message.");
		}
	}
	else if (bytesSent < (ssize_t)message.size())
	{
		client->appendToBufferOut(message.substr(bytesSent));
		markPollFdWritable(clientFd);
	}
}

void	Server::sendPendingMessages(Client* client)
{
	std::string& outBuffer = client->getBufferOut();

	while (!outBuffer.empty())
	{
		int bytesSent = send(client->getClientFd(), outBuffer.c_str(),
				outBuffer.size(), 0);

		if (bytesSent > 0)
		{
			outBuffer.erase(0, bytesSent);
		}
		else if (bytesSent == -1)
		{
			if (errno != EAGAIN || errno != EWOULDBLOCK)
			{
				disconnectClient(client, "Cannot send pending message");
			}
			break;
		}
	}
}

void	Server::markPollFdWritable(int fd)
{
	for (size_t i = 0; i < pollFds.size(); ++i)
	{
		if (pollFds[i].fd == fd)
		{
			pollFds[i].events |= POLLOUT; // Add flag POLLOUT
			return;
		}
	}
}

void	Server::logMessage(const std::string &msg) const
{
	std::time_t	now;
	char buf[20];
	
	now = std::time(NULL);
	std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

	std::cout << "[" << buf << "] " << msg << std::endl;
}

// Exception
const char*	Server::ClientDisconnectedException::what() const throw()
{
	return ("Client disconnected");
}
