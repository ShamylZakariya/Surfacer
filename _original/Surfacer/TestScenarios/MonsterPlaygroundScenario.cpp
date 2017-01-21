//
//  MonsterPlaygroundScenario.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 12/5/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "MonsterPlaygroundScenario.h"


#include <cinder/app/App.h>
#include <cinder/ImageIO.h>
#include <cinder/Utilities.h>

#include "Background.h"
#include "GameConstants.h"
#include "GameNotifications.h"
#include "PlayerHud.h"
#include "Mirror.h"
#include "Monster.h"
#include "Player.h"
#include "PowerUp.h"
#include "PowerPlatform.h"
#include "Sensor.h"
#include "ViewportController.h"


#include "Barnacle.h"
#include "Urchin.h"
#include "Centipede.h"
#include "Grub.h"
#include "ShubNiggurath.h"
#include "Shoggoth.h"
#include "Tongue.h"
#include "YogSothoth.h"

#include "SurfacerApp.h"


using namespace game;

namespace {


	enum MonsterType {
		BARNACLE,
		URCHIN,
		GRUB,
		SHOGGOTH,
		SHUB_NIGGURATH,
		YOG_SOTHOTH,
		CENTIPEDE,
		END_MONSTER_TYPE
	};


	const real LevelScale = 0.5;
	const bool PLAYER_ENABLED = true;
	const bool POWERUPS_ENABLED = true;
	const bool MIRRORS_ENABLED = true;
	
	const bool LAVA_ENABLED = true;
	const bool MONSTER_AI_ENABLED = false;

	int MONSTER_TYPE = BARNACLE;
	bool CYCLE_MONSTER_TYPE = true;
	bool TRACK_MONSTER = true;

	class KeyboardMonsterController : public core::Component
	{
		public:
		
			KeyboardMonsterController(){}
			virtual ~KeyboardMonsterController(){}
			
			virtual void update( const time_state &time )
			{
				if ( MONSTER_AI_ENABLED ) return;
			
				InputDispatcher *input = InputDispatcher::get();
				Monster *monster = (Monster*)this->owner();
				GameLevel *level = static_cast<GameLevel*>(monster->level());
				
				MonsterController *controller = monster->controller();
				Player *player = level->player();

				if ( controller )
				{
					Vec2 dir = controller->enabled() ? controller->direction() : Vec2(0,0);
					if ( input->isKeyDown( app::KeyEvent::KEY_LEFTBRACKET ))
					{
						dir.x = -1;
					}
					
					if ( input->isKeyDown( app::KeyEvent::KEY_RIGHTBRACKET ))
					{
						dir.x = +1;
					}
					
					if ( input->isKeyDown( app::KeyEvent::KEY_BACKSLASH )) monster->attack( true );

					if ( input->isKeyDown( app::KeyEvent::KEY_a ))
					{
						real delta = 0.1;
						if ( input->isShiftDown() ) delta *= -1;
						controller->setAggression(controller->aggression() + delta );
					}

					if ( input->isKeyDown( app::KeyEvent::KEY_f ))
					{
						real delta = 0.1;
						if ( input->isShiftDown() ) delta *= -1;
						controller->setFear(controller->fear() + delta );
					}

					controller->setDirection( dir );
				}
			}
	};	
				
