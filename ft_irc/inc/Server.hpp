/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhvidste <rhvidste@student.hive.email.com  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/18 10:22:17 by rhvidste          #+#    #+#             */
/*   Updated: 2025/12/18 11:05:49 by rhvidste         ###   ########.fr       */
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
typedef std::unordered_map<int, Client*>              ClientsMapFd;
typedef std::unordered_map<std::string, Client*>      ClientsMapNick;
typedef std::unordered_map<std::string, Channel>      ChannelMap;

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

  // ------------------------------------------------------------ connection helpers
  void  disconnectClient(int fd, std::vector<pollfd>& poll_fds, size_t& i);
  void  maybeWelcome(Client& client);

public:

  // ------------------------------------------------------------------------ init
  Server(int port, const std::string& password);
  ~Server();

  // ----------------------------------------------------------------- server logic
  bool refreshPollEvents(std::vector<pollfd>& poll_fds);

  void run();

  // --------------------------------------------------------------------- getters
  Client  *getClientByFd      ( int fd );
  Client  *getClientByNick    ( const std::string &nick );
  Channel *getChannel         ( const std::string &name );

  Channel& getOrCreateChannel ( const std::string& name );
  std::string getPass() { return _password; };

  // ------------------------------------------------------------- command handler
  void    handleCommand       ( Client &client, const Command &cmd );

  // ------------------------------------------------------------- routing helpers
  void  sendNumeric (  Client& client, int code, const std::string& text );
  void  sendError   (  Client& client, int code, const std::string& text );
};
