#ifndef CLIENTMESSAGEHANDLER_HPP
#define CLIENTMESSAGEHANDLER_HPP

#include <string>
#include <vector>

class Server;
class Client;

class ClientMessageHandler
{
	public:
		static void handleMessage(Server &server, Client &client);

	private:
		ClientMessageHandler() {} // Block default constructor

		// Basic IRC commands
		static void handlePass(Server &server, Client &client,
			const std::vector<std::string> &tokens);
		static void handleNick(Server &server, Client &client,
			const std::vector<std::string> &tokens);
		static void handleUser(Server &server, Client &client,
			const std::vector<std::string> &tokens);
		static void handlePrivMsg(Server &server, Client &client,
			const std::vector<std::string> &tokens);
		static void handleJoin(Server &server, Client &client,
			const std::vector<std::string> &tokens);
		static void handlePart(Server &server, Client &client,
			const std::vector<std::string> &tokens);
		static void handleQuit(Server &server, Client &client,
			const std::vector<std::string> &tokens);

		// Operator commands
		static void handleKick(Server &server, Client &client,
			const std::vector<std::string> &tokens);
		static void handleInvite(Server &server, Client &client,
			const std::vector<std::string> &tokens);
		static void handleTopic(Server &server, Client &client,
			const std::vector<std::string> &tokens);
		static void handleMode(Server &server, Client &client,
			const std::vector<std::string> &tokens);

		// Utilities
		static std::vector<std::string>	tokenize(std::string &buffer);
	
};

#endif