	Monster *monster_factory( Level *level, int type, const Vec2 &worldPosition )
	{
		switch( type )
		{
			case BARNACLE:
			{
				app::console() << level->time().time << " : CREATING BARNACLE" << std::endl;
			
				Barnacle::init barnacleInit;
				barnacleInit.size *= Rand::randFloat( 0.75, 1.25 );
				barnacleInit.position = worldPosition;
				
				Barnacle *barnacle = new Barnacle();
				barnacle->addComponent( new KeyboardMonsterController() );
				barnacle->initialize( barnacleInit );
				level->addObject( barnacle );
				
				barnacle->controller()->enable(MONSTER_AI_ENABLED);
				
				return barnacle;
				break;
			}

			case URCHIN:
			{
				app::console() << level->time().time << " : CREATING URCHIN" << std::endl;

				Urchin::init beetleInit;
				beetleInit.size *= Rand::randFloat( 0.75, 1.25 );
				beetleInit.position = worldPosition;
				
				Urchin *beetle = new Urchin();
				beetle->addComponent( new KeyboardMonsterController() );
				beetle->initialize( beetleInit );
				level->addObject( beetle );

				beetle->controller()->enable(MONSTER_AI_ENABLED);
				
				return beetle;
				break;
			}

			case GRUB:
			{
				app::console() << level->time().time << " : CREATING GRUB" << std::endl;

				Grub::init init;
				init.position = worldPosition;
				
				Grub *grub = new Grub();
				grub->initialize(init);
				grub->addComponent( new KeyboardMonsterController() );
				grub->controller()->enable( MONSTER_AI_ENABLED );
				level->addObject( grub );

				return grub;
				break;
			}

			case SHOGGOTH:
			{
				app::console() << level->time().time << " : CREATING SHOGGOTH" << std::endl;

				Shoggoth::init init;
				init.position = worldPosition;
				init.fluidInit.radius *= Rand::randFloat( 0.75, 1.25 );

				Shoggoth *shoggoth = new Shoggoth();
				shoggoth->addComponent( new KeyboardMonsterController() );
				shoggoth->initialize( init );
				level->addObject( shoggoth );

				shoggoth->controller()->enable(MONSTER_AI_ENABLED);

				return shoggoth;
				break;
			}

			case SHUB_NIGGURATH:
			{
				app::console() << level->time().time << " : CREATING SHUB_NIGGURATH" << std::endl;

				ShubNiggurath::init init;
				init.position = worldPosition;
//				init.maxGrubs = 0;
				

				ShubNiggurath *shub = new ShubNiggurath();
				shub->addComponent( new KeyboardMonsterController() );
				shub->initialize( init );
				level->addObject( shub );

				shub->controller()->enable(MONSTER_AI_ENABLED);

				return shub;
				break;
			}

			case YOG_SOTHOTH:
			{
				app::console() << level->time().time << " : CREATING YOG_SOTHOTH" << std::endl;

				YogSothoth::init init;
				init.position = worldPosition;
				init.fluidInit.radius *= Rand::randFloat( 0.75, 1.25 );

				YogSothoth *ys = new YogSothoth();
				ys->addComponent( new KeyboardMonsterController() );
				ys->initialize( init );
				level->addObject( ys );

				ys->controller()->enable(MONSTER_AI_ENABLED);

				return ys;
				break;
			}

			case CENTIPEDE:
			{
				app::console() << level->time().time << " : CREATING CENTIPEDE" << std::endl;

				Centipede::init init;
				init.position = worldPosition;

				Centipede *centipede = new Centipede();
				centipede->initialize(init);
				centipede->addComponent( new KeyboardMonsterController() );
				centipede->controller()->enable( MONSTER_AI_ENABLED );
				level->addObject( centipede );

				return centipede;
				break;
			}

		
		}
	
		return NULL;
	}
	
	Tongue *_testTongue = NULL;
	Barnacle *_testBarnacle = NULL;
	
}

/*
		std::vector< Monster* > _monsters;
		
		Font _font;
		
		cpBody *_mouseBody, *_draggingBody;
		cpConstraint *_mouseJoint;						
		Vec2 _mouseWorld, _previousMouseWorld;
*/

MonsterPlaygroundScenario::MonsterPlaygroundScenario():
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

MonsterPlaygroundScenario::~MonsterPlaygroundScenario()
{}

