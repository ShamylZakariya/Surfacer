//
//  LevelLoadingScenario.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 6/12/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "LevelLoadingScenario.h"

#include "Player.h"
#include "ViewportController.h"
#include "SurfacerApp.h"
#include "SvgParsing.h"

#include "Terrain.h"
#include "Background.h"

#include <cinder/Utilities.h>


using namespace game;

/*
		game::ViewportController *_cameraController;
*/

LevelLoadingScenario::LevelLoadingScenario()
{
	takeFocus();
	resourceManager()->pushSearchPath( getHomeDirectory() / "Dropbox/Code/Projects/Surfacer/Resources" );
}

LevelLoadingScenario::~LevelLoadingScenario()
{}

void LevelLoadingScenario::setup()
{
	GameScenario::setup();
	
	if ( false )
	{
		std::vector< std::string > colors;
		colors.push_back( "#FF00FF" );
		colors.push_back( "#CC00FF00" );
		colors.push_back( "rgb(0,128,255)");
		colors.push_back( "rgba(255,128,0,0.5)");
		colors.push_back( "rgb(0,100%,50%)");
		
		foreach( const std::string &color, colors )
		{
			ColorA parsedColor;
			if (core::util::svg::parseColor( color, parsedColor ))
			{
				app::console() << "Parsed \"" << color << "\" to: " << parsedColor << std::endl;
			}
			else
			{
				app::console() << "!!!Unable to parse color \"" << color << "\"" << std::endl;
			}
		}
	}
	
		
	loadLevel( "Levels/TestLevel0.surfacerLevel" );


	//
	//	We're good to go!
	//
		
	if ( level() )
	{
		level()->camera().lookAt( v2r(cpBBCenter( level()->bounds())), 20 );
	}
}

void LevelLoadingScenario::shutdown()
{
	GameScenario::shutdown();
}

void LevelLoadingScenario::resize( Vec2i size )
{
	GameScenario::resize(size);
}

void LevelLoadingScenario::update( const time_state &time )
{
	GameScenario::update( time );
	GameLevel *level = gameLevel();

	if ( !level->cameraController()->tracking() )
	{
		const real dp = 10.0f;
		if ( isKeyDown( app::KeyEvent::KEY_UP ))	level->cameraController()->setPan( level->cameraController()->pan() + Vec2r( 0,   -dp ));
		if ( isKeyDown( app::KeyEvent::KEY_DOWN ))	level->cameraController()->setPan( level->cameraController()->pan() + Vec2r( 0,   +dp ));
		if ( isKeyDown( app::KeyEvent::KEY_LEFT ))	level->cameraController()->setPan( level->cameraController()->pan() + Vec2r( +dp, 0 ));
		if ( isKeyDown( app::KeyEvent::KEY_RIGHT ))	level->cameraController()->setPan( level->cameraController()->pan() + Vec2r( -dp, 0 ));		
	}
}

void LevelLoadingScenario::draw( const render_state &state )
{
	GameScenario::draw( state );
}

bool LevelLoadingScenario::mouseDrag( const app::MouseEvent &event, const Vec2r &delta )
{
	GameLevel *level = gameLevel();

	if ( level->player() ) 
	{
		return false;
	}

	if ( isKeyDown( app::KeyEvent::KEY_SPACE ) )
	{
		level->cameraController()->setPan( level->cameraController()->pan() + delta );
		return true;
	}
	
	return false;
}	

// input
bool LevelLoadingScenario::keyDown( const app::KeyEvent &event )
{
	switch( event.getCode() )
	{
		case app::KeyEvent::KEY_BACKQUOTE:
		{
			setRenderMode( RenderMode::mode( (int(renderMode()) + 1) % RenderMode::COUNT ));
			return true;
		}
		
		case app::KeyEvent::KEY_F12:
		{
			screenshot( getHomeDirectory() / "Desktop", "MonsterPlaygroundScenario" );
			return true;
		}
		
		case app::KeyEvent::KEY_F11:
		{
			app::setFullScreen( !app::isFullScreen());
			return true;
		}
		
		case app::KeyEvent::KEY_SLASH:
		{
			level()->setPaused( !level()->paused() );
			break;
		}

		default: break;
	}
	
	return false;
}

		
