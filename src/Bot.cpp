#include "Bot.hpp"
#include "Server.hpp"
#include "Client.hpp"
#include "Channel.hpp"
#include "config.hpp"
#include "Utils.hpp"
#include <sstream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <cstdio>

static int randRange(int lo, int hi)
{
    return lo + (std::rand() % (hi - lo + 1));
}

Bot::Bot(Server& srv, const std::string& nick, const std::string& user)
: server(&srv), me(new Client(-1)), start(std::time(NULL))
{
    // Authenticated Identity
    me->setNickname(nick);
    me->setUsername(user);
    me->setHostname(serverConfig::serverName);
    me->setPasswordAccepted(true);
    me->setAuthenticated(true);
    std::srand(static_cast<unsigned int>(start));
}

Bot::~Bot()
{
    delete me;
}

const Client* Bot::getIdentityBot() const
{
    return me;
}

const std::string& Bot::getNickBot() const
{
    return me->getNickname();
}

void Bot::registerInServer()
{
    // Registrer Bot like a real client (without fd)
    // -> allow PRIVMSG <BotNick> it works with clientsByNick
    server->registerBotClient(me);
}

void Bot::join(const std::string& channelName)
{
    // Create channel if doesn't exists and add Bor like a user
    std::map<std::string, Channel*> chans = server->getChannels();
    Channel* ch = NULL;
    std::map<std::string, Channel*>::iterator it = chans.find(channelName);
    if (it == chans.end())
    {
        server->addChannelBot(channelName, ""); // Privat metohd to Bot
        chans = server->getChannels();
        ch = chans[channelName];
    }
    else
        ch = it->second;
    if (ch)
        ch->addUser(me);
    
    // Msg to JOIN like a normal client
    std::string joinMsg = ":" + me->getNickname() + "!" + me->getUsername()
        + "@" + me->getHostname() + " JOIN " + channelName + "\r\n";
    
    const std::map<std::string, const Client*>& users = ch->getUsers();
    for (std::map<std::string, const Client*>::const_iterator ui = users.begin(); ui != users.end(); ++ui)
        server->sendRaw(ui->second, joinMsg); 
}

void Bot::onUserJoinedChannel(const Channel* ch, const Client* who)
{
    if (!ch || !who) return;
    // If Bot is in channel, send a msg
    const std::map<std::string, const Client*>& users = ch->getUsers();
    if (users.find(me->getNickname()) != users.end())
    {
        replyChannel(ch->getName(), "Welcome " + who->getNickname() + " 👋 — try !help to see commands");
    }
}

void Bot::onChannelMessage(const Channel* ch, const Client* from, const std::string& text)
{
    if (!ch || !from) return;
    if (text.empty() || text[0] != '!') return; // Prefx cmds

    std::string cmd, rest;
    size_t sp = text.find(' ');
    if (sp == std::string::npos) { cmd = text.substr(1); rest = ""; }
    else { cmd = text.substr(1, sp-1); rest = Utils::trim(text.substr(sp+1)); }

    cmd = toLower(cmd);
    if (cmd == "help")      cmdHelp(ch, from);
    else if (cmd == "echo") cmdEcho(ch, from, rest);
    else if (cmd == "roll") cmdRoll(ch, from, rest);
    else if (cmd == "choose") cmdChoose(ch, from, rest);
    else if (cmd == "uptime") cmdUptime(ch, from);
    else replyChannel(ch->getName(), "Command not found. Try !help");
}

void Bot::onDirectMessage(const Client* from, const std::string& text)
{
    if (!from) return;
    // Ask with NOTICE
    if (text == "help" || text == "!help") {
        replyUser(from, "Commands:\n!help, to see commands\n!echo <msg>, to chat\n!roll , to roll a dice\n!choose a|b|c, i will choose a choice\n!uptime, to see my logtime");
        return;
    }
    replyUser(from, "Hi " + from->getNickname() + ". Type !help");
}

void Bot::replyChannel(const std::string& channel, const std::string& text)
 {
    // Send a PRIVMSG to everyone like a real client
    std::map<std::string, Channel*> chans = server->getChannels();
    std::map<std::string, Channel*>::iterator it = chans.find(channel);
    if (it == chans.end())
        return;

    const std::map<std::string, const Client*>& users = it->second->getUsers();
    for (std::map<std::string, const Client*>::const_iterator ui = users.begin(); ui != users.end(); ++ui)
    {
        if (ui->second != me)
        {
            // Ignore Bots
            server->sendPrivMsg(me, channel, ui->second, text); 
        }
    }
}

void Bot::replyUser(const Client* to, const std::string& text)
{
    server->sendNotice(to, text); 
}

std::string Bot::toLower(std::string s)
{
    for (size_t i=0;i<s.size();++i) s[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
    return s;
}

// ----- Commands -----
void Bot::cmdHelp(const Channel* ch, const Client* from)
{
    (void)from;
    (void)from;
    replyChannel(ch->getName(), "Commands:");
    replyChannel(ch->getName(), "!help - to see commands");
    replyChannel(ch->getName(), "!echo <msg> - to chat");
    replyChannel(ch->getName(), "!roll - to roll a dice");
    replyChannel(ch->getName(), "!choose a|b|c - I will choose a choice");
    replyChannel(ch->getName(), "!uptime - to see my logtime");
}

void Bot::cmdEcho(const Channel* ch, const Client* from, const std::string& rest)
{
    if (rest.empty()) replyChannel(ch->getName(), from->getNickname()+": (empty)");
    else replyChannel(ch->getName(), rest);
}

void Bot::cmdRoll(const Channel* ch, const Client* from, const std::string& rest)
{
    // Void argument
    (void)rest; 
    // Roll a Dice
    int rollValue = randRange(1, 6);
    std::ostringstream oss;
    oss << from->getNickname() << " rolls a dice with value: " << rollValue;
    replyChannel(ch->getName(), oss.str());
}

void Bot::cmdChoose(const Channel* ch, const Client* from, const std::string& rest)
{
    (void)from;
    std::vector<std::string> opts = Utils::split(rest, '|');
    if (opts.empty())
    {
        replyChannel(ch->getName(), "How to use: !choose a|b|c"); return;
    }
    int i = randRange(0, (int)opts.size()-1);
    replyChannel(ch->getName(), "I choose: " + Utils::trim(opts[i]));
}

void Bot::cmdUptime(const Channel* ch, const Client* from)
{
    (void)from;
    std::time_t now = std::time(NULL); long s = (long)difftime(now, start);
    long h = s/3600; s%=3600; long m = s/60; s%=60;
    std::ostringstream oss; oss << "Uptime: " << h << "h " << m << "m " << s << "s";
    replyChannel(ch->getName(), oss.str());
}