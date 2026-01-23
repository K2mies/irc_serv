#include "Server.hpp"
#include "Channel.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

// ------------------------------------------------------------ command handlers

std::string Server::prefix(Client& client) {
	std::string out;
	out = client.nick() + "! " + client.nick() + "@ircserv" + " ";
	return out;
}

// ---------------------------------------------------------------- command PASS
void  Server::cmdPASS ( Client& client, const Command& cmd ){
	if (  cmd.params.empty()  ) {
		sendError(client, 461, "PASS :Not enough parameters");
		return;
	}
	if (  client.isRegistered()  ) {
		sendError(client, 462, ":You may not register"  );
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

void Server::cmdJOIN( Client& client, const Command& cmd ) {
	if (  cmd.params.empty()  ) {
		sendError(client, 461, "JOIN :Not enough parameters");
		return;
	}
	const std::string& name = cmd.params[0];
	if (_channels.contains(name)) {
		Channel& ch = _channels.at(name);
		if (ch.invite_only && !ch.invites.contains(client.fd())) {
			std::string	err = pref + "475 " + client.nick() + " " + ch.name + " :Cannot join channel (+k)";
			client.queue(err);
			sendError(client, 489, "JOIN :invite only peasant");
			return;
		}
		else if (ch.limit && ch.members.size() >= (unsigned long)ch.limit) {
			sendError(client, 471, "ERR_CHANNELISFULL");
			return;
		}
		if (!ch.key.empty() && cmd.params.size() < 2) {
			std::string	err = pref + "475 " + client.nick() + " " + ch.name + " :Cannot join channel (+k)";
			client.queue(err);
			sendError(client, 461, "JOIN :Not enough parameters");
			return;
		}	
		if (!ch.key.empty() && cmd.params[1] != ch.key) {
			std::string	err = pref + "475 " + client.nick() + " " + ch.name + " :Cannot join channel (+k)";
			client.queue(err);
			sendError(client, 475, "ERR_BADCHANNELKEY");
			return;			
		}
		ch.members.insert(client.fd());
	}
	else {
		auto [it, created] = _channels.try_emplace(name, name); // calls Channel(name)
		if (created) {
			Channel& ch = it->second;
			ch.members.insert(client.fd());
			ch.ops.insert(client.fd());
		}
	}
	Channel& ch = _channels.at(name);
	std::string out = prefix(client) + "JOIN " + ch.name;
	client.queue(out);
	if (ch.topic.empty())
		out = pref + "331 " + client.nick() + " " + ch.name + " :No topic set!!! " + ch.name;
	else
		out = pref + "332 " + client.nick() + " " + ch.name + " " + ch.topic + ch.name;
	client.queue(out);
	out = pref + "353 " + client.nick() + " = " + ch.name + " :";
	for (auto& fd : ch.members) {
		auto it = _clients_by_fd.find(fd);
		if (it == _clients_by_fd.end() || it->second == nullptr)
			continue;
		Client* client = it->second;
		if (ch.ops.contains(client->fd()))
			out += "@";
		out += client->nick() + " ";
	}
	client.queue(out);
	out = pref + "366 " + client.nick() + " " + ch.name + " End of /NAMES list";
	client.queue(out);
}

//void Server::cmdJOIN( Client& client, const Command& cmd ) {
//	if (  cmd.params.empty()  ) {
//		sendError(client, 461, "JOIN :Not enough parameters");
//		return;
//	}
//	const std::string& name = cmd.params[0];
//	if (_channels.contains(name)) {
//		Channel& ch = _channels.at(name);
//		if (ch.members.contains(client.fd()))
//			return	;	
//		if (ch.invite_only){
//			if (!ch.invites.contains(client.fd())){
//				sendError(client, 473, name + " :Cannot join channel (+i)");
//				return ;
//			}
//
//			//consume invite on success:
//			ch.invites.erase(client.fd());
//		}
//
//		//now safe to join
//		ch.members.insert(client.fd());
//
//	//	if (ch.invite_only && !ch.invites.contains(client.fd())) {
//	//		sendError(client, 489, "JOIN :Invite only peasant");
//	//		return;
//	//	}
//	//	else
//	//		ch.members.insert(client.fd());
//	}
//	else {
//		auto [it, created] = _channels.try_emplace(name, name); // calls Channel(name)
//		Channel& ch = it->second;
//		if (created) {
//			ch.members.insert(client.fd());
//			ch.ops.insert(client.fd());
//		}
//	}
//	Channel& ch = _channels.at(name);
//	std::string out = prefix(client) + "JOIN :" + ch.name ;
//	client.queue(out);
//	out = pref + "332 " + client.nick() + " " + ch.name + " :Welcome to " + ch.name;
//	client.queue(out);
//	out = pref + "353 " + client.nick() + " = " + ch.name + " :";
//	for (auto& fd : ch.members) {
//		auto it = _clients_by_fd.find(fd);
//		if (it == _clients_by_fd.end() || it->second == nullptr)
//			continue;
//		Client* client = it->second;
//		if (ch.ops.contains(client->fd()))
//			out += "@";
//		out += client->nick() + " ";
//	}
//	//out += "\r\n";
//	client.queue(out);
//	out = pref + "366 " + client.nick() + " " + ch.name + " :End of /NAMES list";
//	client.queue(out);
//}

void Server::cmdTOPIC( Client& client, const Command& cmd ) {
	if (!_channels.contains(cmd.params[0])) {
		sendError(client, 401, "TOPIC :No such channel");
		return;
	}
	Channel& ch = _channels.at(cmd.params[0]);
	if (cmd.params.size() == 1) {

		if (ch.topic.empty()) {
			client.queue(pref + "331" + client.nick() + ch.name + ":No topic set");
			return;
		}
		else {
			client.queue(pref + "332 " + client.nick() + " " + ch.name + " " + ch.topic);
			return;	
		}
	}
}

void	Server::cmdINVITE(Client& inviter, const Command& cmd){
	// INVITE <nick> <channel>
	if (cmd.params.size() < 2){
		sendError(inviter, 461, "INVITE :Not enough parameters");
		return ;
	}

	const std::string& nick		= cmd.params[0];
	const std::string& chanName = cmd.params[1];

	// find target client
	Client* target = getClientByNick(nick);
	if (!target){
		sendError(inviter, 401, nick + " :No such nick/channel");
		return ;
	}

	// find Channel
	Channel* ch = getChannel(chanName);
	if (!ch){
		sendError(inviter, 403, chanName + " :No such channel");
		return ;
	}

	// inviter must be in channel
	if (!ch->members.contains(inviter.fd())){
		sendError(inviter, 442, chanName + " :You're not on that channel");
		return ;
	}

	// if invite-only, only ops can invite
	if (ch->invite_only && !ch->ops.contains(inviter.fd())){
		//sendError(inviter, 482, chanName + " :You're not a channel operator");
		//sendError(inviter, 341, "");

		sendError(inviter, 341, chanName + " :You're not a channel operator");
		// technically the wrong numeric but seems to behaive as expected
		//sendError(inviter, 482, chanName + " :You're not a channel operator");
		return ;
	}

	// target already in channel?
	if (ch->members.contains(target->fd())){
		sendError(inviter, 443, nick + " " + chanName + " :is already on channel");
		return;}

	// record invite
	ch->invites.insert(target->fd());

	// numeric back to inviter: 341 RPL_INVITING
	// format	:server 341 <inviteNick> <targetNick> <channel>
	sendNumeric(inviter, 341, nick + " " + chanName);

	// send INVITE message to target
	// format: :<inviter> INVITE <target> :<channel>
	std::string msg = ":" + inviter.nick() + "!" + inviter.user() + "@ircserv " + "INVITE " + nick + " :" + chanName;
	target->queue(msg);
}

void Server::broadcast(Channel& ch, std::string msg, const Client* sender) {
	for (auto it = ch.members.begin(); it != ch.members.end(); ) {
		Client* client = getClientByFd(*it);
		if (!client) {
			it = ch.members.erase(it); // CLEAN DEAD FD
			continue;
		}
		if (sender && client->fd() == sender->fd()) {
			++it;
			continue;
		}
		client->queue(msg);
		++it;
	}
}


// void Server::cmdKICK( Client& c, const Command& cmd ) {


// }


// ---------------------------------------------------------------- command NICK

static bool isValidNick(const std::string &nick){
	if (nick.empty()) return false;
	if (nick[0] == '#' || nick[0] == ':')return false;
	for (size_t i = 0; i < nick.size(); ++i){
		if (nick[i] == ' ' || nick[i] == '\r' || nick[i] == '\n')
			return false;
	}
	return true;
}

void Server::cmdNICK(Client& client, const Command& cmd){
	if (cmd.params.empty()){
		sendError(client, 431, ":No nickname given");
		return ;
	}

	const std::string& newNick = cmd.params[0];
	if (  newNick.empty())  {
		sendError(client, 431, ":No nickname given");
		return ;
	}

	// is the nick valid?
	if (!isValidNick(newNick)){
		sendError(client, 432, newNick + " :Erroneous nickname");
		return ;
	}

	//if nick is already the same
	if (client.nick() == newNick)
		return ;

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

	client.setNick(  newNick );
	_clients_by_nick[newNick] = &client;

}

// ---------------------------------------------------------------- command USER
void  Server::cmdUSER( Client& client, const Command& cmd){
	if (cmd.params.empty()){
		sendError(client, 461, "USER :Not enough parameters");
		return ;
	}
	if (client.isRegistered()){
		sendError(client, 462, ":You may not register");
		return ;
	}

	client.setUser(cmd.params[0]);
	client.tryCompleteRegistration();
}

// ------------------------------------------------------------- command PRIVMSG
void	Server::cmdPRIVMSG( Client& sender, const Command& cmd){
	// in this case client == sender
	if (cmd.params.empty()){
		sendError(sender, 411, ":No recipent given (PRIVMSG)");
		return;
	}
	if	(cmd.params.size() < 2 || cmd.params[1].empty()){
		sendError(sender, 412, ":No text to send");
		return;
	}
	const std::string& target	= cmd.params[0];
	const std::string& text		= cmd.params[1];

	//build the prefix part once
	const std::string prefix = ":" + sender.nick() + "!" + sender.user() + "@ircserv ";

	//channel message
	if (!target.empty() && target[0] == '#'){
		Channel* ch = getChannel(target);
		if (!ch){
			sendError(sender, 403, target + " :No such channel");
			return;
		}

		if (!_channels.contains(target)) {
			sendError(sender, 403, target + " :No such channel");
			return;
		}

		const std::string prefix	= ":" + sender.nick() + "!" + sender.user() + "@ircserv ";
		const std::string out		= prefix + "PRIVMSG " + target + " :" + text + "\r\n";
		broadcast(*ch, out, &sender);
		return;
	}

	// // Direct message (nick)
	Client *destination = getClientByNick(target);
	if (!destination){
		sendError(sender, 401, target + " :No such nick/channel");
		return ;
	}

	const std::string out = prefix + "PRIVMSG " + target + " :" + text + "\r\n";
	destination->queue(out);

	// //TEMP FOR DEBUGGING
	// std::cerr << "PRIVMSG target=[" << target << "] map_size=" << _clients_by_nick.size() << "\n";
}

void Server::cmdMODE(Client& client, const Command& cmd) {
	if (cmd.params.empty()) {
		sendError(client, 461, "MODE :Not enough parameters");
		return;
	}

	const std::string& target = cmd.params[0];

	// MODE <nick>
	if (target == client.nick()) {
		// Query or set own modes → just say "+i"
		sendNumeric(client, 221, "+i");
		return;
	}

	// MODE #channel
	if (!target.empty() && target[0] == '#') {
		return;
	}

	sendError(client, 502, ":Cannot change mode for other users");
}



//TODO------------currently not used anywhere

// Optionally queue something back (usually not necessary)
// c.queue("ERROR :Closing Link"); // optional

// Mark client for disconnection.
// We cannot safely erase from poll_fds inside cmdQUIT without having access to poll_fds.
// So we use a simple approach: close the fd here; the loop will notice and remove it.
//close(c.fd());

// ------------------------------------------------------------ connection helpers
void  Server::disconnectClient(int fd, std::vector<pollfd>& poll_fds, size_t& i){
	ClientsMapFd::iterator it = _clients_by_fd.find(fd);
	std::cout << "Client disconnected fd="
						<< fd << "\n";

	if (it == _clients_by_fd.end()){
		// fd not found; still remove poll entry if you want, but usually shouldn't happen
		poll_fds.erase(poll_fds.begin() + i );
		--i;
		return;
	}

	Client* client = it->second;

	// remove nick mapping if present
	if (!client->nick().empty()){
		ClientsMapNick::iterator nit = _clients_by_nick.find(client->nick());
		if (nit != _clients_by_nick.end() && nit->second == client)
			_clients_by_nick.erase(nit);
	}

	const char* msg = "ERROR :Closing Link\r\n";
	(void)send(fd, msg, strlen(msg), MSG_NOSIGNAL);
	//send(fd, msg, strlen(msg), MSG_NOSIGNAL);
	shutdown(fd, SHUT_RDWR);
	// (Later) remove from channels, broadcast QUIT, etc.

	close(fd);
	delete client;
	_clients_by_fd.erase(it);

	//remove from poll list
	poll_fds.erase(poll_fds.begin() + i);
	--i;
}

// -------------------------------------- INIT ----------------------------------
Server::Server(int port, const std::string& password)
										: _port(port),
										  _password(password),
										  _listen_socket_fd(-1) {}

// ----------------------------- DESTRUCTOR ------------------------------
Server::~Server(){
	for (ClientsMapFd::iterator it = _clients_by_fd.begin();
		it != _clients_by_fd.end(); ++it) {
				close(it->first);
				delete it->second;
	}
	close(_listen_socket_fd);
}

// ------------------------------------- GETTERS --------------------------------
Client*	Server::getClientByFd(	int	fd	){
	ClientsMapFd::iterator it = _clients_by_fd.find(fd);
	if (it == _clients_by_fd.end())
		return nullptr;
	return it->second;
}

Client* Server::getClientByNick(const std::string& nick){
	ClientsMapNick::iterator it = _clients_by_nick.find(nick);
	if (it == _clients_by_nick.end())
		return nullptr;
	return it->second;
}

Channel* Server::getChannel(const std::string& name){
	ChannelMap::iterator it = _channels.find(name);
	if (it == _channels.end())
		return nullptr;
	return &it->second;
}

Channel& Server::getOrCreateChannel(const std::string& name){
	ChannelMap::iterator it = _channels.find(name);
	if (it != _channels.end())
		return (it->second);

	Channel	ch(name);
	_channels.insert({name, ch});
	it = _channels.find(name);
	return it->second;
}


// -------------------------- SERVER LOGIC -------------------------
bool	Server::refreshPollEvents(std::vector<pollfd>& poll_fds){
	bool any_pending = false;

	for (size_t j = 1; j < poll_fds.size(); ++j){
		ClientsMapFd::iterator it = _clients_by_fd.find(poll_fds[j].fd);
		if (it == _clients_by_fd.end())
			continue;

		Client* c = it->second;
		if (c->hasPendingOutput()){
			poll_fds[j].events = POLLIN | POLLOUT;
			any_pending = true;
		}
		else	{
			poll_fds[j].events = POLLIN;
		}
	}
	return any_pending;;
}


void Server::run(){
	/* Create listening socket */
	_listen_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_listen_socket_fd < 0){
		throw std::runtime_error("socket failed");
	}

	/* Set socket options */
	int yes = 1;
	if (setsockopt(_listen_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes))){
		close(_listen_socket_fd);
		throw std::runtime_error("setting socket options failed");
	}

	/* Building an address struct: what IP + port do we listen on? */
	sockaddr_in	addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family			= AF_INET;
	addr.sin_addr.s_addr	= INADDR_ANY;
	addr.sin_port			=htons(_port);

	/* Attach socket to that address (bind) */
	if (bind(_listen_socket_fd, (sockaddr*)&addr, sizeof(addr)) < 0){
		close(_listen_socket_fd);
		throw std::runtime_error("socket bind failed");
	}

	/* Make the listening socket non-blocking */
	if (fcntl(_listen_socket_fd, F_SETFL, O_NONBLOCK)){
		close(_listen_socket_fd);
		throw std::runtime_error("fcntl failed");
	}

	/* Put socket in listening mode */
	if (listen(_listen_socket_fd, SOMAXCONN) < 0){
		close(_listen_socket_fd);
		throw std::runtime_error("socket listen failed");
	}

	/* Create a list of fds we want to monitor
		pollfd is a struct which has three important fields:
			1. fd <= which socket to watch
			2. events <= what you care about
			3. revents <= what happened (filled by poll())
	   We put the listening socket at index 0, watching POLLIN */
	std::vector<pollfd> poll_fds;
	poll_fds.push_back(pollfd{_listen_socket_fd, POLLIN, 0});

	std::cout << "Server listening on port "
			<< _port
			<< " ...\n";

	// ------------------------- MAIN LOOP -------------------------
	while (check_signals()){
		/* POLL: poll(...) blocks the event loop(this is OK!) until
		something happens on any watched fd. Parameters given:
			1. poll.fds.data() gives poll the underlying array
			2. poll_fds.size() how many?
			3. -1 indicates "wait indefinitely"
		When poll() returns, it has filled in revents for each entry
		describing what happened. */

		//if (poll(poll_fds.data(), poll_fds.size(), -1) < 0){
		//	if (!check_signals())
		//		break;
		//	throw std::runtime_error("poll failed");
		//}

		// new version that should not stall
		// NEW: ensure POLLOUT is enabled for anyone with queued output
		bool any_pending = refreshPollEvents(poll_fds);

		// NEW: if we have pending output, don't sleep in poll()
		int	timeout_ms = any_pending ? 0 : -1;

		if (poll(poll_fds.data(), poll_fds.size(), timeout_ms) < 0){
			if (!check_signals())
				break;
			throw std::runtime_error("poll failed");
		}


		// ----------------------- ACCEPT ----------------------
		if (poll_fds[0].revents & POLLIN){
			while (true){
				int client_fd = accept(_listen_socket_fd, NULL, NULL);
				if (client_fd < 0) {
					break;
				}
				if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0){
					close(client_fd);
					continue;
				}
				_clients_by_fd[client_fd] = new Client(client_fd);
				poll_fds.push_back(pollfd{client_fd, POLLIN, 0});

				std::cout << "Client connected fd="
						<< client_fd << "\n";
			}
			continue;
		}

		/* Handling events: accept/read/write
			Check: Did poll say the listening socket is readable?
				if yes, then a connection is waiting in the kernel's queue.
			accept() creates a new socket(client_fd) dedicated to that client.
			set the client socket non_blocking (fcntl)
			Create a new Client object and store it in the data.
			Add the client socket into poll_fds so poll can watch it.
			After this, server is watching:
				- the listen socket(index 0)
				- each connected client (index 1..N) */
		for (size_t i = 1; i < poll_fds.size(); i++){
			if (!check_signals())
				break;
			pollfd& poll_fd = poll_fds[i];

			ClientsMapFd::iterator it = _clients_by_fd.find(poll_fd.fd);
			if (it == _clients_by_fd.end()){
				poll_fds.erase(poll_fds.begin() + i);
				--i;
				continue;
			}
			Client* client = it->second;

			// If the fd is in a bad state, disconnect immediately
			if (poll_fd.revents & (POLLHUP | POLLERR | POLLNVAL)){
				if (poll_fds[i].fd == _listen_socket_fd){
					if (!check_signals())
						break;
					throw std::runtime_error("listening socket failed in main loop");
				}
				// TEMP DEBUG REVENTS
				std::cerr
						<< "Disconnecting fd=" 
						<< poll_fd.fd
						<< " due to revents=" 
						<< poll_fd.revents 
						<< "\n";

				disconnectClient(poll_fd.fd, poll_fds, i);
				continue;
			}

			bool should_disconnect = false;

			/* Handle readable sockets */
			if (poll_fd.revents & POLLIN){
				// ------------------- READ --------------------
				char  buf[4096];
				ssize_t n = recv(  poll_fd.fd, buf, sizeof(  buf ), 0);

				//Disconnect handling
				//a.  n == 0 → client closed connection
				//b.  n < 0 → error
				//1.  But because you’re non-blocking, n < 0
				//    can be “try again later” (EAGAIN).
				//    So you’ll refine this later.

				if  ( n == 0  ) {
					// peer closed connection
					disconnectClient(poll_fd.fd, poll_fds, i);
					continue;
				}

				if (n < 0){
					// TEMP DEBUG RECV
					std::cerr 
							<< "Disconnecting fd=" 
							<< poll_fd.fd 
							<< " because recv==0\n";
					// non-blocking "no data right now" is NOT a disconnect
					//if (errno != EAGAIN && errno != EWOULDBLOCK){} // ERRNO IS FORBIDDEN IN THIS PROJECT
					// disconnectClient(poll_fd.fd, poll_fds, i);
					continue;
				}
				//Buffer the bytes and extract complete CRLF lines
				//
				//1.  You append raw bytes into the client’s input buffer.
				//2.  popLine() looks for \r\n and extracts complete lines.
				//3.  For each complete line you got, you queue a response.
				//so..:
				//1.  recv() gives you bytes
				//2.  your buffer “accumulates” bytes
				//3.  popLine() turns bytes into messages
				//4.  This is how you handle partial receives correctly.

				client->inbuf().append( buf, n );
				//TEMP DEBUG
				//std::cout << "Raw from irssi: " << client->_in << "\n";

				std::string line;
				while ( client->popLine(  line  ) ) {
					Command cmd = parseCommand(line);
					std::cerr << cmd.name;
					for (auto x : cmd.params){
						std::cerr << x << " ";
					}
					std::cerr << "\n";
					if (cmd.name == "QUIT"){
						should_disconnect = true;
						break;
					}
					handleCommand(*client, cmd);
					// handleEverything(*this, client, line);

				}
			}
			// -------------------------------- QUIT CLIENT --------------------------------
			if (should_disconnect){
				disconnectClient(poll_fd.fd, poll_fds, i);
				continue;
			}
			// ----------------------------------- WRITE -----------------------------------
			//Writing queued output (POLLOUT)
			//
			//1.  Meaning:
			//            a.  the socket is writable right now
			//            b.  so you flush some of the output buffer
			//2.  Important concept: send may send only part
			//3.  So you erase only the bytes actually sent.

			if (  (poll_fd.revents & POLLOUT) && client->hasPendingOutput()) {
				std::string& out  = client->outbuf();
				ssize_t sent      = send(poll_fd.fd, out.data(), out.size(), 0);

				if (  sent > 0  ) {
					std::cout << "sent from server output: " << out << std::endl; //TEMP temp call for debugging
					out.erase(  0, sent );
				}
				else if (sent < 0){
				// If it's not a "try again later", treat as disconnect
				//if (errno != EAGAIN && errno != EWOULDBLOCK){} // ERNNO IS FORBIDDEN
					//disconnectClient(poll_fd.fd, poll_fds, i);//this might be causing the premature kick when using invite
					continue;
				}
			}

			// ----------------------------------------------------- update POLLOUT interest

			//Decide whether to watch for POLLOUT
			//
			//1.  Always watch for readable (POLLIN) so you can receive commands anytime
			//2.  Only watch for writable (POLLOUT) when you actually have bytes to send
			//3.  If you always watch POLLOUT, you can cause the event loop to wake constantly.

			if  ( client->hasPendingOutput()  ) {
				poll_fd.events = POLLIN | POLLOUT;
			} else  {
				poll_fd.events = POLLIN;
			}
		}
	}
}

