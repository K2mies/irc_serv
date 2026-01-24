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
	std::string topic	= "";
	std::string key		= "";
	int limit		= 0;
	std::unordered_set<int> invites;
	std::unordered_set<int> ops;
	std::unordered_set<int> members;

	bool invite_only = true;

	int checkPass(std::string pw);
	int setPass(std::string pw);
    explicit Channel(const std::string& n) : name(n), topic("") {}
    Channel() : topic("") {} // IMPORTANT: needed for try_emplace/emplace sometimes
    // ~Channel();
private:
	std::string _pass = "";
	// void Channel::opCmds(Server& s, int fd, std::string& cmd, std::string arg);
};
