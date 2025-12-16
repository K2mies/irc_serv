#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>

// int socket(int domain, int type, int protocol)

int main() {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    std::cerr << "Error: failed to open socket" << std::endl;
    return (1);
  }

  std::cout << "Fd: " << fd << std::endl;
  return (0);
}
