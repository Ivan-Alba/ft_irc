#include "Channel.hpp"
#include "Client.hpp"

// Constructor
Channel::Channel(const std::string &name, const std::string &topic)	: name(name),
	topic(topic), key(""), userLimit(50), inviteOnly(false), topicBlocked(true) {}

// Destructor
Channel::~Channel()
{
	users.clear();
	operators.clear();
}

// Getter
const std::string&	Channel::getName() const
{
	return (this->name);
}

const std::string&	Channel::getTopic() const
{
	return (this->topic);
}

const std::string&	Channel::getKey() const
{
	return (this->key);
}

int	Channel::getUserLimit() const
{
	return (this->userLimit);
}

bool	Channel::isInviteOnly() const
{
	return (this->inviteOnly);
}

bool	Channel::isTopicBlocked() const
{
	return (this->topicBlocked);
}

std::map<std::string, const Client*>	Channel::getUsers() const
{
	return (this->users);
}

// Setter
void	Channel::setTopic(const std::string &newTopic)
{
	this->topic = newTopic;
}

void	Channel::setKey(const std::string &newKey)
{
	this->key = newKey;
}

void	Channel::setUserLimit(int newLimit)
{
	this->userLimit = newLimit;
}

void	Channel::setInviteOnly(bool inviteOnly)
{
	this->inviteOnly = inviteOnly;
}

void	Channel::setTopicBlocked(bool topicBlocked)
{
	this->topicBlocked = topicBlocked;
}

// Utilities
void	Channel::addUser(const Client *newUser)
{
	users[newUser->getNickname()] = newUser;
}

void	Channel::addOperator(const Client *newOperator)
{
	operators.insert(newOperator);
}

void	Channel::removeUser(const std::string &nickname)
{
	users.erase(nickname);
}

void	Channel::removeOperator(const Client *operatr)
{
	operators.erase(operatr);
}
