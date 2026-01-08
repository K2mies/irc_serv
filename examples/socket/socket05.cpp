#include <arpa/inet.h> // For htons(), inet_addr()
#include <cstring>     // For memset()
#include <filesystem>
#include <iostream>
#include <netinet/in.h> // For sockaddr_in
#include <stdexcept>
#include <string>
#include <sys/socket.h> // For socket(), bind(), listen(), accept()
#include <unistd.h>     // For close()
#include <thread>
#include <vector>

// A standard, high-numbered port to use for testing
const int PORT = 8080;

// Use the loopback address (127.0.0.1) for local testing
const std::string IP_ADDRESS = "0.0.0.0";

// Maximum number of pending connections the queue can hold
const int BACKLOG = 10;

//void handle_client(int new_socket){
//  char buffer[1024];
//  std::string client_nick = "guest";
//  bool is_registered = false; //Track if we've welcomed them
//
//  const char *hello = ":server NOTICE * :Hello! I am your c++ server.\r\n";
//  send(new_socket, hello, strlen(hello), 0);
//
//  while (true){
//    memset(buffer, 0, sizeof(buffer));
//    int bytes_recieved = recv(new_socket, buffer, sizeof(buffer) - 1, 0);
//
//    if (bytes_recieved <= 0){
//      if (bytes_recieved == 0){
//        std::cout << "\nSTATUS: Client disconnected gracefully." << std::endl;
//      }
//      else{
//        std::cerr << "\nERROR: Recv failed" << std::endl;
//      }
//      break;
//    }
//
//    std::string message(buffer);
//    std::cout << "RECIEVED: " << message;
//
//    // 1. Capture Nickname
//    if (message.find("NICK") != std::string::npos) {
//      size_t start = message.find("NICK") + 5;
//      size_t end = message.find_first_of("\r\n", start);
//      client_nick = message.substr(start, end - start);
//    }
//
//    // 2. Capture USER command and send Welcome
//    // Most clients send NICK and USER together. 
//    // Once we see USER, we send the 001 "Welcome" numeric.
//    if (!is_registered && (message.find("USER") != std::string::npos)) {
//        std::string rpl_001 = ":localhost 001 " + client_nick + " :Welcome to the IRC Network, " + client_nick + "!\r\n";
//        send(new_socket, rpl_001.c_str(), rpl_001.length(), 0);
//        
//        // Also send a 'MOTD' (Message of the day) to make it feel official
//        std::string motd = ":localhost 372 " + client_nick + " :- You are now fully connected! -\r\n";
//        std::string motd_end = ":localhost 376 " + client_nick + " :End of /MOTD command.\r\n";
//        send(new_socket, motd.c_str(), motd.length(), 0);
//        send(new_socket, motd_end.c_str(), motd_end.length(), 0);
//        
//        is_registered = true;
//        std::cout << "STATUS: " << client_nick << " is now fully registered." << std::endl;
//    }
//
//    // 3. Keep-alive (PING/PONG)
//    if (message.find("PING") == 0) {
//        std::string pong = "PONG localhost\r\n";
//        send(new_socket, pong.c_str(), pong.length(), 0);
//    }
//
////    // 1. Handle PING (Crucial for staying connected)
////    if(message.find("PING") == 0){
////      std::string pong = "PONG " + message.substr(5); //send back the same tocken
////      send(new_socket, pong.c_str(), pong.length(), 0);
////    }
////
////    // 2. Handle NICK/USER Registration
////    if (message.find("NICK") != std::string::npos){
////      size_t start = message.find("NICK") + 5;
////      size_t end = message.find_first_of("\r\n", start);
////      client_nick = message.substr(start, end - start);
////
////      //IRC standard "welcome" sequence
////      std::string rpl_001 = ":localhost 001 " + client_nick + " :Welcome to the IRC Network!\r\n";
////      std::string rpl_002 = ":localhost 002 " + client_nick + " :Your host is localhost, running version 0.1\r\n";
////      send(new_socket, rpl_001.c_str(), rpl_001.length(), 0);
////      send(new_socket, rpl_002.c_str(), rpl_002.length(), 0);
////    }
////
////    // 3. Simple JOIN placeholder (to stop irssi from complaining)
////    if (message.find("JOIN") != std::string::npos){
////      size_t start = message.find("JOIN") + 5;
////      size_t end = message.find_first_of("\r\n", start);
////
////      //Define Channel variable
////      std::string channel = message.substr(start, end - start);
////
////      std::cout << "DEBUG: User " << client_nick << " joining channel: " << channel << std::endl;
////
////      //Format: :Nick!User@Host JOIN :#handle_client
////      std::string join_msg = ":" + client_nick + " JOIN " + channel + "\r\n";
////      send(new_socket, join_msg.c_str(), join_msg.length(), 0);
////    }
//  }
//  close(new_socket);
//}

