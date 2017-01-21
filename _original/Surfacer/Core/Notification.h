#pragma once

//
//  Notification.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/29/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <boost/any.hpp>

namespace core {

class Scenario;

class Notification
{
	public:
	
		typedef unsigned int ID;

	public:
	
		Notification( ID nId );
		Notification( ID nId, const boost::any &message );
		
		virtual ~Notification();
		
		ID identifier() const { return _identifier; }
		const boost::any &message() const { return _message; }

	private:
	
		ID _identifier;
		boost::any _message;
		
};

class NotificationListener
{
	public:
	
		NotificationListener(){}
		virtual ~NotificationListener(){}
		
		virtual void notificationReceived( const Notification &note ){}

};

/**
	Broadcasts notifications to the current Scenario, all GameObjects & Behaviors in the current Level,
	the ui::Stack, all ui::Layers and all ui::Views which are in the ui::Stack.
	
	Obviously, this is relatively expensive, so only do it when something really needs to be broadcast
	in a manner that makes more direct forms of event dispatch impractical. A good thing would be
	player death, end-of-level, etc.	
*/
class NotificationDispatcher
{
	public:
	
		NotificationDispatcher( Scenario *scenario );
		virtual ~NotificationDispatcher();
		
		Scenario *scenario() const { return _scenario; }
		
		/**
			Broadcasts @a note to EVERYTHING
		*/
		virtual void broadcast( const Notification &note );
		
	private:
	
		Scenario *_scenario;
		
};

}