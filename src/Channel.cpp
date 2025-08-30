#include "Channel.hpp"
#include "Client.hpp"

// Constructor
Channel::Channel(const std::string &name, const std::string &topic)	: name(name),
					topic(topic), key(""), userLimit(-1), inviteOnly(false),
					topicBlocked(true) {}

// Destructor
Channel::~Channel()
{
	users.clear();
	operators.clear();
	invited.clear();
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

const std::map<std::string, const Client*>&	Channel::getUsers() const
{
	return (this->users);
}

const std::set<const Client*>&	Channel::getOperators() const
{
	return (this->operators);
}

const std::set<const Client*>&	Channel::getInvited() const
{
	return (this->invited);
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
	if (users.find(newUser->getNickname()) == users.end())
	{
		users[newUser->getNickname()] = newUser;

		if (this->users.size() == 1)
		{
			addOperator(newUser);
		}

		if (this->isInviteOnly())
		{
			removeInvited(newUser);
		}
	}
}

void	Channel::addOperator(const Client *newOperator)
{
	if (operators.find(newOperator) == operators.end())
		operators.insert(newOperator);
}

void	Channel::addInvited(const Client *newInvited)
{
	if (invited.find(newInvited) == invited.end())
		invited.insert(newInvited);
}

void	Channel::removeUser(const std::string &nickname)
{
	users.erase(nickname);
}

void	Channel::removeOperator(const Client *client)
{
	operators.erase(client);
}

void	Channel::removeInvited(const Client *client)
{
	invited.erase(client);
}

void	Channel::cleanInvited()
{
	invited.clear();
}
