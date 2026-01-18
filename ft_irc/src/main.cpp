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

int main(int argc, char **argv){
	if (argc != 3){
		std::cerr
							<< "Usage: ./ircserv <port> <password>"
							<< std::endl;
		return (1);
	}

	try {
		int port              = std::atoi(argv[1]);
		std::string password  = argv[2];

		Server server(port, password);
		server.run();
	} catch (std::exception & e) {
			std::cout << "Fatal error: " << e.what() << ".\n";
			return (1);
	}
	
	return(0);
}
