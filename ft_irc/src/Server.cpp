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

/*
 * @breif main server logic 
 *
 * 1.Set up the listening socket (one-time)
 * 2.Wait for events (in a loop)
 * 3.React to events (accept / read / write / cleanup)
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
  //1.  fd = which file descriptor
  //2.  events = what we’re interested in
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
  //1.  fd = which socket to watch
  //2.  events = what you care about
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
    
    //Loop over client sockets and handle read/write
    //
    //1.  You start at 1 because 0 is the listening socket.
    //2.  For each client:
    //                    a.  poll_fd.fd is their socket fd
    //                    b.  you lookup the corresponding Client*

    for ( size_t i = 1; i < poll_fds.size(); i++) {
          pollfd& poll_fd = poll_fds[i];
          Client* client  = _clients_by_fd[poll_fd.fd];
  
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
              std::cout
                        << "LINE: ["
                        << line
                        << "]"
                        << std::endl;
              client->queue("ECHO :" + line);
            }
          } 
          // ----------------------------------------------------------------------- write

          //Writing queued output (POLLOUT)
          //
          //1.  Meaning:
          //            a.  the socket is writable right now
          //            b.  so you flush some of the output buffer
          //2.  Important concept: send may send only part
          //3.  So you erase only the bytes actually sent.

          if (  poll_fd.revents & POLLOUT) {
                std::string& out  = client->outbuf();
                ssize_t sent      = send(poll_fd.fd, out.data(), out.size(), 0);
                if (  sent > 0  ) {
                  out.erase(  0, sent );
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
