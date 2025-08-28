#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <set>
#include <map>

class Client;

class Channel
{
	private:
		std::string								name;
		std::string								topic;
		std::string								key;
		std::map<std::string, const Client*>	users;
		std::set<const Client*>					operators;
		std::set<const Client*>					invited;
		int										userLimit;
		bool									inviteOnly;
		bool									topicBlocked;

		Channel(); //Block default constructor

	public:
		// Constructor
		Channel(const std::string &name, const std::string &topic);
		
		// Destructor
		~Channel();

		// Getter
		const std::string&	getName() const;
		const std::string&	getTopic() const;
		const std::string&	getKey() const;
		int					getUserLimit() const;
		bool				isInviteOnly() const;
		bool				isTopicBlocked() const;

		const std::map<std::string, const Client*>&	getUsers() const;
		const std::set<const Client*>&	getOperators() const;
		const std::set<const Client*>&	getInvited() const;

		// Setter
		void	setTopic(const std::string &newTopic);
		void	setKey(const std::string &newKey);
		void	setUserLimit(int newLimit);
		void	setInviteOnly(bool inviteOnly);
		void	setTopicBlocked(bool topicBlocked);

		// Utilities
		void	addUser(const Client* newUser);
		void	addOperator(const Client* newOperator);
		void	addInvited(const Client* newInvited);
		void	removeUser(const std::string &nickname);
		void	removeOperator(const Client* client);
		void	removeInvited(const Client* client);

		void	cleanInvited();

};

#endif
