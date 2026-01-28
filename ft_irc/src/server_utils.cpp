#include "Server.hpp"

std::string Server::prefix(Client& client) {
	std::string out;
	out = ":" + client.nick() + "!" + client.nick() + "@ircserv ";
	return out;
}

void Server::handleCommand( Client& client, const Command& cmd ){
	// ------------------------------------------------------------ for registration
	if ( cmd.name == "PING" ){
		// token is usually in params[0]
		if ( !cmd.params.empty())
			client.queue("PONG :" + cmd.params[0]  );
		else
			client.queue("PONG"); //fall_back
		return;
	}

	if (  cmd.name == "CAP"){
		return;
		}

	if (  cmd.name == "PASS"){
		authPASS       (client, cmd  );
		if (client.isRegistered())
			return;
	}

	if (  cmd.name == "NICK"){
		authNICK       ( client, cmd );
		if (client.isRegistered())
			return;
	}

	if (  cmd.name == "USER"){
		authUSER       ( client, cmd );	
		if (client.isRegistered())
			return;
	}

	// Before registration, ignore other commands (or reply 451 later)
	if (!client.isRegistered()){

		client.tryCompleteRegistration();
		if (client.isRegistered()){
			maybeWelcome(	client	);
			client.setWelcomed();
		}
		return;
	}

	// ---------------------------------------------------------- after registration
	if ( cmd.name == "QUIT") {
		disconnectClient(client.fd(), poll_fds);
		return;
	}

	if ( cmd.name == "JOIN") {
		cmdJOIN ( client, cmd );
		return;
	}

	if (cmd.name == "MODE") {
		cmdMODE(client, cmd );
		return;
	}

	if (cmd.name == "TOPIC") {
		cmdTOPIC( client, cmd );
		return;
	}

	if (cmd.name == "PRIVMSG"){
		cmdPRIVMSG(client, cmd);
		return;
	}

	if (cmd.name == "INVITE"){
		cmdINVITE(client, cmd);
		return;
	}

	if (cmd.name == "PART"){
		cmdPART(client, cmd);
		return;
	}

	if ( cmd.name == "KICK" ) {
		cmdKICK(client, cmd);
		return;
	}

	//IF COMMAND IS INVALID
	sendError(client, 421, cmd.name + " :Unknown command");
}

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

void Server::broadcast(Channel& chan, std::string msg, const Client* sender) {
	for (auto chan_it = chan.members.begin(); chan_it != chan.members.end(); ) {
		Client* client = getClientByFd(*chan_it);
		if (!client) {
			chan_it = chan.members.erase(chan_it); // CLEAN DEAD FD
			continue;
		}
		if (sender && client->fd() == sender->fd()) {
			++chan_it;
			continue;
		}
		client->queue(msg);
		++chan_it;
	}
}

void Server::broadcastQuit(Client* client, const std::string& msg) {
	for (auto& [name, chan] : _channels) {
		if (chan.members.contains(client->fd())) {
			for (auto chan_it = chan.members.begin(); chan_it != chan.members.end(); ) {
				Client* target = getClientByFd(*chan_it);
				if (!target) {
					chan_it = chan.members.erase(chan_it); // CLEAN DEAD FD
					continue;
				}
				if (client && target->fd() == client->fd()) {
					++chan_it;
					continue;
				}
				(void)send((*target).fd(), msg.c_str(), msg.size(), MSG_NOSIGNAL);
				++chan_it;
			}
		}
	}
}

// -------------------------------------------------------------------- error handler
static std::string pad3(int code){
	std::string out = std::to_string(code);
	while (out.size() < 3)
		out = "0" + out;
	return out;
}

void Server::sendNumeric(  Client& client, int code, const std::string& text){
	std::string nick = client.nick().empty() ? "*" : client.nick();
	
	client.queue(":ircserv " + pad3(code) + " " + nick + " " + text);
}

void Server::sendError( Client& client, int code, const std::string& text  ){
	sendNumeric(client, code, text);
}

void  Server::signalShutdown(){
	while (!_clients_by_fd.empty()) {
		int fd = _clients_by_fd.begin()->first;
		std::string msg = prefix((*getClientByFd(fd))) + "QUIT :*** DISCONNECTED ***\r\n";
		(void)send(fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
		disconnectClient(fd, poll_fds);
	}
	if (_listen_socket_fd >= 0) {
		close(_listen_socket_fd);
		_listen_socket_fd = -1;
	}
}

void::Server::removeClientFromChannels(int fd){
	for (auto chan_it = _channels.begin(); chan_it != _channels.end();){
		Channel& chan = chan_it->second;

		chan.invites.erase(fd);
		chan.ops.erase(fd);
		chan.members.erase(fd);
		if (chan.members.empty()){
			chan_it = _channels.erase(chan_it);
		}
		else
			++chan_it;
	} 
}

void Server::disconnectClient(int fd, std::vector<pollfd>& poll_fds) {
	if (fd < 0) return;

	ClientsMapFd::iterator client_fd_it = _clients_by_fd.find(fd);
	if (client_fd_it == _clients_by_fd.end()) return;

	Client* client = client_fd_it->second;

	std::cout << "Client disconnected fd=" << fd << "\n";

	std::string quit_msg = ":" + client->nick() + "!" + client->user() +
						  "@ircserv QUIT :Client disconnected\r\n";
	broadcastQuit(client, quit_msg);

	removeClientFromChannels(fd);

	// remove nick mapping if present
	if (!client->nick().empty()){
		ClientsMapNick::iterator client_nick_it = _clients_by_nick.find(client->nick());
		if (client_nick_it != _clients_by_nick.end() && client_nick_it->second == client)
			_clients_by_nick.erase(client_nick_it);
	}

	if (fd >= 0) {
		const char* msg = "*** DISCONNECTED ***\r\n";
		(void)send(fd, msg, strlen(msg), MSG_NOSIGNAL);
		shutdown(fd, SHUT_RDWR);
		close(fd);
		fd = -1;
	}

	for (auto poll_it = poll_fds.begin(); poll_it != poll_fds.end(); ++poll_it) {
		if (poll_it->fd == fd) {
			poll_fds.erase(poll_it);
			break;
		}
	}

	delete client;
	_clients_by_fd.erase(client_fd_it);
}
