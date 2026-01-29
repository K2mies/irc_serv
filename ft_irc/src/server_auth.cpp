#include "Server.hpp"

// --------------------------------------------------------------------- command PASS
void  Server::authPASS ( Client& client, const Command& cmd ){
	if (  cmd.params.empty()  ) {
		sendError(client, 461, "PASS :Not enough parameters");
		return;
	}
	if (  client.isRegistered()  ) {
		sendError(client, 462, ":You are already registered"  );
		return;
	}

	const std::string& pass = cmd.params[0];
	if (  _password.empty() || pass == _password ) {
		client.setPassOk(  true  );
	}
	else {
		sendError( client, 464, ":Password incorrect");
	}
}

// --------------------------------------------------------------------- command NICK
static bool isValidNick(const std::string &nick){
	if (nick.empty()) return false;
	if (nick[0] == '#' || nick[0] == ':')return false;
	if (std::isdigit(nick[0])) return false;
	for (size_t i = 0; i < nick.size(); ++i){
		if (nick[i] == ' ' || nick[i] == '\r' || nick[i] == '\n')
			return false;
	}
	return true;
}

void Server::authNICK(Client& client, const Command& cmd){
	if (cmd.params.empty() && !client.isRegistered()){
		sendError(client, 431, ":No nickname given");
		return ;
	}
	bool nick_changed = false;

	if (cmd.params.empty() && client.isRegistered()){
		std::string msg = ":" + client.nick() + " NICK :" + client.nick() + "\r\n";
		(void)send(client.fd(), msg.c_str(), msg.size(), MSG_NOSIGNAL);
		std::cerr << "Server output: " << msg << "\n";
		return ;
	}

	const std::string& newNick = cmd.params[0];
	if (  newNick.empty())  {
		sendError(client, 431, ":No nickname given");
		return ;
	}

	// is the nick valid?
	if (!isValidNick(newNick)){
		sendError(client, 432, newNick + ":Erroneous nickname");
		return ;
	}

	//if nick is already the same
	if (client.nick() == newNick)
		return ;
	else
		nick_changed = true;

	// Nick in use by someone else?
	ClientsMapNick::iterator it = _clients_by_nick.find(newNick);
	if (it != _clients_by_nick.end() && it->second != &client){
		sendError(client, 433, newNick + " :Nickname is already in use");
		return ;
	}

	// Remove old nick mapping if present
	if (!client.nick().empty())  {
		ClientsMapNick::iterator old = _clients_by_nick.find(client.nick()  );
		if (  old != _clients_by_nick.end() && old->second == &client){
			_clients_by_nick.erase(old);
		}
	}
	std::string msg = ":" + client.nick() + " NICK :" + newNick + "\r\n";
	client.setNick(  newNick );
	_clients_by_nick[newNick] = &client;
	if (nick_changed) {
		(void)send(client.fd(), msg.c_str(), msg.size(), MSG_NOSIGNAL);
		std::cerr << "Server output: " << msg << "\n";
	}
}

// --------------------------------------------------------------------- command USER
void  Server::authUSER( Client& client, const Command& cmd){
	if (  client.isRegistered()  ) {
		sendError(client, 462, ":You are already registered"  );
		return;
	}
	if (cmd.params.empty() || cmd.params.size() < 4){
		sendError(client, 461, "USER :Not enough parameters");
		return ;
	}
	
	client.setUser(cmd.params[0]);
}

// ------------------------------------------------------------------ Welcome message
void  Server::maybeWelcome(Client& client){
	std::string prefix = ":ircserv ";

		client.queue(prefix + "001 " + client.nick() +
				" :Welcome to the Internet Relay Network " +
				client.nick() + "!" + client.user() + "@localhost");

		client.queue(prefix + "002 " + client.nick() +
				" :Your host is localhost, running version 1.0");

		client.queue(prefix + "003 " + client.nick() +
				" :This server was created today");

		client.queue(prefix + "004 " + client.nick() +
				" localhost 127.0.0.1");

		// MOTD start
		client.queue(prefix + "375 " + client.nick() +
				" :- Message of the Day -");

		client.queue(prefix + "372 " + client.nick() +
				" :- Welcome to my IRC server!");

		client.queue(prefix + "376 " + client.nick() +
				" :End of /MOTD command");
}
