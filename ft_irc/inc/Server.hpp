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
#include <iostream>
#include <unordered_map>
#include <vector>

typedef std::unordered_map<int, Client*>             ClientsMapFd;
typedef std::unordered_map<std::string, Client*>     ClientsMapNick;
typedef std::unordered_map<std::string, Channel*>    ChannelMap;

class Server {
private:

  uint32_t          _port;
  std::string       _password;

  int               _listen_socket_fd;
  ClientsMapFd      _clients_by_fd;
  ClientsMapNick    _clients_by_nick;
  ChannelMap        _channels;

private:
  // ------------------------------------------------------------ command handlers
  void  cmdPASS(    Client& c, const Command& cmd );
  void  cmdNICK(    Client& c, const Command& cmd );
  void  cmdUSER(    Client& c, const Command& cmd );
  void  cmdJOIN(    Client& c, const Command& cmd );
  void  cmdQUIT(    Client& c, const Command& cmd );
  void  cmdPRIVMSG( Client& c, const Command& cmd );

public:
  Server(int port, const std::string& password);

  Client  *getClientByFd(       int fd );
  Client  *getClientByNick(     const std::string &nick );
  Channel *getChannel(          const std::string &name );

  Channel& getOrCreateChannel(  const std::string& name );

  void    handleCommand(        Client &client, const Command &cmd );

  // ------------------------------------------------------------- routing helpers
  void  sendNumeric(  Client& c, int code, const std::string& text );
  void  sendError(    Client& c, int code, const std::string& text );
};
