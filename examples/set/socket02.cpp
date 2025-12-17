#include <iostream>
#include <set>

class Client {
public:
  int fd;
  std::string nick;
  std::string user;
  bool registered;
};

class Channel {
public:
  std::string name;
  std::set<Client *> members;
  std::set<Client *> operators;

  // A dedicated function for adding members with error checking
  bool addMember(Client *newClient) {

    bool is_fd_dup = false;
    bool is_nick_dup = false;
    bool is_user_dup = false;

    for (Client *m : members) {
      if (m->fd == newClient->fd) {
        std::cout << "Error: fd duplicates detected: " << newClient->fd
                  << std::endl;

        is_fd_dup = true;
      }
      if (m->nick == newClient->nick) {
        std::cout << "Error: Nickname already in use: " << newClient->nick
                  << std::endl;

        is_nick_dup = true;
      }
      if (m->user == newClient->user) {
        std::cout << "Error: Username already in use: " << newClient->user
                  << std::endl;

        is_user_dup = true;
      }
    }
    if (is_fd_dup == true || is_nick_dup == true || is_user_dup == true)
      return (false);

    members.insert(newClient);
    return (true);
  }
};

int main() {
  Channel chan;

  Client client00;
  Client client01;
  Client client02;

  client00.nick = "test_nick00";
  client00.user = "test_user00";
  client00.fd = 1;

  client01.nick = "test_nick01";
  client01.user = "test_user01";
  client01.fd = 2;

  client02.nick = "test_nick02";
  client02.user = "test_user02";
  client02.fd = 3;

  chan.addMember(&client00);
  chan.addMember(&client01);
  chan.addMember(&client02);
  // chan.members.insert(&client00);
  // chan.members.insert(&client01);
  // chan.members.insert(&client02);

  for (auto &member : chan.members) {
    std::cout << member->nick << ", " << std::endl;
  }
}
