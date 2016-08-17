//
//  SensorTestScenario.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/10/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "SensorTestScenario.h"


#include <cinder/app/App.h>
#include <cinder/ImageIO.h>
#include <cinder/Utilities.h>

#include "Background.h"
#include "Fluid.h"
#include "GameAction.h"
#include "GameConstants.h"
#include "GameNotifications.h"
#include "PlayerHud.h"
#include "Player.h"
#include "PowerUp.h"
#include "PowerPlatform.h"
#include "Sensor.h"
#include "ViewportController.h"

#include "SurfacerApp.h"


using namespace game;

namespace {

	const real LevelScale = 0.5;
	
	const std::string ActionDispatcherJSONConfig =
"{\
	\"ThisIsMyMessageBoxSensorID\":{\
		\"class\":\"MessageBoxAction\",\
		\"initializer\":{\
			\"message\":\"<p>This is a test MessageBoxAction message for sensor ID <strong>'ThisIsMyMessageBoxSensorID'</strong></p>\",\
			\"type\":\"info\"\
		}\
	},\
	\"ThisIsANOTHERMessageBoxSensorID\":{\
		\"class\":\"MessageBoxAction\",\
		\"initializer\":{\
			\"message\":\"<p>This should tag one of the batteries</p>\",\
			\"type\":\"tag\",\
			\"targetIdentifier\":\"Battery0\"\
		}\
	}\
}";

}

/*
		game::ViewportController *_cameraController;		
		Font _font;
		
		cpBody *_mouseBody, *_draggingBody;
		cpConstraint *_mouseJoint;						
		Vec2r _mouseWorld, _previousMouseWorld;
*/

SensorTestScenario::SensorTestScenario():
	_cameraController(NULL),
	_font("HelveticaNeue-Light", 18),
	_mouseBody(NULL),
	_draggingBody(NULL),
	_mouseJoint(NULL),
	_mouseWorld(0,0),
	_previousMouseWorld(0,0)
{
	takeFocus();
	resourceManager()->pushSearchPath( getHomeDirectory() / "Dropbox/Code/Projects/Surfacer/Resources" );
}

SensorTestScenario::~SensorTestScenario()
{}

