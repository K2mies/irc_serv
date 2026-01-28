/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jforsten <jforsten@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/18 10:22:17 by rhvidste          #+#    #+#             */
/*   Updated: 2026/01/20 20:48:50 by jforsten         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma   once
#include "Channel.hpp"
#include "Client.hpp"
#include "Command.hpp"
#include "Signals.hpp"

#include <iostream>
#include <stdlib.h>
#include <poll.h>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <cerrno>

// --------------------------------------------------------------------- typedef
typedef std::unordered_map<int, Client*>             ClientsMapFd;
typedef std::unordered_map<std::string, Client*>     ClientsMapNick;
typedef std::unordered_map<std::string, Channel>     ChannelMap;

class Server {
private:

	uint16_t          _port;
	std::string       _password;

	int               _listen_socket_fd;

	ClientsMapFd      _clients_by_fd;
	ClientsMapNick    _clients_by_nick;
	ChannelMap        _channels;

private:
	// ------------------- [ server_auth.cpp ] ------------------ authentication
	void  authPASS     ( Client& client, const Command& cmd );
	void  authNICK     ( Client& client, const Command& cmd );
	void  authUSER     ( Client& client, const Command& cmd );
	void  maybeWelcome ( Client& client );

	// ------------------- [ server_cmds.cpp ] ------------------------ commands
	void  cmdKICK      ( Client& client, const Command& cmd );
	void  cmdINVITE    ( Client& client, const Command& cmd );
	void  cmdTOPIC     ( Client& client, const Command& cmd );
	void  cmdMODE      ( Client& client, const Command& cmd );
	void  cmdJOIN      ( Client& client, const Command& cmd );
	void  cmdPRIVMSG   ( Client& client, const Command& cmd );
	void  cmdPART      ( Client& client, const Command& cmd );

	// ------------------- [ server_utils.cpp ] ----------------- error handler
	void  disconnectClient( int fd, std::vector<pollfd>& poll_fds );

public:
	std::string pref = ":ircserv ";
	std::vector<pollfd> poll_fds;

	// ------------------------------------------------------------------- init
	Server( int port, const std::string& password );
	~Server();

	// ---------------------------------------------------------------- getters
	Client  *getClientByFd      ( int fd );
	Client  *getClientByNick    ( const std::string &nick );
	Channel  *getChannel         ( const std::string &name );

	// ----------------------------------------------------------- server logic
	void run();

	// ------------------- [ server_utils.cpp ] ------------------ server utils
	std::string  prefix( Client& client );
	void  handleCommand( Client &client, const Command &cmd );
	bool  refreshPollEvents( std::vector<pollfd>& poll_fds );
	void  broadcast( Channel& ch, std::string msg, const Client* sender = 0 );
	void  broadcastQuit( Client* client, const std::string& msg );

	// ------------------- [ server_utils.cpp ] ----------------- error handler
	void  sendNumeric ( Client& client, int code, const std::string& text );
	void  sendError   ( Client& client, int code, const std::string& text );
	void  signalShutdown();
	void  removeClientFromChannels( int fd );
};
