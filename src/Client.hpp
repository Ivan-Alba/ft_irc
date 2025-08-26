#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client
{
	private:
		int			clientFd;
		std::string	nickname;
		std::string	username;
		bool		passwordAccepted;
		bool		authenticated;
		std::string	buffer;

		Client(); // Block default constructor

	public:
		// Constructor
		Client(int fd);

		// Destructor
		~Client();

		// Getter
		int					getClientFd() const;
		const std::string&	getUsername() const;
		const std::string&	getNickname() const;
		bool				isPasswordAccepted() const;
		bool				isAuthenticated() const;

		const std::string&	getBuffer() const;
		std::string&		getBuffer();

		// Setter
		void	setClientFd(int fd);
		void	setNickname(const std::string &nickname);
		void	setUsername(const std::string &username);
		void	setPasswordAccepted(bool isAccepted);
		void	setAuthenticated(bool isAuth);

		// Utilities
		void	appendToBuffer(const std::string &newData);
};

#endif