void SensorTestScenario::setup()
{
	GameScenario::setup();

	_mouseBody = cpBodyNew(INFINITY, INFINITY);
	
	//
	//	Create the Level instance, and get the ResourceManager
	//
	
	GameLevel::init levelInit;
	levelInit.gravity = Vec2r( 0, -9.8 );
	levelInit.defaultZoom = 25;

	GameLevel *level = new GameLevel();
	level->initialize( levelInit );
	setLevel( level );
		
	//
	//	Create the terrain instance
	//	
	
	terrain::Terrain::init terrainInit;			
	terrainInit.scale = LevelScale;
	terrainInit.levelImage = "SensorTestScenario.png";
	terrainInit.materialTexture = "GreyTerrain.png";
	terrainInit.materialColor = Color( 0.2, 0.2, 0.2 );
	terrainInit.greebleTextureAtlas = "BlueTerrainGreebleAtlas.png";
	terrainInit.greebleSize = 0.5;
	terrainInit.greebleTextureIsMask = true;
	terrainInit.sectorSize = Vec2i( 64, 64 );
	
	terrain::Terrain *terrain = new terrain::Terrain();
	terrain->initialize( terrainInit );
	level->addObject( terrain );

	//
	//	Add a background renderer
	//

	Background::init backgroundInit;
	backgroundInit.clearColor = Color::black();
	backgroundInit.sectorSize = 64;
	
	real scale = 1.5;
	backgroundInit.addLayer( "ColorfulBackground.png", scale );
	backgroundInit.addLayer( "ColorfulBackground.png", std::pow(scale,2), ColorA(1,1,1,0.5) );
	Background *background = new Background();
	background->initialize( backgroundInit );

	level->addObject( background );
	

	//
	//	Add the Player instance
	//

	if ( true )
	{
		Player::init playerInit;
		playerInit.height = 2.5;
		playerInit.width = 1.1;

		// by the narrow area to crouch through
		//playerInit.position = Vec2r( 60,112 );
		
		// by the powerups
		//playerInit.position = Vec2r( 100,86 );

		//	by the lava
		//playerInit.position = Vec2r( 205.899, 87.8 );

		//	top right
		playerInit.position = Vec2r( 210, 146 );

		playerInit.cuttingBeamInit.maxRange = 200;
		playerInit.cuttingBeamInit.cutStrength = 1 * 1;
		playerInit.cuttingBeamInit.attackStrength = 1;
		playerInit.cuttingBeamInit.width = LevelScale;
		playerInit.cuttingBeamInit.cutWidth = 2*LevelScale;
		
		Player *player = new Player();
		player->initialize( playerInit );
		
		level->addObject( player );	
		
		//player->selectWeaponType( WeaponType::MAGNETO_BEAM );		
	}

	if ( true )
	{
		Sensor::init sensorInit;
		Sensor *sensor = NULL;

		PowerUp::init 
			medikitInit,
			batteryInit;

		PowerUp 
			*battery = NULL,
			*medikit = NULL;

		//
		//	Add some powerups to the top-right
		//
		
		PowerPlatform::init platformInit;
		platformInit.position = Vec2r( 211,144 );
		platformInit.size = Vec2r(4,2);
		PowerPlatform *platform = new PowerPlatform();
		platform->initialize( platformInit );
		level->addObject( platform );

		medikitInit.identifier = "Medikit0";
		medikitInit.amount = 10;
		medikitInit.type = PowerUpType::MEDIKIT;

		medikit = new PowerUp();
		medikitInit.position = Vec2r(195,143);
		medikit->initialize(medikitInit);
		level->addObject( medikit );					

		medikit = new PowerUp();
		medikitInit.identifier = "Medikit1";
		medikitInit.position = Vec2r(199,143);
		medikit->initialize(medikitInit);
		level->addObject( medikit );


		batteryInit.identifier = "Battery0";
		batteryInit.amount = 10;
		batteryInit.type = PowerUpType::BATTERY;

		battery = new PowerUp();
		batteryInit.position = Vec2r(202,143);
		battery->initialize(batteryInit);
		level->addObject( battery );

		battery = new PowerUp();
		batteryInit.identifier = "Battery1";
		batteryInit.position = Vec2r(206,144);
		battery->initialize(batteryInit);
		level->addObject( battery );

		if ( false )
		{
			// create a viewport fit sensors
			sensorInit.position = Vec2r(180,142);
			sensorInit.width = 20;
			sensorInit.height = 6;
			sensorInit.angle = toRadians( real(15) );
			sensor = new ViewportFitSensor();
			sensor->initialize( sensorInit );
			level->addObject( sensor );

			sensorInit.position = Vec2r(198,142);
			sensorInit.width = 20;
			sensorInit.height = 6;
			sensorInit.angle = toRadians( real(-15) );
			sensor = new ViewportFitSensor();
			sensor->initialize( sensorInit );
			level->addObject( sensor );

			sensorInit.position = Vec2r(190,172);
			sensorInit.width = sensorInit.height = 6;
			sensorInit.height = sensorInit.height = 60;
			sensorInit.angle = toRadians( real(0) );
			sensor = new ViewportFitSensor();
			sensor->initialize( sensorInit );
			level->addObject( sensor );
		}

		// create terrain sensors
		sensorInit.position = Vec2r(160,143);
		sensorInit.width = sensorInit.height = 4;
		sensorInit.angle = toRadians( real(0) );
		sensorInit.targetType = SensorTargetType::TERRAIN;
		sensorInit.voxelId = 135;
		sensorInit.eventName = "Bar";
		sensor = new Sensor();
		sensor->initialize( sensorInit );
		level->addObject( sensor );

		sensorInit.position = Vec2r(150,141);
		sensorInit.width = sensorInit.height = 4;
		sensorInit.angle = toRadians( real(0) );
		sensorInit.targetType = SensorTargetType::TERRAIN;
		sensorInit.voxelId = 178;
		sensorInit.eventName = "Baz";
		sensor = new Sensor();
		sensor->initialize( sensorInit );
		level->addObject( sensor );

		sensorInit.position = Vec2r(155,150);
		sensorInit.width = sensorInit.height = 4;
		sensorInit.angle = toRadians( real(0) );
		sensorInit.targetType = SensorTargetType::TERRAIN;
		sensorInit.voxelId = 0; // All voxels!
		sensorInit.eventName = "Qux";
		sensor = new Sensor();
		sensor->initialize( sensorInit );
		level->addObject( sensor );
		
		// create a message box sensor
		sensorInit.eventName = "ThisIsMyMessageBoxSensorID";
		sensorInit.position = Vec2r(220,150);
		sensorInit.width = sensorInit.height = 16;
		sensorInit.targetType = SensorTargetType::PLAYER;
		sensor = new Sensor();
		sensor->initialize( sensorInit );
		level->addObject( sensor );

		// create a message box sensor which tags the battery
		sensorInit.eventName = "ThisIsANOTHERMessageBoxSensorID";
		sensorInit.position = Vec2r(200,150);
		sensorInit.width = sensorInit.height = 16;
		sensorInit.targetType = SensorTargetType::PLAYER;
		sensor = new Sensor();
		sensor->initialize( sensorInit );
		level->addObject( sensor );
		
		
		
		// sanity check
		GameObject *found = level->objectByInstanceId( sensor->instanceId());
		assert( sensor == found );

		level->actionDispatcher()->initialize( ActionDispatcherJSONConfig );
	}


	if ( true )
	{
		PowerUp::init 
			medikitInit,
			batteryInit;

		PowerUp 
			*battery = NULL,
			*medikit = NULL;

		//
		//	By the lava
		//
		
		batteryInit.position = Vec2r( 198, 90 );
		batteryInit.type = PowerUpType::BATTERY;
		battery = new PowerUp();
		battery->initialize( batteryInit );
		level->addObject( battery );

		batteryInit.position = Vec2r( 198, 88 );
		battery = new PowerUp();
		battery->initialize( batteryInit );
		level->addObject( battery );

		batteryInit.position = Vec2r( 198, 86 );
		battery = new PowerUp();
		battery->initialize( batteryInit );
		level->addObject( battery );
		
	}
	

	//
	//	On the left valley 
	//


	if ( true )
	{
		PowerUp::init 
			medikitInit,
			batteryInit;

		PowerUp 
			*battery = NULL,
			*medikit = NULL;


		battery = new PowerUp();
		batteryInit.position = Vec2r(81.5,84);
		batteryInit.type = PowerUpType::BATTERY;
		battery->initialize(batteryInit);
		level->addObject( battery );

		battery = new PowerUp();
		batteryInit.position = Vec2r(83.7,83);
		battery->initialize(batteryInit);
		level->addObject( battery );

		medikit = new PowerUp();
		medikitInit.position = Vec2r(88.9,82.3);
		medikitInit.type = PowerUpType::MEDIKIT;
		medikit->initialize(medikitInit);
		level->addObject( medikit );
	}
	
	//
	//	On the top left
	//


	if ( true )
	{
		PowerUp::init 
			medikitInit,
			batteryInit;

		PowerUp 
			*battery = NULL,
			*medikit = NULL;


		battery = new PowerUp();
		batteryInit.position = Vec2r(50,114);
		batteryInit.type = PowerUpType::BATTERY;
		battery->initialize(batteryInit);
		level->addObject( battery );

		battery = new PowerUp();
		batteryInit.position = Vec2r(47,116);
		battery->initialize(batteryInit);
		level->addObject( battery );

		medikit = new PowerUp();
		medikitInit.position = Vec2r(44,118);
		medikitInit.type = PowerUpType::MEDIKIT;
		medikit->initialize(medikitInit);
		level->addObject( medikit );
	}	
	
	//
	//	Add Lava
	//

	if ( true )
	{
		Fluid::init lavaInit;
		lavaInit.particle.radius = 0.4;	
		lavaInit.particle.visualRadiusScale = 4;	
		lavaInit.particle.density = 200;
		lavaInit.particle.entranceDuration = 5;

		lavaInit.attackStrength = 1;
		lavaInit.injuryType = InjuryType::LAVA;
		lavaInit.fluidParticleTexture = "LavaParticle.png";
		lavaInit.fluidToneMapTexture = "LavaToneMap.png";
		lavaInit.clumpingForce = 50;

		lavaInit.boundingVolumePosition = Vec2r(216,85);
		lavaInit.boundingVolumeRadius = 5;
		
		Fluid *lava = new Fluid();
		lava->initialize( lavaInit );		
		level->addObject( lava );		
	}
	

	//
	//	We're good to go!
	//
					
	level->camera().lookAt( v2r(cpBBCenter( level->bounds())), 10 );
	
	_cameraController = new ViewportController( level->camera(), ViewportController::Zeno );
	_cameraController->setConstraintMask( ViewportController::ConstrainPanning | ViewportController::ConstrainZooming );
	level->addBehavior( _cameraController );		
}

