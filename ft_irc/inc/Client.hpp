/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jforsten <jforsten@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/18 10:56:52 by rhvidste          #+#    #+#             */
/*   Updated: 2026/01/20 20:40:25 by jforsten         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include  <string>
#include  <unordered_set>

// -------------------------------------------------------------------- type defs
typedef std::unordered_set<std::string> ChannelSet;

class Client {
public:
  int           _fd;

  std::string   _nick;
  std::string   _user;

  bool          _passOk     = false;
  bool          _hasNick    = false;
  bool          _hasUser    = false;
  bool          _registered = false;
  bool          _welcomed   = false;

  ChannelSet    _channels;

  std::string   _in;
  std::string   _out;
  
  public:

  // ----------------------------------------------------------------- constructor
  explicit  Client(int fd);

  // ---------------------------------------------------------------------- getter
  int       fd()    const;

  // ----------------------------------------------------- identity / registration
  const std::string&  nick()          const;
  const std::string&  user()          const;
  bool                isRegistered()  const;
  bool                isWelcomed()    const;

  // ---------------------------------------------------------------- registration
  void  setNick   ( const std::string& nick );
  void  setUser   ( const std::string& user );
  void  setPassOk ( bool ok );
  void  tryCompleteRegistration(); // sets registered when PASS(if needed)+NICK+USER are ready
  void  setWelcomed();

  // ----------------------------------------------------------- inbound buffering
  std::string& inbuf();                 // append recv() bytes here
  bool  popLine( std::string &line );   // extract one \r\n line

  // ---------------------------------------------------------- outbound buffering
  void  queue( const std::string& msg ); // always appends \r\n in one place (recommended)
  bool  hasPendingOutput() const;
  std::string &outbuf();

  // ------------------------------------------------------ membership bookkeeping
  void  joinChannel   ( const std::string& chan );
  void  leaveChannel  ( const std::string& chan );
  const ChannelSet& channels() const;
};
