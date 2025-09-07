#ifndef BOT_HPP
#define BOT_HPP

#include <string>
#include <ctime>

class Server;
class Client;
class Channel;

class Bot
{
    public:
        Bot(Server& srv, const std::string& nick = "BotServ", const std::string& user="bot");
        ~Bot();

        // Getter
        const Client* getIdentityBot() const;
        const std::string& getNickBot() const;

        // Registration in server (clientsByNick)
        void registerInServer();

        // Join into a channel
        void join(const std::string& channelName);

        // Server Hooks
        void onUserJoinedChannel(const Channel* ch, const Client* who);
        void onChannelMessage(const Channel* ch, const Client* from, const std::string& text);
        void onDirectMessage(const Client* from, const std::string& text);

    private:
        Server*       server;
        Client*       me;           // Intern client identity
        std::time_t   start;

        // helpers
        void replyChannel(const std::string& channel, const std::string& text);
        void replyUser(const Client* to, const std::string& text);
        static std::string toLower(std::string s);

        // Commands
        void cmdHelp(const Channel* ch, const Client* from);
        void cmdEcho(const Channel* ch, const Client* from, const std::string& rest);
        void cmdRoll(const Channel* ch, const Client* from, const std::string& rest);
        void cmdChoose(const Channel* ch, const Client* from, const std::string& rest);
        void cmdUptime(const Channel* ch, const Client* from);

};

#endif