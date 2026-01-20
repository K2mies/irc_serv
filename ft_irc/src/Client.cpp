/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jforsten <jforsten@student.hive.fi>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/12 11:31:40 by rhvidste          #+#    #+#             */
/*   Updated: 2026/01/20 19:38:03 by jforsten         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include "Client.hpp"

// ----------------------------------------------------------------- constructor
Client::Client( int fd  )  :  _fd( fd ),
                              _passOk     ( false ),
                              _hasNick    ( false ),
                              _hasUser    ( false ),
                              _registered ( false ),
                              _welcomed   ( false ) {}

// ---------------------------------------------------------------------- getter
int Client::fd() const {  return _fd; }


// ----------------------------------------------------- identity / registration
const std::string&  Client::nick()          const{ return  _nick; }
const std::string&  Client::user()          const{ return  _user; }
bool                Client::isRegistered()  const{ return _registered;  }
bool                Client::isWelcomed()    const{ return _welcomed;  }

// ----------------------------------------------------- identity / registration
void  Client::setNick(  const std::string& nick ){
  _nick     = nick;
  _hasNick  = !_nick.empty(); // empty nick means not yet set by default
}

void  Client::setUser(  const std::string& user ){
  _user     = user;
  _hasUser  = !_user.empty(); //empty user means not yet set by default
}

void  Client::setPassOk(  bool ok ){  _passOk = ok; }

/*
 * @brief validates and confirms registrtion
 */
void  Client::tryCompleteRegistration(){
  if( _registered ){
    std::cout
              << "Client is already registered..."
              << std::endl;
    return;
  }
  if(_passOk && _hasNick && _hasUser){
    _registered = true;
  }
}

void  Client::setWelcomed(){
  _welcomed = true;
}

// ----------------------------------------------------------- inbound buffering
/*
 * @brief getter for the _in buffer
 */
std::string& Client::inbuf(){ return _in; }

/* @brief parse the line for /r/n
 * 1: Look inside _in
 * 2: If it contains a full IRC line ending in \r\n placee into line
 * 3: Remove that line (and the CRLF) from _in
 * 4: Return true if a line was popped, otherwise false
 * @param line  reference to a line variable
 */
//bool Client::popLine( std::string& line ) {
//
//  // IRC lines end with \r\n
//  const std::string::size_type pos = _in.find("\r\n");
//
//  if (pos == std::string::npos)
//    return (  false );
//
//  line =  _in.substr(0, pos);
//          _in.erase (0, pos + 2); // remove line + "\r\n"
//
//  return (  true  );
//}

/*
 * @brief more forgiving version of popLine that includes /n
 * preferable used on local jesting
 * @param line  referene to a line variable
 */
bool  Client::popLine(  std::string &line ){

  //IRC lines end with "\r \n"
  std::string::size_type pos = _in.find("\r\n");
  std::size_t eol_len = 2;

  if (  pos == std::string::npos  ){
    pos = _in.find('\n'  );
    eol_len = 1;
    if (  pos == std::string::npos ){
      return (  false );
    }
    // If the line ends with "\r\n", the earlier find() would have caught it.
    // If it ends with "\r", strip it (e.g. when receiving "\r\n" split oddly).
    if (  pos > 0 && _in[pos - 1] == '\r'){
      line =  _in.substr(0, pos - 1);
    } else{
      line =  _in.substr(0, pos);
    }
  } else{
      line =  _in.substr(0, pos);
  }

  _in.erase(0, pos + eol_len);
  return ( true );
}

// ---------------------------------------------------------- outbound buffering

/*
 * @brief appends "\r\n" to the end of message as long as it is not already there (safer version)
 * @param msg   const string message variable
 */
void Client::queue( const std::string& msg  ){
	_out += msg;

	if (msg.size() < 2 || msg.substr(msg.size() - 2) != "\r\n")
		_out += "\r\n";
}

/*
 * @brief checks wether output (_out) is or is not empty
 */
bool  Client::hasPendingOutput() const{ return !_out.empty();  }

/*
 * @brief getter for the _out buffer
 */
std::string& Client::outbuf(){  return _out; }


// ------------------------------------------------------ membership bookkeeping
void  Client::joinChannel  (const std::string& chan){  _channels.insert  (chan  );  }
void  Client::leaveChannel (const std::string& chan){  _channels.erase   (chan  );  }
const ChannelSet& Client::channels()  const {  return _channels; }
