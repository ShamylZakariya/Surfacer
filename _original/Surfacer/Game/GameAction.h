#pragma mark once

//
//  Action.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/19/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Classloader.h"

namespace game {

class GameLevel;
class Action;

#pragma mark - ActionDispatcher

/**
	class ActionDispatcher
	
	Initialize with JSONData that looks something like so:
	
	{
		"AnEventName": {
			"class": "MessageBoxAction",
			"initializer":{
				"message":"This is my message box message",
				"modal":false
			}
		},
		"SomeOtherEventName": {
			"class": "FooAction",
			"initializer":{
				"paramZero":456,
				"paramOne":"hello"				
			}
		}	
	}
	
	It maps an event name ( can come from anywhere, but generally will represent a SensorID ) and maps
	it to an Action subclass, and the parameters to construct that class with.
	
	

*/
class ActionDispatcher
{
	public:
	
		ActionDispatcher( GameLevel * level );
		virtual ~ActionDispatcher();
		
		GameLevel *level() const { return _level; }
		
		void initialize( const std::string &JSONData );
		virtual void initialize( const ci::JsonTree &jsonData );
		
		virtual void eventStart( std::string eventName );
		virtual void eventEnd( std::string eventName );


	private:
	
		ci::JsonTree _initializer;
		GameLevel *_level;
		std::map< std::string, Action* > _actions;
		
};



#pragma mark - Action

class Action : public core::Classloadable
{
	public:
	
		struct init : public core::util::JsonInitializable
		{
			init(){}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{}

		};

	public:
	
		Action();
		virtual ~Action();
		
		JSON_INITIALIZABLE_INITIALIZE();
		
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
		
		/**
			Start this Action
			Whatever this game action represents -- monster emission, end-of-level, messagebox, etc: cause it to happen
			using the parameters in initializer() to specialize.
		*/
		virtual void start( GameLevel * ) = 0;

		/**
			Whatever this Action did, stop it
		*/
		virtual void stop( GameLevel * ) = 0;
				
	private:
	
		init _initializer;
		
};


}