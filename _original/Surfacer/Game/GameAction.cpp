//
//  Action.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/19/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "GameAction.h"
#include "Level.h"

#include <cinder/app/App.h>

using namespace ci;
namespace game {

#pragma mark - ActionDispatcher

/*
		ci::JsonTree _initializer;
		GameLevel *_level;
		std::map< std::string, Action* > _actions;
*/

ActionDispatcher::ActionDispatcher( GameLevel *level ):
	_level(level)
{}

ActionDispatcher::~ActionDispatcher()
{
	for ( std::map< std::string, Action* >::iterator it(_actions.begin()),end(_actions.end()); it != end; ++it )
	{
		Action *action = it->second;
		if ( action )
		{
			action->stop( _level );
			delete action;
		}
	}
}

void ActionDispatcher::initialize( const std::string &JSONData )
{
	ci::JsonTree root( JSONData );
	initialize( root );
}

void ActionDispatcher::initialize( const ci::JsonTree &jsonData )
{
	_initializer = jsonData;
}

void ActionDispatcher::eventStart( std::string eventName )
{
	try
	{
		const ci::JsonTree
			ActionInfo = _initializer[eventName];
		
		//app::console() << "ActionDispatcher::eventStart( " << eventName << " ) - ActionInfo: " << ActionInfo << "\n\ninitializer: " << _initializer << std::endl;
		
		const ci::JsonTree
			ActionClass = ActionInfo["class"],
			ActionInit = ActionInfo["initializer"];

		try
		{
			Action *action = core::classload<Action>( ActionClass.getValue(), ActionInit );

			assert( action );
			assert( !_actions[eventName] );

			_actions[eventName] = action;
			action->start( _level );
		}
		catch( const std::exception &e )
		{
			app::console() << "ActionDispatcher::eventStart - Unable to load action " << ActionClass.getValue() << " exception: " << e.what() << std::endl;
		}
	}
	catch( const cinder::JsonTree::ExcChildNotFound & ){}
}

void ActionDispatcher::eventEnd( std::string eventName )
{
	Action *action = _actions[eventName];
	if ( action )
	{
		action->stop(_level);
		delete action;
		_actions.erase(eventName);
	}
}


#pragma mark - Action

Action::Action(){}
Action::~Action(){}

void Action::initialize( const init &i )
{
	_initializer = i;
}

}