void MonsterPlaygroundScenario::setup()
{
	GameScenario::setup();

	_mouseBody = cpBodyNew(INFINITY, INFINITY);
	
	//
	//	Create the Level instance, and get the ResourceManager
	//
	
	GameLevel::init levelInit;
	levelInit.name = "Monster Playerground";
	levelInit.gravity = Vec2( 0, -9.8 );	
	levelInit.defaultZoom = 15;

	GameLevel *level = new GameLevel();
	level->initialize( levelInit );
	setLevel( level );

	//
	//	Create the terrain instance
	//	
	
	terrain::Terrain::init terrainInit;			
	terrainInit.scale = LevelScale;
	terrainInit.levelImage = "MonsterPlayground.png";
	terrainInit.materialTexture = "GreyTerrain.png";
	terrainInit.materialColor = Color( 0.2, 0.2, 0.2 );
	terrainInit.greebleTextureAtlas = "BlueTerrainGreebleAtlas.png";
	terrainInit.greebleSize = 0.25;
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

	if ( PLAYER_ENABLED )
	{
		Player::init playerInit;
		playerInit.health.initialHealth = playerInit.health.maxHealth = 100;
		playerInit.health.crushingInjuryMultiplier = 1;
		playerInit.height = 2.5;
		playerInit.width = 1.1;
		
		// by the powerups
		//playerInit.position = Vec2( 200,172 ) * LevelScale;

		//	by the lava
		//playerInit.position = Vec2( 205.899, 87.8 );

		//	by the test tongue
		playerInit.position = Vec2( 140, 94 );

		//	top right
		//playerInit.position = Vec2( 210, 146 );

		playerInit.cuttingBeamInit.maxRange = 200;
		playerInit.cuttingBeamInit.cutStrength = 0.1;
		playerInit.cuttingBeamInit.attackStrength = 1;
		playerInit.cuttingBeamInit.width = LevelScale;
		playerInit.cuttingBeamInit.cutWidth = 2*LevelScale;
		
		Player *player = new Player();
		player->initialize( playerInit );
		
		level->addObject( player );	
		
		//player->selectWeaponType( WeaponType::MAGNETO_BEAM );		
	}
	else if ( false )
	{
		Monster *monster = monster_factory( level, MONSTER_TYPE, Vec2(200,88) );
		
		if ( monster )
		{
			monster->aboutToBeDestroyed.connect( this, &MonsterPlaygroundScenario::_monsterWasDestroyed );
			_monsters.push_back( monster );
		}

	}
	
	if ( POWERUPS_ENABLED )
	{
		//
		//	Add some powerups to the top-right
		//
		
		PowerPlatform::init platformInit;
		platformInit.position = Vec2( 211,144 );
		platformInit.size = Vec2(4,2);
		PowerPlatform *platform = new PowerPlatform();
		platform->initialize( platformInit );
		level->addObject( platform );
					
		PowerUp::init medikitInit;
		medikitInit.amount = 10;
		medikitInit.type = PowerUpType::MEDIKIT;

		PowerUp *medikit = new PowerUp();
		medikitInit.position = Vec2(195,143);
		medikit->initialize(medikitInit);
		level->addObject( medikit );					

		medikit = new PowerUp();
		medikitInit.position = Vec2(199,143);
		medikit->initialize(medikitInit);
		level->addObject( medikit );


		PowerUp::init batteryInit;
		batteryInit.amount = 10;
		batteryInit.type = PowerUpType::BATTERY;

		PowerUp *battery = new PowerUp();
		batteryInit.position = Vec2(202,143);
		battery->initialize(batteryInit);
		level->addObject( battery );

		battery = new PowerUp();
		batteryInit.position = Vec2(206,144);
		battery->initialize(batteryInit);
		level->addObject( battery );


		//
		//	By the lava
		//
		
		batteryInit.position = Vec2( 198, 90 );
		battery = new PowerUp();
		battery->initialize( batteryInit );
		level->addObject( battery );

		batteryInit.position = Vec2( 198, 88 );
		battery = new PowerUp();
		battery->initialize( batteryInit );
		level->addObject( battery );

		batteryInit.position = Vec2( 198, 86 );
		battery = new PowerUp();
		battery->initialize( batteryInit );
		level->addObject( battery );


		//
		//	On the left valley 
		//

		battery = new PowerUp();
		batteryInit.position = Vec2(81.5,84);
		battery->initialize(batteryInit);
		level->addObject( battery );

		battery = new PowerUp();
		batteryInit.position = Vec2(83.7,83);
		battery->initialize(batteryInit);
		level->addObject( battery );

		medikit = new PowerUp();
		medikitInit.position = Vec2(88.9,82.3);
		medikit->initialize(medikitInit);
		level->addObject( medikit );
	}
	
	if ( MIRRORS_ENABLED )
	{
		Mirror::init mirrorInit;
		mirrorInit.position = Vec2( 80, 100 );
		Mirror *mirror = new Mirror();
		mirror->initialize( mirrorInit );
		level->addObject( mirror );
		
		mirrorInit.position = Vec2( 70,120 );
		mirror = new Mirror();
		mirror->initialize( mirrorInit );
		level->addObject( mirror );
		
	}



	//
	//	Add Lava
	//

	if ( LAVA_ENABLED )
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

		lavaInit.boundingVolumePosition = Vec2(216,85);
		lavaInit.boundingVolumeRadius = 5;
		
		Fluid *lava = new Fluid();
		lava->initialize( lavaInit );		
		level->addObject( lava );		
	}
		
	//
	//	We're good to go!
	//
					
	//level->setRenderMode( RenderMode::DEBUG );
	level->camera().lookAt( v2(cpBBCenter( level->bounds())), 10 );
}

