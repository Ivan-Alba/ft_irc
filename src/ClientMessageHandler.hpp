#ifndef CLIENTMESSAGEHANDLER_HPP
#define CLIENTMESSAGEHANDLER_HPP

#include <string>
#include <vector>
#include <map>
#include <exception>

class Server;
class Client;

typedef void (*CommandHandler)(Server&, Client&, const std::vector<std::string>&);

class ClientMessageHandler
{
	public:

		struct modeContext
		{
			Server						&server;
			Channel						&channel;
			Client						&client;
			std::vector<std::string>	&tokens;
			size_t						paramIndex;
		};

		static void handleMessage(Server &server, Client &client);

	private:
		static std::map<std::string, CommandHandler>	commandMap;
		
		ClientMessageHandler() {} // Block default constructor

		// Command process
		static void	initCommandMap();
		static void	processCommand(Server &server, Client &client,
			const std::vector<std::string> &tokens);

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
		static void handlePing(Server &server, Client &client,
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
		static std::vector<std::string>	tokenize(std::string &line);
		
		static void			printTokens(const std::vector<std::string> &tokens);
};

#endif
