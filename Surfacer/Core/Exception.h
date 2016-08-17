#pragma once

//
//  Exception.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 1/5/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <exception>
#include <cinder/app/AppBasic.h>

namespace core {

class Exception : public std::exception
{
	public:
	
		Exception( const std::string &w ):_what(w)
		{
			std::cerr
				<< std::endl
				<< "------------------------------------------------------------------------" << std::endl
				<< "\nTHROWING:\n" << w << std::endl
				<< "------------------------------------------------------------------------" << std::endl
				<< std::endl;
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