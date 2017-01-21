#pragma once

//
//  Classloader.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/21/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "JsonUtils.h"

namespace core {

class Classloadable : public util::JsonInitializable
{
	public:
	
		Classloadable(){}
		virtual ~Classloadable(){}
};

typedef Classloadable* ClassloadableFactoryFunction(void);

/**
	@class Classloader
	Dynamically loads classes from the executable by classname. Does not support classes in plugins or external
	frameworks. 
	
	Classes which you want to be able to classload must derive from Classloadable, and must, in their .cpp file (or 
	somewhere which is not a header ) call the CLASSLOAD(SomeClassName) macro. This creates a factory function and
	exports it for dynamic resolution by CFBundleGetFunctionPointerForName.
	
	You won't use Classloader directly, rather, you'll use
		- core::classload( const std::string &className)
		- core::classload( const std::string &className, const ci::JsonTree &initializer )
		
	The latter function attempts to load the class, and then passes the ci::JsonTree object to the
	instance's initialize() method - which in general will directly configure the object, or will configure
	the object's init struct and pass that in turn to the object's initialize method which takes the init struct.	
*/
class Classloader 
{
	public:
	
		static Classloader *instance();
		
		Classloadable *create( const std::string &className );
	
	private:
	
		Classloader();
		~Classloader();

		ClassloadableFactoryFunction *_load( const std::string &functionName );
		void * _getFunctionPtrForName( const std::string &functionName );

	private:
		
		static Classloader* _instance;
		
		///////////////////////////////////////////////////////////////
		
		typedef std::map< std::string, ClassloadableFactoryFunction* > ClassloaderMap;
		ClassloaderMap _factories;		
};


/**
	@brief Load an instance by classname
*/
template< class T >
T* classload( const std::string &name )
{
	Classloadable *cl = Classloader::instance()->create( name );
	if ( cl )
	{
		T *obj = dynamic_cast<T*>(cl);
		if ( obj )
		{
			return obj;
		}
		else 
		{
			delete cl;
		}
	}

	return NULL;
}

/**
	@brief Load an instance by classname and initialize it.
	The object being initialized MUST:
		define a public struct 'init' which is constructable from a ci::JsonTree
		define a method initialize( const init & )
*/
template< class T >
T* classload( const std::string &name, const ci::JsonTree &initializer )
{
	Classloadable *cl = Classloader::instance()->create( name );
	if ( cl )
	{
		T *obj = dynamic_cast<T*>(cl);
		if ( obj )
		{
			cl->initialize( initializer );
			return obj;
		}
		else 
		{
			delete cl;
		}
	}

	return NULL;
}


}

#define CLASSLOAD(cname) extern "C" __attribute__((visibility ("default"))) core::Classloadable * cname ## _create(void){ return new cname; }
