#include "Server.hpp"
// #include <stdlib.h>

// --------------------------------------------------------------------- command KICK
void Server::cmdKICK(Client& client, const Command& cmd) {
	// KICK <channel> <user> (<comment>)
	if (cmd.params.size() < 2) {
		sendError(client, 461, "KICK :Need more parameters");
		return;
	}
	if (!_channels.contains(cmd.params[0])) {
		sendError(client, 403, "KICK :no such channel [ " + cmd.params[0] + " ]");
		return;
	}
	Channel& chan = _channels.at(cmd.params[0]);
	Client* target = getClientByNick(cmd.params[1]);
	if (!target) {
		sendError(client, 401, "KICK :No such nick");
		return;
	}
	int fd = (*target).fd();

	if (!chan.ops.contains(client.fd())) {
		sendError(client, 482, "KICK :You're not channel operator");
		return;
	}
	if (!chan.members.contains(fd)) {
		sendError(client, 441, "KICK :They aren't on that channel");
		return;
	}
	
	std::string comment = "";
	if (cmd.params.size() >= 3)
		comment = " using [" + cmd.params[2] + "] as the reason (comment)";

	std::string msg = prefix(client) + "KICK " + chan.name + " " + cmd.params[1] \
									 + ":" + client.nick() + " to remove " \
									 + cmd.params[1] + " from channel" + comment + "\r\n";
	(void)send(fd, msg.c_str(), msg.size(), MSG_NOSIGNAL);
	broadcast(chan, msg, nullptr);
	if (chan.ops.contains(fd))
		chan.ops.erase(fd);
	if (chan.invites.contains(fd))
		chan.invites.erase(fd);
	chan.members.erase(fd);
	if (chan.members.size() == 0) {
		_channels.erase(chan.name);
	}
}

// ------------------------------------------------------------------- command INVITE
void	Server::cmdINVITE(Client& inviter, const Command& cmd){
	// INVITE <nick> <channel>
	if (cmd.params.size() < 2){
		sendError(inviter, 461, "INVITE :Not enough parameters");
		return ;
	}

	const std::string& nick		= cmd.params[0];
	const std::string& chanName = cmd.params[1];

	if (nick[0] == '#') {
		sendError(inviter, 401, nick + " :No such nick");
		return;
	}
	if (chanName[0] != '#') {
		sendError(inviter, 403, chanName + " :No such channel");
		return;
	}
	// find target client
	Client* target = getClientByNick(nick);
	if (!target){
		sendError(inviter, 401, nick + " :No such nick");
		return ;
	}

	// find Channel
	Channel* ch = getChannel(chanName);
	if (!ch){
		sendError(inviter, 403, chanName + " :No such channel");
		return ;
	}

	// inviter must be in channel
	if (!ch->members.contains(inviter.fd())){
		sendError(inviter, 442, chanName + " :You're not on that channel");
		return ;
	}

	// if invite-only, only ops can invite
	if (ch->invite_only && !ch->ops.contains(inviter.fd())){
		//sendError(inviter, 482, chanName + " :You're not a channel operator");
		//sendError(inviter, 341, "");

		sendError(inviter, 482, chanName + " :You're not a channel operator");
		// technically the wrong numeric but seems to behaive as expected
		//sendError(inviter, 482, chanName + " :You're not a channel operator");
		return ;
	}

	// target already in channel?
	if (ch->members.contains(target->fd())){
		sendError(inviter, 443, nick + " " + chanName + " :is already on channel");
		return;}

	// record invite
	ch->invites.insert(target->fd());

	// numeric back to inviter: 341 RPL_INVITING
	// format	:server 341 <inviteNick> <targetNick> <channel>
	sendNumeric(inviter, 341, inviter.nick() + " " + target->nick() + " " + chanName);

	// send INVITE message to target
	// format: :<inviter> INVITE <target> :<channel>
	std::string msg = ":" + inviter.nick() + "!" + inviter.user() + "@ircserv " + "INVITE " + nick + " :" + chanName;
	target->queue(msg);
}

// -------------------------------------------------------------------- command TOPIC
void Server::cmdTOPIC( Client& client, const Command& cmd ) {
	// TOPIC <channel> - to view topic
	// TOPIC <channel> <topic> - to change topic
	if (cmd.params.empty() || !_channels.contains(cmd.params[0])) {
		sendError(client, 401, "TOPIC :No such channel");
		return;
	}
	Channel& chan = _channels.at(cmd.params[0]);
	if (cmd.params.size() == 1) {
		if (chan.topic.empty()) {
			client.queue(pref + "331 " + client.nick() + " " + chan.name + ":No topic set");
			return;
		}
		else {
			client.queue(pref + "332 " + client.nick() + " " + chan.name + " " + chan.topic);
			return;	
		}
	}
	else {
		if (chan.topic_operator && !chan.ops.contains(client.fd())) {
			sendError(client, 482, "TOPIC :You are not operator");
			return;
		}
		if (cmd.params.size() < 2) {
			sendError(client, 461, "TOPIC :Not enough parameters");
			return;
		}
		chan.topic = cmd.params[1];
		std::string s = prefix(client) + "TOPIC " + chan.name + " : " + chan.topic;
		broadcast(chan, s);
	}
}

