#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client
{
	private:
		int			clientFd;
		std::string	nickname;
		std::string	username;
		std::string	hostname;
		bool		passwordAccepted;
		bool		authenticated;
		bool		isInvisible;
		std::string	buffer;
		std::string	bufferOut;

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
		const std::string&	getHostname() const;
		bool				isPasswordAccepted() const;
		bool				isAuthenticated() const;
		bool				getIsInvisible() const;

		const std::string&	getBuffer() const;
		std::string&		getBuffer();

		const std::string&	getBufferOut() const;
		std::string&		getBufferOut();

		// Setter
		void	setClientFd(int fd);
		void	setNickname(const std::string &nickname);
		void	setUsername(const std::string &username);
		void	setHostname(const std::string &hostname);
		void	setPasswordAccepted(bool isAccepted);
		void	setAuthenticated(bool isAuth);
		void	setIsInvisible(bool isNotVisible);

		// Utilities
		void	appendToBuffer(const std::string &newData);
		void	appendToBufferOut(const std::string &newData);
};

#endif
