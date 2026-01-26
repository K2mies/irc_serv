#include "Server.hpp"

// ------------------------------------------------------------------------ init
Server::Server(int port, const std::string& password)
										: _port(port),
										  _password(password),
										  _listen_socket_fd(-1) {}

Server::~Server(){
	for (ClientsMapFd::iterator it = _clients_by_fd.begin();
		it != _clients_by_fd.end(); ++it) {
				close(it->first);
				delete it->second;
	}
	if (_listen_socket_fd >= 0)
		close(_listen_socket_fd);
}

// --------------------------------------------------------------------- getters
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

// ----------------------------------------------------------------- server logic
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

				disconnectClient(poll_fd.fd, poll_fds);
				--i;
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
					disconnectClient(poll_fd.fd, poll_fds);
					--i;
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
				disconnectClient(poll_fd.fd, poll_fds);
				--i;
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