void SensorTestScenario::shutdown()
{
	GameScenario::shutdown();
	setLevel( NULL );
}

void SensorTestScenario::resize( Vec2i size )
{
	GameScenario::resize( size );
}

void SensorTestScenario::update( const time_state &time )
{
	GameScenario::update( time );
	GameLevel *level = gameLevel();

	{
		cpVect mouseBodyPos = cpv(_mouseWorld),
			   mouseBodyVel = cpvmult(cpvsub(mouseBodyPos, cpv(_previousMouseWorld)), time.deltaT );

		cpBodySetPos( _mouseBody, mouseBodyPos );
		cpBodySetVel( _mouseBody, mouseBodyVel );
		_previousMouseWorld = v2r(mouseBodyVel);
		
		if ( _draggingBody )
		{
			cpBodySetAngVel( _draggingBody, cpBodyGetAngVel(_draggingBody) * 0.9 );
		}		
	}

	if ( !level->cameraController()->tracking() )
	{
		const real dp = 10.0f;
		if ( isKeyDown( app::KeyEvent::KEY_UP ))	level->cameraController()->setPan( level->cameraController()->pan() + Vec2r( 0,   -dp ));
		if ( isKeyDown( app::KeyEvent::KEY_DOWN ))	level->cameraController()->setPan( level->cameraController()->pan() + Vec2r( 0,   +dp ));
		if ( isKeyDown( app::KeyEvent::KEY_LEFT ))	level->cameraController()->setPan( level->cameraController()->pan() + Vec2r( +dp, 0 ));
		if ( isKeyDown( app::KeyEvent::KEY_RIGHT ))	level->cameraController()->setPan( level->cameraController()->pan() + Vec2r( -dp, 0 ));		
	}
}

