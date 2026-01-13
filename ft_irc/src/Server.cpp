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

#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

// ------------------------------------------------------------------------ init
Server::Server(int port, const std::string& password)
                                        : _port(port),
                                          _password(password),
                                          _listen_socket_fd(-1) {}

void Server::run(){

  // --------------------------------------------------- create a listening socket
  _listen_socket_fd = socket( AF_INET, SOCK_STREAM, 0  );

  if (  _listen_socket_fd < 0 ){
        perror( "socket"  );
        return ;
  }

  int yes = 1;
  setsockopt( _listen_socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof( yes ) );

  sockaddr_in     addr;
  std::memset (&addr, 0, sizeof( addr  ) );
  addr.sin_family       = AF_INET;
  addr.sin_addr.s_addr  = INADDR_ANY;
  addr.sin_port         = htons(_port );

  if( bind( _listen_socket_fd, (  sockaddr* )&addr, sizeof( addr  ) ) < 0){
      perror( "bind"  );
      return ;
  }

  if( listen( _listen_socket_fd, SOMAXCONN  ) < 0 ){
      perror( "listen"  );
      return ;
  }

  fcntl(  _listen_socket_fd, F_SETFL, O_NONBLOCK  );

  std::cout
            << "Server listening on port " 
            << _port
            << std::endl;

  // ------------------------------------------------------------------- poll loop
  std::vector<pollfd> poll_fds;
  poll_fds.push_back( pollfd{  _listen_socket_fd, POLLIN, 0});

  while (true){
    if (  poll(  poll_fds.data(), poll_fds.size(), -1  ) < 0 ){
          perror( "poll"  );
          break ;
    }
    // --------------------------------------------------------------- listen socket
    if (  poll_fds[0].revents & POLLIN  ){

      int client_fd = accept( _listen_socket_fd, NULL, NULL );

      if( client_fd >= 0) {

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
    for ( size_t i = 1; i < poll_fds.size(); i++) {
          pollfd& poll_fd = poll_fds[i];
          Client* client  = _clients_by_fd[poll_fd.fd];
  
          // ------------------------------------------------------------------------ read
          if  ( poll_fd.revents & POLLIN  ) {
            char  buf[4096];
            size_t n = recv(  poll_fd.fd, buf, sizeof(  buf ), 0);
  
            if  ( n <= 0  ) {
  
              std::cout
                        << "Client disconnected fd="
                        << poll_fd.fd
                        << std::endl;
  
              close(  poll_fd.fd  );
              delete  client;
              _clients_by_fd.erase( poll_fd.fd );
              poll_fds.erase(poll_fds.begin() + i );
              
              --i;
              continue;
            }
  
            client->inbuf().append( buf, n );
  
            std::string line;
            while ( client->popLine(  line  ) ) {
              std::cout
                        << "LINE: ["
                        << line
                        << "]"
                        << std::endl;
              client->queue("ECHO :" + line);
            }
          } 
          // ----------------------------------------------------------------------- write
          if (  poll_fd.revents & POLLOUT) {
                std::string& out  = client->outbuf();
                ssize_t sent      = send(poll_fd.fd, out.data(), out.size(), 0);
                if (  sent > 0  ) {
                  out.erase(  0, sent );
                }
          }
  
          // ----------------------------------------------------- update POLLOUT interest
          if  ( client->hasPendingOutput()  ) {
            poll_fd.events = POLLIN | POLLOUT;
          } else  {
            poll_fd.events = POLLIN;
          }
    }
  }
}
