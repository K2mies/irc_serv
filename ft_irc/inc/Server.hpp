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

#include "Channel.hpp"
#include "Client.hpp"
#include "Command.hpp"
#include <iostream>
#include <unordered_map>

typedef std::unordered_map<int, Client>           ClientMap;
typedef std::unordered_map<std::string, Channel>  ChannelMap;

class Server {
private:

  int         _listen_socket_fd;
  ClientMap   _clients;
  ChannelMap  _channels;
  uint32_t    _port;

public:

  Server();
  Server(const Server &);
  ~Server() = default;

  Server &operator=(Server const &) = delete;

  Client  *getClientByFd(int fd);
  Client  *getClientByNick(const std::string &nick);
  Channel *getChannel(const std::string &name);

  void    handleCommand(Client &client, const Command &cmd);
};
