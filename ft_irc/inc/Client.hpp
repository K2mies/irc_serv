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

#pragma once

#include<iostream>
#include<unordered_set>

class Client {
public:

  explicit  Client(int fd);

  int       fd()    const;

  // ----------------------------------------------------- identity / registration
  const std::string&  nick() const;
  const std::string&  user() const;
  bool  isRegistered()       const;

  // ---------------------------------------------------------------- registration
  void setNick( const std::string& n );
  void setUser( const std::string& u );
  void setPassOk( bool ok );
  void tryCompleteRegistration(); // sets registered when PASS(if needed)+NICK+USER are ready

  // ----------------------------------------------------------- inbound buffering
  std::string& inbuf();               // append recv() bytes here
  bool popLine( std::string &line );  // extract one \r\n line

  // ---------------------------------------------------------- outbound buffering
  void queue( const std::string& msg ); // always appends \r\n in one place (recommended)
  bool hasPendingOutput() const;
  std::string &outbuf();

  // ------------------------------------------------------ membership bookkeeping
  void joinChannel(   const std::string& chan );
  void leaveChannel(  const std::string& chan );
  const std::unordered_set<std::string>& channels() const;

private:
  int           _fd;

  std::string   _nick;
  std::string   _user;

  bool          _passOk     = false;
  bool          _hasNick    = false;
  bool          _hasUser    = false;
  bool          _registered = false;

  std::unordered_set<std::string> _channels;

  std::string   _in;
  std::string   _out;
};
