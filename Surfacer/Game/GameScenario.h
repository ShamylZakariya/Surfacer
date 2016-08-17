#pragma once

//
//  GameScenario.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Scenario.h"

namespace core {

	class DamagedMonitorFilter;

}

namespace game {

class GameLevel;

class GameScenario : public core::Scenario
{
	public:
	
		GameScenario();
		virtual ~GameScenario();

		// NotificationListener
		virtual void notificationReceived( const core::Notification &note );

		// Scenario
		virtual void setup();
		virtual void shutdown();
		virtual void update( const core::time_state &time );
		
		// GameScenario
		GameLevel *gameLevel() const { return (GameLevel*)level(); }

		/*
			gets the current player damage effect strength
		*/
		real playerInjuryEffectStrength() const { return _injuryEffectStrength; }
		
		/**
			Attempts to load a level from a level bundle.
			A level bundle is a folder formed as such:
			- LevelFoo.surfacerLevel
				- manifest.json
				- graphics files, etc
			
			The level bundle folder is pushed to the top of the resourceManager's search paths.
			
			If the level loading succeeded, returns the GameLevel and makes it the active gameLevel()			
		*/

		GameLevel *loadLevel( const ci::fs::path &levelBundleFolder );

	private:
	
		real _injuryEffectStrength;
		core::DamagedMonitorFilter *_injuryEffectFilter;

};


}