// --------------------------------------------------------------------- command MODE
void Server::cmdMODE(Client& client, const Command& cmd) {
	if (cmd.params.empty()) {
		sendError(client, 461, "MODE :Not enough parameters");
		return;
	}

	const std::string& target = cmd.params[0];
	std::string mode = "";
	if (cmd.params.size() > 1) 
		mode = cmd.params[1];

	// ---------------- USER MODE ----------------
	if (!target.empty() && target[0] != '#') {
		if (target != client.nick()) {
			sendError(client, 502, ":Cannot change mode for other users");
			return;
		}

		if (target != client.nick()) {
			sendError(client, 502, ":Cannot change mode for other users");
			return;
		}
		return;
	}

	// ---------------- CHANNEL MODE ----------------
	Channel* chan = getChannel(target);
	if (!chan) {
		sendError(client, 403, target + " :No such channel");
		return;
	}

	if (mode.empty()) {
		// just query channel modes
		std::string modeStr = "+";
		std::string params;

		if (chan->invite_only) modeStr += "i";
		if (chan->topic_operator) modeStr += "t";
		if (!chan->key.empty()) {
			modeStr += "k";
			params += " " + chan->key;
		}

		client.queue(":ircserv 324 " + client.nick() + " " + chan->name + " " + modeStr + params);
		return;
	}

	bool is_op = chan->ops.contains(client.fd());

	if (mode == "+i") {
		if (!is_op) {
			sendError(client, 482, ":You are not channel operator");
			return;
		}
		chan->invite_only = true;
	} else if (mode == "-i") {
		if (!is_op) {
			sendError(client, 482, ":You are not channel operator");
			return;
		}
		chan->invite_only = false;
	} else if (mode == "+t") {
		if (!is_op) {
			sendError(client, 482, ":You are not channel operator");
			return;
		}
		chan->topic_operator = true;
	} else if (mode == "-t") {
		if (!is_op) {
			sendError(client, 482, ":You are not channel operator");
			return;
		}
		chan->topic_operator = false;
	} else if (mode == "+k" || mode == "-k") {
		if (!is_op) {
			sendError(client, 482, ":You are not channel operator");
			return;
		}
		if (mode == "+k") {
			if (cmd.params.size() < 3) { sendError(client, 461, "MODE :Not enough parameters"); return; }
			chan->key = cmd.params[2];
		} else {
			chan->key.clear();
		}
	} else if (mode == "+o" || mode == "-o") {
		if (!is_op) {
			sendError(client, 482, ":You are not channel operator");
			return;
		}
		if (cmd.params.size() < 3) {
			sendError(client, 461, "MODE :Not enough parameters");
			return;
		}
		const std::string& nick = cmd.params[2];
		Client* target_client = getClientByNick(nick);
		if (!target_client || !chan->members.contains(target_client->fd())) {
			sendError(client, 441, nick + " :User not in channel");
			return;
		}
		if (mode == "+o")
			chan->ops.insert(target_client->fd());
		else
			chan->ops.erase(target_client->fd());
	} else if (mode == "+l" || mode == "-l") {
		if (!is_op) {
			sendError(client, 482, ":You are not channel operator");
			return;
		}
		if ((mode == "+l" && cmd.params.size() < 3) || (mode == "-l" && cmd.params.size() < 2)) {
			sendError(client, 461, "MODE :Not enough parameters");
			return;
		}
		if (mode == "+l"){
			int limit = std::atoi(cmd.params[2].c_str());
			if (limit >= 1){
				chan->limit = limit;
			}
			else {
				sendError(client, 461, "MODE :Not enough parameters");
				return;
			}
		}
		else
			chan->limit = 0;
	} else {
		sendError(client, 501, "MODE :Unknown MODE flag");
		return;
	}
	// broadcast the mode change
	std::string msg = prefix(client) + "MODE " + chan->name + " :" + mode;
	if ((mode == "+k" || mode == "+o" || mode == "-o" || mode == "+l") && cmd.params.size() >= 3)
		msg += " " + cmd.params[2];
	client.queue(msg);
	broadcast(*chan, msg, &client);
}

