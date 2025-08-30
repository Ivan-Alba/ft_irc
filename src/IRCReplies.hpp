#ifndef IRC_REPLIES_HPP
#define IRC_REPLIES_HPP

// ============================
//  NUMERIC REPLIES (RPL_)
// ============================

#define	RPL_WELCOME			001	// "Welcome to the IRC network <nick>"

#define	RPL_YOURHOST		002
#define	RPL_CREATED			003
#define	RPL_MYINFO			004

# define RPL_TOPIC			332	// <client> <channel> :<topic>
# define RPL_NOTOPIC		331	// <client> <channel> :No topic is set

#define	RPL_NAMEREPLY		353	// "<client> = <channel> :[nickname {space nickname}*]"
#define	RPL_ENDOFNAMES		366	// "<client> <channel> :End of NAMES list"

#define RPL_CHANNELMODEIS   312 // "<chanel> <mode> <mode params>"
#define RPL_UMODEIS         221 // "<user mode string>"

// ============================
//  ERROR REPLIES (ERR_)
// ============================

// --- PASS / AUTH ---
#define	ERR_NEEDMOREPARAMS		461	// "<command> :Not enough parameters"
#define	ERR_ALREADYREGISTRED	462	// ":You may not reregister"
#define	ERR_PASSWDMISMATCH		464	// ":Password incorrect"

// --- JOIN ---
#define ERR_BADCHANNELKEY		475	// "<nick> <channel> :Cannot join channel (+k) - password required"
#define ERR_INVITEONLYCHAN		473	// "<nick> <channel> :Cannot join channel (+i)"

#define ERR_CHANOPRIVSNEEDED	482	// "<client> <channel> :You're not channel operator"

// --- NICK ---
#define	ERR_NONICKNAMEGIVEN		431	// ":No nickname given"
#define	ERR_ERRONEUSNICKNAME	432	// "<nick> :Erroneous nickname"
#define	ERR_NICKNAMEINUSE		433	// "<nick> :Nickname is already in use"

// --- USER ---
#define	ERR_ALREADYREGISTRED	462	// ":You may not reregister"

// --- PRIVMSG / NOTICE ---
#define	ERR_NORECIPIENT			411	// ":No recipient given (<command>)"
#define	ERR_NOTEXTTOSEND		412	// ":No text to send"
#define	ERR_NOSUCHNICK			401	// "<nick> :No such nick"
#define	ERR_NOSUCHCHANNEL		403	// "<channel> :No such channel"

// --- GENERAL ---
#define	ERR_UNKNOWNCOMMAND		421	// "<command> :Unknown command"
#define	ERR_NOTREGISTERED		451	// ":You have not registered"

// --- CHANNELS ---
#define	ERR_NOTONCHANNEL		442	// "<channel> :You're not on that channel"
#define	ERR_USERNOTINCHANNEL	441	// "<nick> <channel> :They aren't on that channel"
#define	ERR_CANNOTSENDTOCHAN	404	// "<channel> :Cannot send to channel"
#define ERR_CHANNELISFULL		471	// "<channel> :Cannot join channel (+l)"

// --- MODE ---
#define ERR_KEYSET				467	// "<client> <channel> :Channel key already set"
#define ERR_CHANOPRIVSNEEDED    482 // "<channel> :You're not channel operator"
#define ERR_UNKNOWNMODE         472 // "<char> : is unkown mode char to me"
#define ERR_USERSDONTMATCH      502 // ":Cant change mode for other users"
#define ERR_UMODEUNKOWNFLAG     501 // ":UKnown MODE flag"

#endif
