/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Command.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhvidste <rhvidste@student.hive.email.com  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/18 13:24:15 by rhvidste          #+#    #+#             */
/*   Updated: 2025/12/18 13:36:53 by rhvidste         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <iostream>
#include <string>
#include <vector>

struct Command {
  std::string               prefix;   // rarely used for client->server
  std::string               name;     // "NICK", "JOIN", ...
  std::vector<std::string>  params;   // includes trailing as one param if parsed that way
};
