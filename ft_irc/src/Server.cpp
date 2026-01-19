#include "Server.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>

// ------------------------------------------------------------ command handlers

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

// ---------------------------------------------------------------- command NICK
void Server::cmdNICK(Client& client, const Command& cmd){
	if (cmd.params.empty()){
		sendError(client, 431, ":No nickname given");
		return ;
	}

	const std::string& newNick = cmd.params[0];
	if (  newNick.empty())  {
		sendError(cleint, 431, ":No nickname given");
		return ;
	}

	// Nick in use by someone else?
	ClientsMapNick::iterator it = _clients_by_nick.find(newNick);
	if (it != _clients_by_nick.end() && it->second != &client){
		sendError(client, 433, newNick + " :Nickname is already in use");
		return ;
	}

	// Remove old nick mapping if present
	if (!c.nick().empty())  {
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

	// (Later) remove from channels, broadcast QUIT, etc.

	close(fd);
	delete client;
	_clients_by_fd.erase(it);

	//remove from poll list
	poll_fds.erase(poll_fds.begin() + i);
	--i;
	}

// ----------------------------- INIT ------------------------------
Server::Server(int port, const std::string& password)
										: _port(port),
										  _password(password),
										  _listen_socket_fd(-1) {}

// ------------------------- SERVER LOGIC --------------------------

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
	while (true){
		/* POLL: poll(...) blocks the event loop(this is OK!) until
		something happens on any watched fd. Parameters given:
			1. poll.fds.data() gives poll the underlying array
			2. poll_fds.size() how many?
			3. -1 indicates "wait indefinitely"
		When poll() returns, it has filled in revents for each entry
		describing what happened. */
		if (poll(poll_fds.data(), poll_fds.size(), -1) < 0){
			close(_listen_socket_fd);
			throw std::runtime_error("poll failed");
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
		for (size_t i = 0; i < poll_fds.size(); ++i){
			/* Handle fatal socket events */
			if (poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)){
				if (poll_fds[i].fd == _listen_socket_fd)
					throw std::runtime_error("listening socket failed in main loop");
				disconnectClient(poll_fds[i].fd, poll_fds, i);
				continue;
			}

			// ----------------------- ACCEPT ----------------------
			if (poll_fds[i].revents & POLLIN && poll_fds[i].fd == _listen_socket_fd){
				while (true){
					int client_fd = accept(_listen_socket_fd, NULL, NULL);
					if (client_fd < 0) {
						std::cerr << "client_fd accept failed";
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

			pollfd& poll_fd = poll_fds[i];

			ClientsMapFd::iterator it = _clients_by_fd.find(poll_fd.fd);
			if (it == _clients_by_fd.end()){
				poll_fds.erase(poll_fds.begin() + i);
				--i;
				continue;
			}
			Client* client = it->second;

			bool should_disconnect = false;

			/* Handle readable sockets */
			if (poll_fds[i].revents & POLLIN){
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

				std::string line;
				while ( client->popLine(  line  ) ) {
					Command cmd = parseCommand(line);
					std::cerr << cmd.name << "\n";
					if (cmd.name == "QUIT"){
						should_disconnect = true;
						break;
					}
					handleCommand(*client, cmd);
				}
			}
			// --------------------- QUIT CLIENT -------------------
			if (should_disconnect){
				disconnectClient(poll_fd.fd, poll_fds, i);
				continue;
			}
			// ---------------------- WRITE ------------------------
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
					// std::cout << "sent from server output: " << out << std::endl; //TEMP temp call for debugging
					out.erase(  0, sent );
				}
				else if (sent < 0){
				// If it's not a "try again later", treat as disconnect
				//if (errno != EAGAIN && errno != EWOULDBLOCK){} // ERNNO IS FORBIDDEN
					disconnectClient(poll_fd.fd, poll_fds, i);
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
	std::string s = std::to_string(code);
	while (s.size() < 3)
		s = "0" + s;
	return s;
}

void Server::sendNumeric(  Client& client, int code, const std::string& text){
	std::string nick = client.nick().empty() ? "*" : client.nick();
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

	if (  cmd.name == "CAP" && !client.isWelcomed()){
		return;
		}

	if (  cmd.name == "PASS" && !client.isWelcomed()){
		cmdPASS       (client, cmd  );
		return;
	}

	if (  cmd.name == "NICK" && !client.isWelcomed()){
		cmdNICK       ( client, cmd );
		return;
	}

	if (  cmd.name == "USER" && !client.isWelcomed()){
		cmdUSER       ( client, cmd );
		if (client.isRegistered()){
			if (!client.isWelcomed()){
				maybeWelcome  ( client      );
				client.setWelcomed();
			}
		}
		return;
	}

	// Before registration, ignore other commands (or reply 451 later)
	if (!client.isRegistered()){
		sendError(client, 451, ":You have not registered");
		return;
	}
	//else{
	//	sendError(client, 421, cmd.name + " :Unknown command");
	//	return;
	//}

	// ---------------------------------------------------------- after registration


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