void MonsterPlaygroundScenario::shutdown()
{
	GameScenario::shutdown();
}

void MonsterPlaygroundScenario::resize( Vec2i size )
{
	GameScenario::resize( size );
}

void MonsterPlaygroundScenario::update( const time_state &time )
{
	GameScenario::update( time );
	GameLevel *level = gameLevel();

	{
		cpVect mouseBodyPos = cpv(_mouseWorld),
			   mouseBodyVel = cpvmult(cpvsub(mouseBodyPos, cpv(_previousMouseWorld)), time.deltaT );

		cpBodySetPos( _mouseBody, mouseBodyPos );
		cpBodySetVel( _mouseBody, mouseBodyVel );
		_previousMouseWorld = v2(mouseBodyVel);
		
		if ( _draggingBody )
		{
			cpBodySetAngVel( _draggingBody, cpBodyGetAngVel(_draggingBody) * 0.9 );
		}		
	}

	if ( !level->cameraController()->tracking() )
	{
		const real dp = 10.0f;
		if ( isKeyDown( app::KeyEvent::KEY_UP ))	level->cameraController()->setPan( level->cameraController()->pan() + Vec2( 0,   -dp ));
		if ( isKeyDown( app::KeyEvent::KEY_DOWN ))	level->cameraController()->setPan( level->cameraController()->pan() + Vec2( 0,   +dp ));
		if ( isKeyDown( app::KeyEvent::KEY_LEFT ))	level->cameraController()->setPan( level->cameraController()->pan() + Vec2( +dp, 0 ));
		if ( isKeyDown( app::KeyEvent::KEY_RIGHT ))	level->cameraController()->setPan( level->cameraController()->pan() + Vec2( -dp, 0 ));		
	}
}

void MonsterPlaygroundScenario::draw( const render_state &state )
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

		gl::drawStringRight( msg.str(), Vec2(app::getWindowWidth() - 10, app::getWindowHeight() - 30 ), Color(1,1,1), _font );
	}
	
	if ( !_monsters.empty() )
	{
		Monster *monster = _monsters.back();

		std::stringstream msg;
		msg << "health: " << str(monster->health()->health(), 2) 
			<< " resistance: " << str( monster->health()->resistance(), 2 )
			<< " aggression: " << str(monster->controller() ? monster->controller()->aggression() : 0, 2 ) 
			<< " fear: " << str(monster->controller() ? monster->controller()->fear() : 0, 2 );
			
		gl::drawString( msg.str(), Vec2(10, app::getWindowHeight() - 40 ), Color(1,1,1), _font );
	}
}

#pragma mark - Input

bool MonsterPlaygroundScenario::mouseDown( const app::MouseEvent &event )
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

bool MonsterPlaygroundScenario::mouseUp( const app::MouseEvent &event )
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

