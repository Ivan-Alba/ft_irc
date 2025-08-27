#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <map>
#include <vector>
#include <poll.h>

class Channel;
class Client;

class Server
{
	private:
		int								listenFd;
		int								port;
		std::string						password;
		std::map<std::string, Channel*>	channels;
		std::map<std::string, Client*>	clientsByNick;
		std::map<int, Client*>			clientsByFd;
		std::vector<struct pollfd>		pollFds;
		
		Server(); // Block default constructor

		void	addChannel(const std::string &name, const std::string &topic);
		void	acceptNewClient();
		void	handleClientMessage(int fd);
		void	addPollFd(int fd);
		void	removePollFd(int fd);
		void	sendToClient(int clientFd, const std::string &message);
		void	sendPendingMessages(Client* client);
		void	markPollFdWritable(int fd);

	public:
		// Constructor
		Server(int port, const std::string &password);
		
		// Destructor
		~Server();

		// Getter
		const std::string&	getPassword() const;


		// Execution loop
		void	run();
		void	disconnectClient(Client *client, const std::string &reason);

		// Utilities
		void	logMessage(const std::string &msg) const;
		void	sendNotice(const Client *client, const std::string &text);	
		void	sendError(const Client *client, const std::string &text);
		void	sendPrivMsg(const Client *from, const Client* to,
					const std::string &text);
		void	sendNumeric(Client* client, int numeric, const std::string &message);
		void	authenticateClient(Client *client);
};

#endif
