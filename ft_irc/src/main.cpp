/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhvidste <rhvidste@student.hive.email.com  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/13 14:57:23 by rhvidste          #+#    #+#             */
/*   Updated: 2026/01/13 15:00:10 by rhvidste         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <iostream>

int main(int argc, char **argv){
  if (argc != 3){
    std::cerr
              << "Usage: ./ircserv <port> <password>"
              << std::endl;
    return (1);
  }

  int port              = std::atoi(argv[1]);
  std::string password  = argv[2];

  Server server(port, password);
  server.run();
  return(0);
}
