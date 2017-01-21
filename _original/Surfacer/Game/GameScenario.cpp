//
//  GameScenario.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "GameScenario.h"

#include "GameAction.h"
#include "Filters.h"
#include "GameComponents.h"
#include "GameLevel.h"
#include "GameNotifications.h"
#include "Player.h"
#include "ViewportController.h"

using namespace ci;
using namespace core;
namespace game {

/*
		real _injuryEffectStrength;
		core::DamagedMonitorFilter *_injuryEffectFilter;
*/

GameScenario::GameScenario():
	_injuryEffectStrength(0),
	_injuryEffectFilter( new core::DamagedMonitorFilter())
{}

GameScenario::~GameScenario()
{}

void GameScenario::notificationReceived( const Notification &note )
{
	switch( note.identifier() )
	{
		case game::Notifications::PLAYER_INJURED:
		{
			const HealthComponent::injury_info &Info = boost::any_cast<HealthComponent::injury_info>(note.message());
			const real Strength = Info.grievousness;
			_injuryEffectStrength = lrp<real>(0.25, Strength, _injuryEffectStrength);

			break;
		}
	}
}

void GameScenario::setup()
{
	Scenario::setup();
	
	// Gameplay filters
	PalettizingFilter *pf = new PalettizingFilter;
	pf->setStrength(1);
	filters()->push(pf);

	pf->setColorModel( PalettizingFilter::RGB );
	pf->setPaletteSize(24, 24, 24);

	NoiseFilter *nf = new NoiseFilter();
	nf->setMonochromeNoise( true );
	nf->setStrength(0.0125);
	filters()->push(nf);
	
	// injury effect filter
	filters()->push(_injuryEffectFilter);
	_injuryEffectFilter->setPixelSizes(8, 1, 4);
	_injuryEffectFilter->setChannelOffsets(Vec2(0,0), Vec2(4,0), Vec2(-4,0));
	_injuryEffectFilter->setScanlineStrength(0.25);
	_injuryEffectFilter->setColorOffset(Vec3(0.5,0,0));
	_injuryEffectFilter->setColorScale(Vec3(0.5,0.5,0.5));
	_injuryEffectFilter->setStrength(0);	
}

void GameScenario::shutdown()
{
	Scenario::shutdown();
	setLevel(NULL);
}

void GameScenario::update( const core::time_state &time )
{
	Scenario::update( time );
	
	GameLevel *level = gameLevel();
	if ( !level ) return;
			
	
	//
	//	Apply injury effect
	//

	Player *player = level->player();
	if ( player )
	{
		if ( player->dead() )
		{
			// max out injury effect filter, while fading to black
			_injuryEffectStrength = lrp<real>(0.5, 1, _injuryEffectStrength );
			
			const real FadeRate = 0.995;
			_injuryEffectFilter->setColorScale( _injuryEffectFilter->colorScale() * FadeRate );
			_injuryEffectFilter->setColorOffset( _injuryEffectFilter->colorOffset() * FadeRate );
		}
		else
		{
			_injuryEffectStrength *= 0.95;
		}
	
		const real Strength = _injuryEffectStrength * _injuryEffectStrength;
		_injuryEffectFilter->setStrength( Strength );
		
		if ( Strength > ALPHA_EPSILON && !(time.step % 3))
		{
			const int PixelScale = Rand::randInt(4, 16);
			_injuryEffectFilter->setPixelSizes(PixelScale, 1, PixelScale/2);
			
			const real 
				GreenOffset = Rand::randFloat(2,8),
				BlueOffset = -Rand::randFloat(2,8);
			
			_injuryEffectFilter->setChannelOffsets(Vec2(0,0), Vec2(GreenOffset,0), Vec2(BlueOffset,0));
		}
	}
}

GameLevel *GameScenario::loadLevel( const fs::path &levelBundleFolder )
{
	const std::string MANIFEST("manifest.json");
	
	fs::path fullPath;
	if ( !resourceManager()->findPath( levelBundleFolder, fullPath ) )
	{
		app::console() << "GameScenario::loadLevel - unable to alias " << levelBundleFolder << " to a folder on the filesystem" << std::endl;
		return NULL;
	}

	if ( fs::exists( fullPath ) && fs::is_directory( fullPath ))
	{
		const std::string &manifestJSONText = resourceManager()->getString( fullPath / MANIFEST );

		if ( !manifestJSONText.empty() )
		{
			try
			{
				ci::JsonTree root( manifestJSONText );
				
				GameLevel *level = new GameLevel();
				level->initialize( root );

				setLevel(level);
				level->resourceManager()->pushSearchPath( fullPath );

				//
				//	Populate level with objects and events
				//

				const ci::JsonTree ObjectsArray = root["objects"];
				for ( ci::JsonTree::ConstIter child(ObjectsArray.begin()),end(ObjectsArray.end()); child != end; ++child )
				{
					level->create( *child );
				}

				level->actionDispatcher()->initialize( root["events"] );
					
			}
			catch(ci::JsonTree::Exception &e)
			{
				app::console() << "GameScenario::loadLevel - Unable to parse level JSON:\n-----" 
					<< manifestJSONText
					<< "\n-----"
					<< "\tERROR: "
					<< e.what()
					<< std::endl;
			}
		}
		else
		{
			app::console() << "GameScenario::loadLevel - Unable to open level manifest file: " << (levelBundleFolder / MANIFEST) << std::endl;
		}
	}
	else
	{
		app::console() << "GameScenario::loadLevel - Level bundle either doesn't exist or isn't a folder!"
			<< " exists: "
			<< str(fs::exists(levelBundleFolder))
			<< " is_directory: "
			<< str(fs::is_directory(levelBundleFolder))
			<< std::endl;
	}
	
	return gameLevel();
}


}