void SensorTestScenario::draw( const render_state &state )
{
	GameScenario::draw( state );


	// establish a top-down coordinate system
	gl::enableAlphaBlending();
	gl::setMatricesWindow( app::getWindowWidth(), app::getWindowHeight(), true );
	
	glActiveTexture( GL_TEXTURE0 );
	glEnable( GL_TEXTURE_2D );

	if ( true )
	{
		std::stringstream msg;
		msg << "fps: " << int(app::App::get()->getAverageFps()) 
			<< " ups: " << int( 1/level()->time().deltaT )
			<< " sps: " << int(SurfacerApp::instance()->getAverageSps());

		gl::drawStringRight( msg.str(), Vec2r(app::getWindowWidth() - 10, app::getWindowHeight() - 30 ), Color(1,1,1), _font );
	}
}

#pragma mark - Input

bool SensorTestScenario::mouseDown( const app::MouseEvent &event )
{		
	Viewport &camera( level()->camera());
	cpSpace *space = level()->space();
	GameLevel *level = gameLevel();

	_mouseWorld = _previousMouseWorld = camera.screenToWorld( event.getPos() );
	app::console() << "Clicked @ " << _mouseWorld << std::endl;
	
	if ( isKeyDown( app::KeyEvent::KEY_SPACE )) return false;

	if ( !level->player() )
	{
		cpShape *shape = cpSpacePointQueryFirst( space, cpv(_mouseWorld), CP_ALL_LAYERS, CP_NO_GROUP);
		if( shape )
		{
			cpBody *body = shape->body;

			if ( body && !cpBodyIsStatic(body) )
			{
				_draggingBody = body;
				_mouseJoint = cpPivotJointNew( _draggingBody, _mouseBody, cpv(_mouseWorld));
				_mouseJoint->maxForce = cpBodyGetMass( body ) * std::max(level->initializer().gravity.length(),real(1)) * 1000;
				cpSpaceAddConstraint( space, _mouseJoint );
				
				return true;
			}
		}
	}
	
	return false;
}

