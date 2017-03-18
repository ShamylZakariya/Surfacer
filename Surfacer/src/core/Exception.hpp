//
//  Exception.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/11/17.
//
//

#ifndef Exception_hpp
#define Exception_hpp

#include <exception>
#include <cinder/Log.h>

namespace core {

class Exception : public std::exception
{
public:

	Exception( const std::string &w ):
		_what(w)
	{
		CI_LOG_EXCEPTION("KABOOM", (*this));
	}

	virtual ~Exception() throw() {}

	virtual const char* what() const throw() { return _what.c_str(); }

private:

	std::string _what;
};

class DrawException : public Exception
{
public:

	DrawException( const std::string &w ):Exception(w){}
	virtual ~DrawException() throw() {}

};

class InitException : public Exception
{
public:

	InitException( const std::string &w ):Exception(w){}
	virtual ~InitException() throw() {}
};

}

#endif /* Exception_hpp */
