#include "Client.hpp"

#include <unistd.h>

// Constructor
Client::Client(int fd) : clientFd(fd), nickname(""), username(""),
	passwordAccepted(false), authenticated(false), buffer("") {}


// Destructor
Client::~Client()
{
	if (clientFd >= 0)
	{
		close(clientFd);
		clientFd = -1;
	}
}

// Getter
int	Client::getClientFd() const
{
	return (this->clientFd);
}

const std::string&	Client::getNickname() const
{
	return (this->nickname);
}

const std::string&	Client::getUsername() const
{
	return (this->username);
}

const std::string&	Client::getHostname() const
{
	return (this->hostname);
}

bool	Client::isPasswordAccepted() const
{
	return (this->passwordAccepted);
}

bool	Client::isAuthenticated() const
{
	return (this->authenticated);
}

const std::string&	Client::getBuffer() const
{
	return (this->buffer);
}

std::string&	Client::getBuffer()
{
	return (this->buffer);
}

const std::string&	Client::getBufferOut() const
{
	return (this->bufferOut);
}

std::string&	Client::getBufferOut()
{
	return (this->bufferOut);
}

// Setter
void	Client::setClientFd(int fd)
{
	this->clientFd = fd;
}

void	Client::setNickname(const std::string &nickname)
{
	this->nickname = nickname;
}

void	Client::setUsername(const std::string &username)
{
	this->username = username;
}

void	Client::setHostname(const std::string &hostname)
{
	this->hostname = hostname;
}



void	Client::setPasswordAccepted(bool isAccepted)
{
	this->passwordAccepted = isAccepted;
}

void	Client::setAuthenticated(bool isAuth)
{
	this->authenticated = isAuth;
}

// Utilities
void	Client::appendToBuffer(const std::string &newData)
{
	this->buffer += newData;
}

void	Client::appendToBufferOut(const std::string &newData)
{
	this->bufferOut += newData;
}
