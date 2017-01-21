//
//  Classloader.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/21/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Classloader.h"
#include <CoreFoundation/CoreFoundation.h>
#include "Exception.h"

namespace core {

#pragma mark - Classloader

/*
		static Classloader* _instance;
		
		///////////////////////////////////////////////////////////////
		
		typedef std::map< std::string, ClassloadableFactoryFunction* > ClassloaderMap;
		ClassloaderMap _factories;		
*/

Classloader *Classloader::_instance = NULL;

Classloader *Classloader::instance()
{
	if ( !_instance )
	{
		_instance = new Classloader();
	}
	
	return _instance;
}

Classloader::Classloader()
{}

Classloader::~Classloader()
{}		
	
Classloadable *Classloader::create( const std::string &className )
{
	ClassloadableFactoryFunction *func = _load( className );
	if ( func )
	{
		return (*func)();
	}

	return NULL;
}

ClassloadableFactoryFunction *Classloader::_load( const std::string &className )
{
	{
		ClassloaderMap::iterator existing = _factories.find( className );
		if ( existing != _factories.end() )
		{
			return existing->second;
		}
	}

	
	void *ptr = _getFunctionPtrForName( className + "_create" );

	if ( !ptr )
	{
		return NULL;
	}

	ClassloadableFactoryFunction *fn = (Classloadable* (*)(void)) ptr;
	_factories[ className ] = fn;
	
	return fn;
}

void *Classloader::_getFunctionPtrForName( const std::string &functionName )
{
	CFStringRef name = CFStringCreateWithCString(kCFAllocatorDefault, functionName.c_str(), kCFStringEncodingUTF8 );
	void *ptr = CFBundleGetFunctionPointerForName( CFBundleGetMainBundle(), name );
	CFRelease(name);

	return ptr;
}



}