// ------------------------------------------------------------- routing helpers

//static  std::string itos(int n){
//  return std::to_string(n);
//}

static std::string pad3(int code){
	std::string out = std::to_string(code);
	while (out.size() < 3)
		out = "0" + out;
	return out;
}

void Server::sendNumeric(  Client& client, int code, const std::string& text){
	std::string nick = client.nick().empty() ? "*" : client.nick();
	
	//TEMP DEBUG FOR NUMERIC
	std::string line = ":ircserv " + pad3(code) + " " + nick + " " + text;
	std::cerr << "SENT LINE: [" << line << "]\n";

	client.queue(":ircserv " + pad3(code) + " " + nick + " " + text);
}

void Server::sendError( Client& client, int code, const std::string& text  ){
	sendNumeric(client, code, text);
}

// ------------------------------------------------------------- command handler
void Server::handleCommand( Client& client, const Command& cmd ){

	// ------------------------------------------------------------ for registration
	if (  cmd.name == "PING"  ){
		// token is usually in params[0]
		if ( !cmd.params.empty())
			client.queue("PONG :" + cmd.params[0]  );
		else
			client.queue("PONG"); //fall_back
		return;
	}

	//if (  cmd.name == "CAP" && !client.isWelcomed()){
	if (  cmd.name == "CAP"){
		return;
		}

	//if (  cmd.name == "PASS" && !client.isWelcomed()){
	if (  cmd.name == "PASS"){
		cmdPASS       (client, cmd  );
		return;
	}

	//if (  cmd.name == "NICK" && !client.isWelcomed()){
	if (  cmd.name == "NICK"){
		cmdNICK       ( client, cmd );
		return;
	}

	//if (  cmd.name == "USER" && !client.isWelcomed()){
	if (  cmd.name == "USER"){
		cmdUSER       ( client, cmd );
		if (client.isRegistered() && !client.isWelcomed()){
			maybeWelcome  ( client      );
			client.setWelcomed();
		}
		return;
	}

	// Before registration, ignore other commands (or reply 451 later)
	if (!client.isRegistered()){
		sendError(client, 451, ":You have not registered");
		return;
	}
	if ( cmd.name == "JOIN") {
		cmdJOIN ( client, cmd );
		std::cout << "JOIN called.. \n";
		return;
	}

	if (cmd.name == "MODE") {
		cmdMODE(client, cmd);
		return;
	}

	if (cmd.name == "PART"){
		const char* msg = "ERROR :Closing Link\r\n";
		(void)send(client.fd(), msg, strlen(msg), MSG_NOSIGNAL);
		//send(fd, msg, strlen(msg), MSG_NOSIGNAL);
		shutdown(client.fd(), SHUT_RDWR);
		return ;
	}

	// if ( cmd.name == "KICK" ) {
	// 	cmdKICK( Client& c, const Command& cmd );
	// 	return;
	// }
	// else{
	// 	sendError(client, 421, cmd.name + " :Unknown command");
	// 	return;
	// }

	// ---------------------------------------------------------- after registration
	if (cmd.name == "PRIVMSG"){
		cmdPRIVMSG(client, cmd);
		return;
	}

	else if (cmd.name == "INVITE"){
		cmdINVITE(client, cmd);
		return;
	}

	//IF COMMAND IS INVALID
	sendError(client, 421, cmd.name + " :Unknown command");
	// Later: JOIN, PRIVMSG, etc.

}

// --------------------------------------------------------- registration helpers
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
