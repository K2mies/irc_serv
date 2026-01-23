/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jforsten <jforsten@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/18 13:05:44 by rhvidste          #+#    #+#             */
/*   Updated: 2026/01/20 20:36:38 by jforsten         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <unordered_set>
#include "Client.hpp"

class Channel {
public:
	std::string name;
	std::string topic = "";
	std::string key = "";
	int limit = 0;
	std::unordered_set<int> invites;
	std::unordered_set<int> ops;
	std::unordered_set<int> members;

	bool invite_only = false;
	bool topic_operator = false;

    explicit Channel(const std::string& n) : name(n), topic(""), key("") {}
    Channel() : topic("") {} // IMPORTANT: needed for try_emplace/emplace sometimes
    // ~Channel();
private:
	// int checkPass(std::string pw);
	// int setPass(std::string pw);
	// std::string _pass = "";
	// void Channel::opCmds(Server& s, int fd, std::string& cmd, std::string arg);
};



// class Channel
// {
// 	public:
// 		explicit Channel( const std::string& name );

// 		const std::string& name() const;

// 		bool  hasMember(  const Client* c ) const;
// 		void  addMember(        Client* c );
// 		void  removeMember(     Client* c );

// 		bool  isOperator( const Client* c ) const;
// 		void  addOperator(      Client* c );
// 		void  removeOperator(   Client* c );

// 		void  broadcast(const std::string& msg, const Client* except = 0); // "except = sender" if set to default(0) will send to everyone

// 	private:
// 		std::string                   _name;
// 		std::unordered_set<Client*>   _members;
// 		std::unordered_set<Client*>   _operators;
// };

