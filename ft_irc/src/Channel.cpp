/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhvidste <rhvidste@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/21 13:55:07 by rhvidste          #+#    #+#             */
/*   Updated: 2026/01/21 14:11:23 by rhvidste         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Channel.hpp"

Channel::Channel(const std::string& name) : _name(name){}

const std::string& Channel::name() const {
  return _name;
}

bool Channel::hasMember(const Client* client) const{
  return _members.find(const_cast<Client*>(client)) != _members.end();
}

void Channel::addMember(Client* client){
  _members.insert(client);
}

void Channel::removeMember(Client* client){
  _members.erase(client);
  _operators.erase(client);
}

bool Channel::isOperator(const Client* client) const {
  return _operators.find(const_cast<Client*>(client)) != _operators.end();
}

void Channel::addOperator(Client* client) {
  _operators.insert(client);
}

void Channel::removeOperator(Client* client){
  _operators.erase(client);
}

void Channel::broadcast(const std::string& msg, const Client* except){
  for (std::unordered_set<Client*>::iterator it = _members.begin(); it != _members.end(); ++it){
    Client* m = *it;
    if (except && m == except) continue;
    m->queue(msg);
  }
}
