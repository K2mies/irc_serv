/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhvidste <rhvidste@student.hive.email.com  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/13 13:01:36 by rhvidste          #+#    #+#             */
/*   Updated: 2026/01/13 14:55:32 by rhvidste         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "Server.hpp"
#include "Command.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cerrno>

// ------------------------------------------------------------ command handlers

void Server::cmdQUIT  ( Client& c, const Command& cmd ){
	(void)cmd;
	(void)c;

	//TODO------------currently not used anywhere

	// Optionally queue something back (usually not necessary)
	// c.queue("ERROR :Closing Link"); // optional

	// Mark client for disconnection.
	// We cannot safely erase from poll_fds inside cmdQUIT without having access to poll_fds.
	// So we use a simple approach: close the fd here; the loop will notice and remove it.
	//close(c.fd());
}

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

// ------------------------------------------------------------------------ init
Server::Server(int port, const std::string& password)
																				: _port(port),
																					_password(password),
																					_listen_socket_fd(-1) {}

/*
 * @breif main server logic 
 *
 * 1.Set up the listening socket (one-time)
 * 2.Wait for events (in a loop)
 * 3.React to events (accept / read / write / cleanup)
 *
 * current pipeline is:
 * recv()
 *   → inbuf
 *    → popLine
 *     → parseCommand
 *      → handleCommand
 *       → queue()
 */
