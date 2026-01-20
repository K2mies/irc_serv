#include "Client.hpp"
#include "Channel.hpp"
#include "Server.hpp"
#include <string_view>

Channel::Channel()
: name("") {}

int getNick(Server& s, std::string& nick) {

	for (auto &[i, c] : s.fds) {
		if (c.nick == nick)
			return c.fd;
	}
	return 0;
}

int Channel::checkPass(std::string pass) {
	if (_pass.empty() || _pass == pass)
		return 0;
	else
		return 1;
}

int Channel::setPass(std::string pass) { _pass = pass; }

void Channel::opCmds(Server& s, int fd, std::string& cmd, std::string arg) {

	if (!ops.contains(fd))
		return;
	if (cmd == "KICK") {
		int victim = getNick(s, arg);
		if (victim) {
			ops.erase(victim);
			invites.erase(victim);
			members.erase(victim);
		} // Optional else if not found if needed...
	}
	if (cmd == "INVITE") {
		int victim = getNick(s, arg);
		Client& v = s.fds.at(victim);
		Client& c = s.fds.at(fd);
		if (victim) {
			v.to += c.nick + "!~" + c.nick + s.pref + "INVITE " + v.nick + ":#" + name + "\r\n";
			c.to += s.pref + "341" + c.nick + " " + v.nick + ":#" + name + "\r\n";
		} else
			c.to += s.pref + "401" + c.nick + " " + v.nick + ":No such nick\r\n";
	}
	if (cmd == "TOPIC")
		topic = arg;

}

