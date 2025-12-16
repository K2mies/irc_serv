#include <arpa/inet.h> // For htons(), inet_addr()
#include <cstring>     // For memset()
#include <iostream>
#include <netinet/in.h> // For sockaddr_in
#include <stdexcept>
#include <string>
#include <sys/socket.h> // For socket(), bind(), listen(), accept()
#include <unistd.h>     // For close()

// A standard, high-numbered port to use for testing
const int PORT = 8080;

// Use the loopback address (127.0.0.1) for local testing
const std::string IP_ADDRESS = "127.0.0.1";

// Maximum number of pending connections the queue can hold
const int BACKLOG = 10;

void run_simple_server() {
  int server_fd, new_socket;
  struct sockaddr_in address;
  int addrlen = sizeof(address);

  // 1a. Create the Socket (socket())
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    throw std::runtime_error("ERROR: Failed to create socket");
  }

  // Set socket options (SO_REUSEADDR)
  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
             sizeof(opt));

  // 1b. Define Address Structure
  memset(&address, 0, addrlen);
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(IP_ADDRESS.c_str());
  address.sin_port = htons(PORT);

  // 1c. Bind the Socket (bind())
  if (bind(server_fd, (struct sockaddr *)&address, addrlen) < 0) {
    close(server_fd);
    throw std::runtime_error("ERROR: Bind failed. Port may already be in use.");
  }
  std::cout << "SUCCESS: Socket bound to " << IP_ADDRESS << ":" << PORT
            << std::endl;
  std::cout << "Server File Descriptor (Listening Socket): " << server_fd
            << std::endl;

  // --- 2. LISTEN FOR CONNECTIONS (listen()) ---
  // Puts the server socket into listening mode.
  // The BACKLOG is the max size of the queue for pending connections.

  if (listen(server_fd, BACKLOG) < 0) {
    close(server_fd);
    throw std::runtime_error("ERROR: Listen failed");
  }

  std::cout << "ACTION: waiting for a client connection..." << std::endl;
  // 'accept()' blocks execution until a client attempts to connect.
  // If successful, it returns a *NEW* socket descriptor (new_socket) for that
  // client.
  if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                           (socklen_t *)&addrlen)) < 0) {
    close(server_fd);
    throw std::runtime_error("ERROR: Accept failed");
  }

  std::cout << "SUCCESS: Client connected! New socket FD for communication: "
            << new_socket << std::endl;

  // --- 4. COMMUNICATION (Simple send/recv) ---
  const char *hello = "Hello, Client! Connection accepted.";
  // Send a simple message to the newly connected client
  send(new_socket, hello, strlen(hello), 0);
  std::cout << "STATUS: Sent 'Hello' message to client." << std::endl;

  // --- 5. CLEANUP ---
  std::cout << "STATUS: Closing sockets." << std::endl;
  // Close the new socket created for the client
  close(new_socket);
  // Close the original listening socket
  close(server_fd);
}

int main() {
  try {
    run_simple_server();
  } catch (const std::exception &e) {
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return (1);
  }
  return (0);
}