void handle_client(int new_socket) {
    char buffer[1024];
    std::string client_nick = "guest";
    bool is_registered = false;

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(new_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0) {
            std::cout << "DEBUG: Connection closed by client." << std::endl;
            break; 
        }

        std::string message(buffer);
        std::cout << "RAW FROM CLIENT: " << message;

        // 1. Handle CAP (Capability Negotiation)
        if (message.find("CAP LS") != std::string::npos) {
            std::string cap_reply = ":localhost CAP * LS :\r\n";
            send(new_socket, cap_reply.c_str(), cap_reply.length(), 0);
        }

        // 2. Handle NICK
        if (message.find("NICK") != std::string::npos) {
            size_t start = message.find("NICK") + 5;
            size_t end = message.find_first_of(" \r\n", start);
            client_nick = message.substr(start, end - start);
        }

        // 3. Handle USER & Complete Registration
        if (!is_registered && (message.find("USER") != std::string::npos)) {
            // Numeric 001 is mandatory for Irssi to stay connected
            std::string rpl_001 = ":localhost 001 " + client_nick + " :Welcome to the C++ IRC Server!\r\n";
            send(new_socket, rpl_001.c_str(), rpl_001.length(), 0);
            
            // Numeric 376 (End of MOTD) tells the client "You can send commands now"
            std::string rpl_376 = ":localhost 376 " + client_nick + " :End of MOTD\r\n";
            send(new_socket, rpl_376.c_str(), rpl_376.length(), 0);
            
            is_registered = true;
        }

        // 4. Handle PING
        if (message.find("PING") != std::string::npos) {
            // Important: Respond with 'PONG' and the server name or token
            std::string pong = ":localhost PONG localhost :" + client_nick + "\r\n";
            send(new_socket, pong.c_str(), pong.length(), 0);
        }
    }
    close(new_socket);
}

void run_simple_server() {
  int server_fd;
  struct sockaddr_in address;
  int addrlen = sizeof(address);

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
    throw std::runtime_error("ERROR: failed to create socket");
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

  memset(&address, 0, addrlen);
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(IP_ADDRESS.c_str()); // Bind to all interfaces
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0){
    close(server_fd);
    throw std::runtime_error("ERROR: Bind failed");
  }

  std::cout << "SUCCESS: Socket bound to " << IP_ADDRESS << ";" << PORT << std::endl;

  if (listen(server_fd, BACKLOG) < 0){
    close(server_fd);
    throw std::runtime_error("ERROR: Listen failed");
  }

  std::cout << "ACTION: Waiting for client connections..." << std::endl;

  while (true){
    int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
    if (new_socket < 0){
      std::cerr << "ERROR: Accept failed" << std::endl;
      continue; // Move to the next itteration to accept another client
    }

    std::cout << "SUCCESS: Client connect! New Socket FD for communication: " << new_socket << std::endl;

    //Handle each client connection in a seperate thread
    std::thread client_thread([new_socket]() {
      handle_client(new_socket);
    });
    client_thread.detach();
  }

  close(server_fd);
}

int main() {
  try{
    run_simple_server();
  }catch(const std::exception &e){
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return (1);
  }
  return (0);
}
