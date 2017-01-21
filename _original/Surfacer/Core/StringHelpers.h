#pragma once

//
//  StringHelpers.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 12/22/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

/**
	@brief Convert @a v to a string
	@return the input parameter as a string.
*/
template< class T >
std::string str( T v )
{
	std::stringstream ss;
	ss << v;
	return ss.str();
}

/**
	@brief Convert @a v to a string
	@return the input parameter as a string with real precision @a prec
*/
template< class T >
std::string str( T v, int prec )
{
	std::stringstream ss;
	ss << std::setprecision( prec ) << std::fixed << v;
	return ss.str();
}

/**
 @brief Convert pointer value @a v to a string
 */
template< class T >
std::string str( T *v )
{
	std::stringstream ss;
	ss << "0x" << std::hex << v;
	return ss.str();
}

/**
	@brief Convert a boolean to string
	@return "true" or "false" according to @a v
*/
inline std::string str( bool v )
{
	return v ? "true" : "false";
}

/**
	@brief Convert a const chararacter pointer to a string
*/
inline std::string str( const char *s )
{
	return std::string( s );
}

/**
	@brief Convert a non-const chararacter pointer to a string
*/
inline std::string str( char *s )
{
	return std::string( s );
}

inline std::string str( const std::string &s )
{
	return s;
}

