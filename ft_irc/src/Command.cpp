/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Command.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: rhvidste <rhvidste@student.hive.email.com  +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/15 12:58:46 by rhvidste          #+#    #+#             */
/*   Updated: 2026/01/15 13:52:16 by rhvidste         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Command.hpp"
#include <cctype>
#include <string>
#include <vector>

static inline void trimSpacesInPlace(std::string& x) {
	size_t a = 0;
	while (a < x.size() && x[a] == ' ') a++;
	size_t b = x.size();
	while (b > a && x[b - 1] == ' ') b--;
	x = x.substr(a, b - a);
}

static void pushCommaSplitParams(std::vector<std::string>& out, const std::string& token) {
	size_t start = 0;
	while (start <= token.size()) {
		size_t comma = token.find(',', start);
		std::string part = (comma == std::string::npos)
			? token.substr(start)
			: token.substr(start, comma - start);

		trimSpacesInPlace(part);
		if (!part.empty())
			out.push_back(part);

		if (comma == std::string::npos) break;
		start = comma + 1;
	}
}

static std::string  toUpper( std::string s ){
	for ( size_t i = 0; i < s.size(); i++ )
		s[i] = static_cast<char>( std::toupper(static_cast<unsigned char>( s[i] )));
	return ( s );
}

/*
 *  @brief  command parser
 *  1.  it uppercases the command so you can compare "PING" reliably.
 *  2.  t supports trailing params (super important for PRIVMSG later).
 */
Command  parseCommand( const std::string &line ){
	Command cmd;

	std::string s = line;
	size_t i = 0;

	// --------------------------------------------------------- skip leading spaces
	while ( i < s.size() && s[i] == ' ' ) i++;

	// ------------------------------------------------- optional prefex : ":prefex"
	if ( i < s.size() && s[i] == ':' ){
		size_t end = s.find( ' ', i );
		if ( end == std::string::npos ){
			// Line is only a prefix ( weird, but handle )
			cmd.prefix = s.substr(i + 1 );
			return( cmd );
		}
		cmd.prefix = s.substr(i + 1, end - (i + 1));
		i = end + 1;
		while ( i < s.size() && s[i] == ' ') i++;
	}

	// ---------------------------------------------------------------- command name
	size_t  end = s.find(' ', i);
	if (    end == std::string::npos  ){
					cmd.name = toUpper(s.substr(i));
					return ( cmd );
	}
	cmd.name = toUpper(s.substr(i, end - i ));
	i = end + 1;

	// ---------------------------------------------------------------------- params
	while ( i < s.size() ){

		while ( i <   s.size()  &&  s[i] == ' ') i++;
		if    ( i >=  s.size()) break;

		// Trailing param: ":" then rest of line (can contain spaces)
		if (  s[i] == ':' ) {
					cmd.params.push_back(s.substr(i + 1 ) );
					break;
		}

		size_t  next =  s.find(' ', i);
		if (    next == std::string::npos){
					std::string token = s.substr(i);
					pushCommaSplitParams(cmd.params, token);
					//cmd.params.push_back(s.substr(i));
					break;
		}

		std::string token = s.substr(i, next - i);
		pushCommaSplitParams(cmd.params, token);
		i = next + 1;
	}
	return ( cmd );
}
