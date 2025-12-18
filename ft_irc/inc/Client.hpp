/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhvidste <rhvidste@student.hive.email.com  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/18 10:56:52 by rhvidste          #+#    #+#             */
/*   Updated: 2025/12/18 11:05:57 by rhvidste         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

class Client {
private:
  int _fd;
  std::string _nick;
  std::string _user;
  bool _registered;
};