// --------------------------------------------------------------------- command JOIN
void Server::cmdJOIN(Client& client, const Command& cmd){
	// JOIN <channel>
	if (cmd.params.empty()){
		sendError(client, 461, "JOIN :Not enough parameters");
		return;
	}

	const std::string& name = cmd.params[0];
	Channel* ch;

	if (_channels.contains(name))
		ch = &_channels.at(name);
	else{
		auto [it, created] = _channels.try_emplace(name, name);
		ch = &it->second;
		ch->ops.insert(client.fd());
	}

	if (ch->invite_only && !ch->invites.contains(client.fd()) &&
		!ch->ops.contains(client.fd())){
		sendError(client, 473, name + " :Cannot join channel (+i)");
		return;
	}

	if (ch->limit && ch->members.size() >= (size_t)ch->limit){
		sendError(client, 471, name + " :Cannot join channel (+l)");
		return;
	}

	if (!ch->key.empty()){
		if (cmd.params.size() < 2){
			sendError(client, 461, "JOIN :Not enough parameters");
			return;
		}
		if (cmd.params[1] != ch->key){
			sendError(client, 475, name + " :Cannot join channel (+k)");
			return;
		}
	}

	ch->members.insert(client.fd());
	ch->invites.erase(client.fd());

	/* JOIN broadcast */
	broadcast(*ch, prefix(client) + "JOIN " + name, nullptr);

	/* Topic */
	if (ch->topic.empty())
		client.queue(pref + "331 " + client.nick() + " " + name + " :No topic is set");
	else
		client.queue(pref + "332 " + client.nick() + " " + name + " :" + ch->topic);

	/* NAMES */
	std::string names = pref + "353 " + client.nick() + " = " + name + " :";
	for (int fd : ch->members){
		Client* c = getClientByFd(fd);
		if (!c) continue;
		if (ch->ops.contains(fd))
			names += "@";
		names += c->nick() + " ";
	}
	client.queue(names);
	client.queue(pref + "366 " + client.nick() + " " + name + " :End of /NAMES list");
}

// ------------------------------------------------------------------ command PRIVMSG
void	Server::cmdPRIVMSG( Client& sender, const Command& cmd){
	// in this case client == sender
	if (cmd.params.empty()){
		sendError(sender, 411, ":No recipent given (PRIVMSG)");
		return;
	}
	if	(cmd.params.size() < 2 || cmd.params[1].empty()){
		sendError(sender, 412, ":No text to send");
		return;
	}
	const std::string& target	= cmd.params[0];
	//const std::string& text		= cmd.params[1];
  std::string text = "";

  // adding space bfore text params if there are many (mostly for nc handling)
  for (size_t i = 1; i < cmd.params.size(); i++){
    if (i == 1)
      text += cmd.params[i];
    else
      text += " " + cmd.params[i];
  }

	//build the prefix part once
	const std::string prefix = ":" + sender.nick() + "!" + sender.user() + "@ircserv ";

	//channel message
	if (!target.empty() && target[0] == '#'){
		Channel* ch = getChannel(target);
		if (!ch){
			sendError(sender, 403, target + " :No such channel");
			return;
		}

		if (!_channels.contains(target)) {
			sendError(sender, 403, target + " :No such channel");
			return;
		}

		if (!ch->members.contains(sender.fd())) {
			sendError(sender, 442, "You're not on that channel");
			return;
		}

		const std::string prefix	= ":" + sender.nick() + "!" + sender.user() + "@ircserv ";
		const std::string out		= prefix + "PRIVMSG " + target + " :" + text;
		broadcast(*ch, out, &sender);
		return;
	}

	// // Direct message (nick)
	Client *destination = getClientByNick(target);
	if (!destination){
		sendError(sender, 401, target + " :No such nick/channel");
		return ;
	}

	const std::string out = prefix + "PRIVMSG " + target + " :" + text;
	destination->queue(out);

	// //TEMP FOR DEBUGGING
	// std::cerr << "PRIVMSG target=[" << target << "] map_size=" << _clients_by_nick.size() << "\n";
}

// --------------------------------------------------------------------- command PART
void Server::cmdPART(Client& client, const Command& cmd) {
	// PART <channel> (<reason>)
	if (cmd.params.empty() || cmd.params.size() < 1) {
		sendError(client, 461, "PART :Need more parameters");
		return;
	}
	if (!_channels.contains(cmd.params[0])) {
		sendError(client, 403, "PART :no such channel [ " + cmd.params[0] + " ]");
		return;
	}
	Channel& chan = _channels.at(cmd.params[0]);
	int fd = client.fd();
	if (!chan.members.contains(fd)) {
		sendError(client, 442, "PART :Not on channel");
		return;
	}

	std::string reason = "";
	if (cmd.params.size() > 1)
		reason = " | reason: [ " + cmd.params[1] + " ]";
	std::string msg = prefix(client) + client.nick() + " has left " + chan.name + reason;
	broadcast(chan, msg, nullptr);

	chan.ops.erase(fd);
	chan.invites.erase(fd);
	chan.members.erase(fd);
	if (chan.members.size() == 0) {
		_channels.erase(chan.name);
	}
}
