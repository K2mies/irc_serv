/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhvidste <rhvidste@student.hive.email.com  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 14:50:22 by rhvidste          #+#    #+#             */
/*   Updated: 2026/01/13 11:57:36 by rhvidste         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */
#include "Client.hpp"
#include <iostream>

int main(){
  std::cout << "=== Client test harness ===\n";

  //fake file descriptor
  Client c(42);

  // Test 1: Input buffering + popLines------------------------

  std::cout << "\n[TEST] popLine()\n";

  c.inbuf() += "NICK alice\r\nUSER bob\r\nPING :123\r\n";
  std::string line;

  while(c.popLine(line)){
    std::cout << "LINE: [" << line << "]\n";
  }

  // Test 2: Output queue---------------------------------------
  std::cout << "\n[TEST] queue()\n";

  c.queue("PING :123");
  c.queue("PONG :123");

  std::cout << "Registered initially? " << c.isRegistered() << std::endl;

  c.setPassOk(true);
  c.setNick("K2MIES");
  c.setUser("Ross");
  c.tryCompleteRegistration();

  std::cout  
            << "Registered after PASS/NICK/USER? "
            << c.isRegistered()
            << "\n";

  // Test 4: Channel membership
  std::cout << "\n[TEST] channel membership\n";

  c.joinChannel("#test");
  c.joinChannel("#42");

  std::cout << "Channels:\n";

  for (const auto& ch : c.channels()){
    std::cout << " - " << ch << std::endl;
  }

  c.leaveChannel("#test");

  std::cout << "Channels after leaving #test:\n";
  for (const auto& ch : c.channels()){
    std::cout << " - " << ch << std::endl;
  }
  
  std::cout << "\n=== End of tests ===\n";

  return(0);
}


