#include "Client.hpp"
#include "Channel.hpp"
#include "Server.hpp"
#include <unordered_set>
#include <algorithm>


// Channel::Channel(const std::string& name) : _name(name){}

// const std::string& Channel::name() const {
//   return _name;
// }

bool Channel::hasMember(const Client* client) const{
  return members.find(const_cast<Client*>(client)) != members.end();
}

// void Channel::addMember(Client* client){
//   _members.insert(client);
// }

// void Channel::removeMember(Client* client){
//   _members.erase(client);
//   _operators.erase(client);
// }

// bool Channel::isOperator(const Client* client) const {
//   return _operators.find(const_cast<Client*>(client)) != _operators.end();
// }

// void Channel::addOperator(Client* client) {
//   _operators.insert(client);
// }

// void Channel::removeOperator(Client* client){
//   _operators.erase(client);
// }

void Channel::broadcast(const std::string& msg, const Client* except){
  for (std::unordered_set<Client*>::iterator it = members.begin(); it != members.end(); ++it){
    Client* m = *it;
    if (except && m == except) continue;
    m->queue(msg);
  }
}

for (auto& i : )

// Channel::Channel()
// : name("") {}

// int getNick(Server& s, std::string& nick) {

// 	for (auto &[i, c] : s.fds) {
// 		if (c.nick == nick)
// 			return c.fd;
// 	}
// 	return 0;
// }

// int Channel::checkPass(std::string pass) {
// 	if (_pass.empty() || _pass == pass)
// 		return 0;
// 	else
// 		return 1;
// }

// int Channel::setPass(std::string pass) { _pass = pass; }

// void Channel::opCmds(Server& s, int fd, std::string& cmd, std::string arg) {

// 	if (!ops.contains(fd))
// 		return;
// 	if (cmd == "KICK") {
// 		int victim = getNick(s, arg);
// 		if (victim) {
// 			ops.erase(victim);
// 			invites.erase(victim);
// 			members.erase(victim);
// 		} // Optional else if not found if needed...
// 	}
// 	if (cmd == "INVITE") {
// 		int victim = getNick(s, arg);
// 		Client& v = s.fds.at(victim);
// 		Client& c = s.fds.at(fd);
// 		if (victim) {
// 			v.to += c.nick + "!~" + c.nick + s.pref + "INVITE " + v.nick + ":#" + name + "\r\n";
// 			c.to += s.pref + "341" + c.nick + " " + v.nick + ":#" + name + "\r\n";
// 		} else
// 			c.to += s.pref + "401" + c.nick + " " + v.nick + ":No such nick\r\n";
// 	}
// 	if (cmd == "TOPIC")
// 		topic = arg;

// }