bool MonsterPlaygroundScenario::mouseWheel( const app::MouseEvent &event )
{
	real wheelScale = 0.1,
		 dz = (event.getWheelIncrement() * wheelScale);

	_setZoom( _zoom() * (1+dz) );
	return true;
}

bool MonsterPlaygroundScenario::mouseMove( const app::MouseEvent &event, const Vec2 &delta )
{
	_mouseWorld = level()->camera().screenToWorld( event.getPos() );
	return false;
}

bool MonsterPlaygroundScenario::mouseDrag( const app::MouseEvent &event, const Vec2 &delta )
{
	GameLevel *level = gameLevel();
	
	_mouseWorld = level->camera().screenToWorld( event.getPos() );
	
	if ( level->player() ) return false;

	if ( isKeyDown( app::KeyEvent::KEY_SPACE ) )
	{
		level->cameraController()->setPan( level->cameraController()->pan() + delta );
		return true;
	}
	
	return false;
}	

bool MonsterPlaygroundScenario::keyDown( const app::KeyEvent &event )
{
	//app::console() << "MonsterPlaygroundScenario::keyDown code: " << event.getCode() << std::endl;

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
		
		case app::KeyEvent::KEY_BACKSPACE:
		{
			Monster *monster = _monsters.empty() ? NULL : _monsters.back();
			Player *player = gameLevel()->player();
			
			if ( monster && monster->alive() ) 
			{
				monster->health()->kill();
				_monsterWasDestroyed(monster);
			}
			else if ( false && _testBarnacle )
			{
				_testBarnacle->health()->kill();
				_testTongue = NULL;
				_testBarnacle = NULL;
			}
			else if ( _testTongue )
			{
				_testTongue->health()->kill();
				_testTongue = NULL;
			}
			else if ( player )
			{
				player->health()->kill();
			}


			return true;
		}
		
		case app::KeyEvent::KEY_BACKSLASH:
		{
			if ( _testTongue )
			{				
				_testTongue->releasePlayer();
			}
			
			break;
		}
		
		case app::KeyEvent::KEY_RETURN:
		{
			_spawnMonster();
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
		
		case app::KeyEvent::KEY_SLASH:
		{
			level()->setPaused( !level()->paused() );
			break;
		}
		
		case app::KeyEvent::KEY_RIGHTBRACKET:
		{
			if ( _testTongue ) _testTongue->setExtension( _testTongue->extension() + 0.1 );
			break;
		}

		case app::KeyEvent::KEY_LEFTBRACKET:
		{
			if ( _testTongue ) _testTongue->setExtension( _testTongue->extension() - 0.1 );
			break;
		}


		default: break;
	}
	
	return false;
}

bool MonsterPlaygroundScenario::keyUp( const app::KeyEvent &event )
{
	return false;
}

#pragma mark - Plumbing

real MonsterPlaygroundScenario::_zoom() const
{
	return gameLevel()->cameraController()->zoom();
}

void MonsterPlaygroundScenario::_setZoom( real z )
{
	GameLevel *level = gameLevel();
	Vec2 center;	
	
	if ( level->player() )
	{
		center = level->camera().worldToScreen( level->player()->position() );
	}
	else
	{
		center = mousePosition();
	}

	level->cameraController()->setZoom( std::max( z, real(0.1) ), center );
}

void MonsterPlaygroundScenario::_spawnMonster()
{
	Monster *monster = monster_factory( level(), MONSTER_TYPE, _mouseWorld );
	
	if ( monster )
	{
		monster->aboutToBeDestroyed.connect( this, &MonsterPlaygroundScenario::_monsterWasDestroyed );
		_monsters.push_back( monster );
	}
	
	if ( CYCLE_MONSTER_TYPE )
	{
		MONSTER_TYPE += 1;
		if ( MONSTER_TYPE == END_MONSTER_TYPE ) MONSTER_TYPE = 0;
	}
}

void MonsterPlaygroundScenario::_monsterWasDestroyed( GameObject *obj )
{
	_monsters.erase( std::remove( _monsters.begin(), _monsters.end(), obj ), _monsters.end() );
}
