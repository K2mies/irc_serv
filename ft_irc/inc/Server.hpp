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
#include <poll.h>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <string>

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
  // ------------------------------------------------------------ command handlers
  void  cmdPASS     ( Client& client, const Command& cmd );
  void  cmdNICK     ( Client& client, const Command& cmd );
  void  cmdUSER     ( Client& client, const Command& cmd );
  void  cmdJOIN     ( Client& client, const Command& cmd );
  void  cmdQUIT     ( Client& client, const Command& cmd );
  void  cmdPRIVMSG  ( Client& client, const Command& cmd );
  void  cmdMODE     ( Client& client, const Command& cmd );
  void  cmdTOPIC    ( Client& client, const Command& cmd );
  void	cmdINVITE   ( Client& client, const Command& cmd );
//   void  cmdKICK     ( Client& c, const Command& cmd );

  // ------------------------------------------------------------ connection helpers
  void  disconnectClient(int fd, std::vector<pollfd>& poll_fds);
  void  maybeWelcome(Client& client);

public:
	std::string pref = ":ircserv ";
  // ------------------------------------------------------------------------ init
  Server(int port, const std::string& password);
  ~Server();

  // ------------------------------------------------------------------------ polls
  std::vector<pollfd> poll_fds;

  // ----------------------------------------------------------------- server logic
  bool refreshPollEvents(std::vector<pollfd>& poll_fds);
  void broadcast(Channel& ch, std::string msg, const Client* sender = 0 );

  void run();

  // --------------------------------------------------------------------- getters
  Client  *getClientByFd      ( int fd );
  Client  *getClientByNick    ( const std::string &nick );
  Channel *getChannel         ( const std::string &name );
  
  std::string prefix(Client& client);

  Channel& getOrCreateChannel ( const std::string& name );
  std::string getPass() { return _password; };

  // ------------------------------------------------------------- command handler
  void    handleCommand       ( Client &client, const Command &cmd );

  // ------------------------------------------------------------- routing helpers
  void  sendNumeric (  Client& client, int code, const std::string& text );
  void  sendError   (  Client& client, int code, const std::string& text );

  // ------------------------------------------------------------- shutdown
  void  signalShutdown();
  void  removeClientFromChannels(int fd);
};