void Server::run(){

	// --------------------------------------------------- create a listening socket
	
	//Create a listening socket
	//
	//1.  socket() asks the OS for a new socket.
	//2.  AF_INET = IPv4
	//3.  SOCK_STREAM = TCP
	//4.  0 = “default protocol” (TCP here)
	//5.  It returns an fd (file descriptor), stored in _listen_socket_fd.
	//6.  This fd is special: it’s the “front door” of your server. 
	//    You don’t use it to talk to clients, you use it to accept them.

	_listen_socket_fd = socket( AF_INET, SOCK_STREAM, 0  );

	if (  _listen_socket_fd < 0 ){
				perror( "socket"  );
				return ;
	}

	// set socket options so that:
	//
	// 1. Lets you bind() the same port again right after restarting
	// 2. Prevents “Address already in use” during development
	
	int yes = 1;
	setsockopt( _listen_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof( yes ) );

	//Build an address struct: what IP + port do we listen on?
	//
	//1.  sin_family = AF_INET → IPv4
	//2.  INADDR_ANY → bind to all interfaces
	//3.  htons(_port) converts your port number into network byte order
	//4.  This struct is just data. It doesn’t do anything until you call bind().

	sockaddr_in     addr;
	std::memset (&addr, 0, sizeof( addr  ) );
	addr.sin_family       = AF_INET;
	addr.sin_addr.s_addr  = INADDR_ANY;
	addr.sin_port         = htons(_port );
	
	//Attach socket to that address (bind)
	//
	//1.  This socket now owns port _port on this machine.

	if( bind    ( _listen_socket_fd, (  sockaddr* )&addr, sizeof( addr  ) ) < 0){
			perror  ( "bind"  );
			return ;
	}

	//Put it in listening mode.
	//Now the socket becomes a server socket and the kernel starts queueing incoming connection attempts.
	//
	//1.  Tells OS: “This socket is a server socket”
	//2.  The OS can now queue incoming connection attempts
	//3.  but you still haven’t accepted any clients

	if( listen  ( _listen_socket_fd, SOMAXCONN  ) < 0 ){
			perror  ( "listen"  );
			return ;
	}

	//Make the listening socket non-blocking
	//Non-blocking means:
	//
	//1.  calls like accept() won’t freeze your server
	//2.  nstead, they return quickly if there’s nothing to do

	fcntl(  _listen_socket_fd, F_SETFL, O_NONBLOCK  );

	std::cout
						<< "Server listening on port " 
						<< _port
						<< std::endl;

	// ------------------------------------------------------------------- poll loop
	
	//Create a list of fds we want to monitor
	//pollfd has three important fields:
	//
	//1.  fd      = which file descriptor
	//2.  events  = what we’re interested in
	//3.  revents = what actually happened (filled by poll())

	//POLLIN
	//
	//1.  POLLIN is an event flag from <poll.h>
	//2.  It means the fd is readable
	//3.  For:
	//        a:listening socket → accept()
	//        b:client socket → recv()
	//4.  poll() tells you when to try, not what to read

	//pollfd is a struct which has the interested
	//
	//1.  fd      = which socket to watch
	//2.  events  = what you care about
	//3.  revents = what happened (filled by poll)
	//4.  You put the listening socket at index 0, watching POLLIN.

	std::vector<pollfd> poll_fds;
	poll_fds.push_back  ( pollfd{  _listen_socket_fd, POLLIN, 0});
	
	//Now your server tracks:
	//
	//1.  a Client* for that fd
	//2.  a pollfd entry so poll tells you when it has data / can be written

	// ------------------------------------------------------------------- main loop
	while ( true  ) {

		//POLL:
		//
		//1.  poll(...) blocks your event loop (this is okay!) until something happens on any watched fd.
		//    a.poll_fds.data() gives poll the underlying array
		//    b.poll_fds.size() is how many
		//    c.-1 means “wait indefinitely”
		//2.  When poll() returns, it has filled in revents for each entry describing what happened.

		if  ( poll        (  poll_fds.data(), poll_fds.size(), -1  ) < 0 ){
					perror      ( "poll"  );
					break ;
		}
		// --------------------------------------------------------------- listen socket
		
		// Handling events: accept / read / write
		//
		//1.  You check: Did poll say the listening socket is readable?
		//2.  if yes, then a connection is waiting in the kernel’s queue.
		//3.  accept() creates a new socket (client_fd) dedicated to that client.
		//4.  You set the client socket non-blocking.
		//5.  You create a Client object and store it in your map.
		//6.  You add the client socket into poll_fds so poll can watch it.
		//7.  After this, your server is watching:
		//    a.  the listen socket (index 0)
		//    b.  each connected client (indices 1..N)

		if (  poll_fds[0].revents & POLLIN  ){
			while ( true  ) {
				int client_fd = accept( _listen_socket_fd, NULL, NULL );
				
				if ( client_fd < 0 ){
					if  (errno == EAGAIN || errno == EWOULDBLOCK  ) break;
					perror( "accept" );
					break;
				}

				fcntl(  client_fd, F_SETFL, O_NONBLOCK  );
				_clients_by_fd[client_fd] = new Client(client_fd  );
				poll_fds.push_back( pollfd{  client_fd, POLLIN, 0 } );
				 
				std::cout
									<< "Client connected fd=" 
									<< client_fd
									<< std::endl;
			}
		}
		// -------------------------------------------------------------- client sockets
		
		//Loop over client sockets and handle read/write
		//
		//1.  You start at 1 because 0 is the listening socket.
		//2.  For each client:
		//                    a.  poll_fd.fd is their socket fd
		//                    b.  you lookup the corresponding Client*

		for ( size_t i = 1; i < poll_fds.size(); i++) {

					//pollfd& poll_fd = poll_fds[i];
					//Client* client  = _clients_by_fd[poll_fd.fd];
						
					//Final form of the above two lines
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
						disconnectClient(poll_fd.fd, poll_fds, i);
						continue;
					}
					
					bool should_disconnect = false;

					// ------------------------------------------------------------------------ read
					//
					// Reading client data (POLLIN on a client fd)
					//
					// 1. Meaning:
					//            a.  poll said the client socket is readable
					//            b.  so you try to recv() bytes
					// 2. This reads some bytes, not “a full IRC message”.
					// 3. TCP is a stream, so you might get:
					//                                      a.  half a line
					//                                      b.  one line
					//                                      c.  multiple lines

					if  ( poll_fd.revents & POLLIN  ) {

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
							if (errno != EAGAIN && errno != EWOULDBLOCK){
								disconnectClient(poll_fd.fd, poll_fds, i);
							}
							continue;
						}

						//TEMP TEMPORARY RECV DEBUG-----------------------------------
						std::cerr << "recv n=" << n << " bytes: ";
						for (ssize_t k = 0; k < n; ++k) {
							unsigned char c = static_cast<unsigned char>(buf[k]);
							if (c == '\r') std::cerr << "\\r";
							else if (c == '\n') std::cerr << "\\n";
							else if (c >= 32 && c <= 126) std::cerr << buf[k];
							else std::cerr << "\\x" << std::hex << (int)c << std::dec;
						}
						std::cerr << std::endl;
						//TEMP END OF TEMPORARY RECF DEBUG------------------------------

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

						//TEMP TEMPORARY INBUF PRIEVIEV-----------------------------------------
						std::cerr << "inbuf preview: ";
						for (size_t k = 0; k < client->inbuf().size(); ++k) {
							unsigned char c = static_cast<unsigned char>(client->inbuf()[k]);
							if (c == '\r') std::cerr << "\\r";
							else if (c == '\n') std::cerr << "\\n";
							else if (c >= 32 && c <= 126) std::cerr << client->inbuf()[k];
							else std::cerr << ".";
						}
						std::cerr << std::endl;
						// TEMP END OF TEMPORARY INBUF PREVIEW----------------------------------
	
						std::string line;
						while ( client->popLine(  line  ) ) {
							std::cerr << "CMD: [" << line << "]" << std::endl;  //TEMP TEMPORARY DEBUG LINE.
							Command cmd = parseCommand(line);

							if (cmd.name == "QUIT"){
								should_disconnect = true;
								break;
							}
							handleCommand(*client, cmd);
						} 
					} 

					if (should_disconnect){
						disconnectClient(poll_fd.fd, poll_fds, i);
						continue;
					}
					// ----------------------------------------------------------------------- write

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
									if (errno != EAGAIN && errno != EWOULDBLOCK){
										disconnectClient(poll_fd.fd, poll_fds, i);
										continue;
									}
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

// --------------------------------------------------------------------- getters

// ------------------------------------------------------------- command handler
void Server::handleCommand(Client& client, const Command& cmd)
{
	if (cmd.name == "PASS") {
			if (cmd.params.empty())
				return;
			client.setPassOk(getPass() == cmd.params[0]);

			return;
	}
	if (cmd.name == "NICK") {
			if (cmd.params.empty())
					return; // error handling omitted

			client.setNick(cmd.params[0]);

			return;
	}

	if (cmd.name == "USER") {
			if (cmd.params.size() < 4)
					return;

			client.setUser(cmd.params[0]);

			client.tryCompleteRegistration();
			if (client.isRegistered())
				sendWelcome(client);
			return;
	}

	if (cmd.name == "PING") {
			if (!cmd.params.empty())
					client.queue("PONG :" + cmd.params[0]);
			else
					client.queue("PONG");
			return;
	}
}

void Server::sendWelcome(Client& client)
{
		std::string prefix = ":ircserv ";

		client.queue(prefix + "001 " + client.nick() +
				" :Welcome to the Internet Relay Network " +
				client.nick() + "!" + client.user() + "@localhost");

		client.queue(prefix + "002 " + client.nick() +
				" :Your host is irc.example.net, running version 1.0");

		client.queue(prefix + "003 " + client.nick() +
				" :This server was created today");

		client.queue(prefix + "004 " + client.nick() +
				" irc.example.net 127.0.0.1");

		// MOTD start
		client.queue(prefix + "375 " + client.nick() +
				" :- Message of the Day -");

		client.queue(prefix + "372 " + client.nick() +
				" :- Welcome to my IRC server!");

		client.queue(prefix + "376 " + client.nick() +
				" :End of /MOTD command");
}