bool SensorTestScenario::mouseUp( const app::MouseEvent &event )
{	
	if ( _mouseJoint )
	{
		cpSpaceRemoveConstraint( level()->space(), _mouseJoint );
		cpConstraintFree( _mouseJoint );
		_draggingBody = NULL;
		_mouseJoint = NULL;
		
		return true;
	}

	return false;
}	

bool SensorTestScenario::mouseWheel( const app::MouseEvent &event )
{
	real wheelScale = 0.1,
		 dz = (event.getWheelIncrement() * wheelScale);

	_setZoom( _zoom() * (1+dz) );
	return true;
}

bool SensorTestScenario::mouseMove( const app::MouseEvent &event, const Vec2r &delta )
{
	_mouseWorld = level()->camera().screenToWorld( event.getPos() );
	return false;
}

bool SensorTestScenario::mouseDrag( const app::MouseEvent &event, const Vec2r &delta )
{
	GameLevel *level = gameLevel();
	
	_mouseWorld = level->camera().screenToWorld( event.getPos() );
	
	if ( level->player() ) return false;

	if ( !level->cameraController()->tracking() && isKeyDown( app::KeyEvent::KEY_SPACE ) )
	{
		level->cameraController()->setPan( level->cameraController()->pan() + delta );
		return true;
	}
	
	return false;
}	

bool SensorTestScenario::keyDown( const app::KeyEvent &event )
{
	//app::console() << "SensorTestScenario::keyDown code: " << event.getCode() << std::endl;

	switch( event.getCode() )
	{
		case app::KeyEvent::KEY_BACKQUOTE:
		{
			setRenderMode( RenderMode::mode( (int(renderMode()) + 1) % RenderMode::COUNT ));
			return true;
		}
		
		case app::KeyEvent::KEY_F12:
		{
			screenshot( getHomeDirectory() / "Desktop", "SensorTestScenario" );
			return true;
		}
		
		case app::KeyEvent::KEY_F11:
		{
			app::setFullScreen( !app::isFullScreen());
			return true;
		}
		
		case app::KeyEvent::KEY_PERIOD:
		{
			_setZoom( _zoom() * 1.1 );
			break;
		}

		case app::KeyEvent::KEY_COMMA:
		{
			_setZoom( _zoom() / 1.1 );
			break;
		}

		default: break;
	}
	
	return false;
}

bool SensorTestScenario::keyUp( const app::KeyEvent &event )
{
	return false;
}

#pragma mark - Plumbing

real SensorTestScenario::_zoom() const
{
	return gameLevel()->cameraController()->zoom();
}

void SensorTestScenario::_setZoom( real z )
{
	GameLevel *level = gameLevel();
	if ( level->cameraController()->tracking() ) return;
	
	Vec2r center;	
	
	if ( level->player() )
	{
		center = level->camera().worldToScreen( level->player()->position() );
	}
	else
	{
		center = mousePosition();
	}

	_cameraController->setZoom( std::max( z, real(0.1) ), center );
}
