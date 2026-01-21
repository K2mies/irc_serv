/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhvidste <rhvidste@student.hive.email.com  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/18 13:05:44 by rhvidste          #+#    #+#             */
/*   Updated: 2025/12/18 13:36:56 by rhvidste         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <unordered_set>
#include "Client.hpp"

class Client;

class Channel
{
  public:
    explicit Channel( const std::string& name );

    const std::string& name() const;

    bool  hasMember(  const Client* client ) const;
    void  addMember(        Client* client );
    void  removeMember(     Client* client );

    bool  isOperator( const Client* client ) const;
    void  addOperator(      Client* client );
    void  removeOperator(   Client* client );

    void  broadcast(const std::string& msg, const Client* except = 0); // "except = sender" if set to default(0) will send to everyone
  
  private:
    std::string                   _name;
    std::unordered_set<Client*>   _members;
    std::unordered_set<Client*>   _operators;
}; 
