//
//  Notification.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/29/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Notification.h"

#include "GameObject.h"
#include "Level.h"
#include "Scenario.h"
#include "UIStack.h"

using namespace ci;
namespace core {

#pragma mark - Notification

/*
		ID _identifier;
*/

Notification::Notification( ID nId ):
	_identifier( nId )
{}

Notification::Notification( ID nId, const boost::any &message ):
	_identifier( nId ),
	_message( message )
{}
	
Notification::~Notification()
{}

#pragma mark - NotificationDispatcher


/*
		Scenario *_scenario;
*/

NotificationDispatcher::NotificationDispatcher( Scenario *scenario ):
	_scenario(scenario)
{}

NotificationDispatcher::~NotificationDispatcher()
{}

void NotificationDispatcher::broadcast( const Notification &note )
{
	_scenario->notificationReceived(note);

	Level *level = _scenario->level();
	if ( level )
	{
		level->notificationReceived( note );

		for (BehaviorSet::iterator behavior(level->behaviors().begin()),end(level->behaviors().end()); behavior != end; ++behavior )
		{
			(*behavior)->notificationReceived(note);
		}

		for (GameObjectSet::iterator obj(level->objects().begin()),end(level->objects().end()); obj != end; ++obj )
		{
			(*obj)->notificationReceived(note);
		}
	}
	
	ui::Stack *stack = _scenario->uiStack();
	stack->notificationReceived(note);
		
	for( ui::Stack::LayerList::iterator layer(stack->layers().begin()),end(stack->layers().end()); layer != end; ++layer )
	{
		(*layer)->notificationReceived(note);
		for ( ui::ViewSet::iterator view((*layer)->views().begin()),end((*layer)->views().end()); view != end; ++view )
		{
			(*view)->notificationReceived(note);
		}
	}	
}


}