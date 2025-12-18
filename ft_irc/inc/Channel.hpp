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

#include "Client.hpp"

class Channel
{
  std::string         name;
  std::set<Client*>   members;
  std::set<Client*>   operators;

  void  broadcast(const std::string &msg, Client* except = NULL);
}; 
