#include <arpa/inet.h> // For htons(), htonl(), INADDR_ANY
#include <iostream>
#include <netinet/in.h> // For sockaddr_in
#include <stdexcept>
#include <string>
#include <sys/socket.h> // For socket(), bind()
#include <unistd.h>     //for close

// A standard, high-numbered port to use for testing
const int PORT = 8080;

// Use the loopback address (127.0.0.1) for local testing
const std::string IP_ADDRESS = "127.0.0.1";

// The main function to set up the server socket
void setup_server_socket() {
  // ===================================
  // 1. CREATE THE SOCKET (socket())
  // ===================================
  // Parameters:
  // - AF_INET: IPv4 protocol family
  // - SOCK_STREAM: Stream socket (TCP)
  // - 0: Default protocol for the given family and type
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (server_fd < 0) {
    throw std::runtime_error("ERROR: Failed to create socket.");
  }
  std::cout << "SUCCESS: socket created, File Descriptor (FD)" << server_fd
            << std::endl;

  // Optional: Set socket options to allow reusing the address/port immediately
  // after the program closes. This helps avoid "Address already in use" errors.
  int opt = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    // We throw an exception on critical errors, but socket options failure is
    // often just a warning. For simplicity, we'll continue, but a real server
    // might log a warning here.
    std::cout << "WARNING: setsockopt failed. Might face 'Address in Use' "
                 "errors later."
              << std::endl;
  }

  // ===================================
  // 2. DEFINE THE SERVER ADDRESS (sockaddr_in)
  // ===================================
  struct sockaddr_in address;

  memset(&address, 0, sizeof(address));

  address.sin_family = AF_INET; // IPv4 family
  // Convert the IP string "127.0.0.1" to binary network format
  address.sin_addr.s_addr = inet_addr(IP_ADDRESS.c_str());
  // Convert the port number (e.g., 8080) to network byte order
  address.sin_port = htons(PORT);

  // Alternative: To listen on ALL available network interfaces (0.0.0.0),
  // use: address.sin_addr.s_addr = htonl(INADDR_ANY);

  // ===================================
  // 3. BIND THE SOCKET TO THE ADDRESS (bind())
  // ===================================
  // Binds the socket file descriptor (server_fd) to the address structure
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    close(server_fd);
    throw std::runtime_error("ERROR: Bind failed. port already in use.");
  }

  std::cout << "SUCCESS: Socket bound to " << IP_ADDRESS << ":" << PORT
            << std::endl;

  // In a real server, the next step would be listen() and accept().

  // ===================================
  // 4. CLEANUP (Close the socket)
  // ===================================
  // In this simple example, we immediately close it after the setup is done.

  close(server_fd);
  std::cout << "SUCCESS: Socket closed" << std::endl;
}

int main() {
  try {
    setup_server_socket();
  } catch (const std::exception &e) {
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
