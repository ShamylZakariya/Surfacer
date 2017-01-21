//
//  SurfaceGameLevel.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 7/7/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "GameLevel.h"

#include "Background.h"
#include "Fluid.h"
#include "GameAction.h"
#include "GameBehaviors.h"
#include "GameComponents.h"
#include "GameConstants.h"
#include "GameNotifications.h"
#include "Player.h"
#include "PlayerHud.h"
#include "Sensor.h"
#include "ViewportController.h"

#include <cinder/Rand.h>
#include <cinder/gl/Texture.h>

using namespace ci;
using namespace core;

namespace game {

namespace {

	Vec2 sparkInitialVelocity()
	{
		const real scale = 4;
		return Vec2( Rand::randFloat(-2*scale, 2*scale), scale );
	}

}


#pragma mark - Level

/*
		init						_initializer;
		terrain::Terrain				*_terrain;
		Player						*_player;
		Fluid						*_acid;

		UniversalParticleSystemController *_cutEffect, *_dustEffect, *_fireEffect, *_smokeEffect, *_monsterInjuryEffect;

		PlayerHud					*_playerHudLayerLayer;
		core::ui::Layer				*_notificationLayer;
		ActionDispatcher				*_actionDispatcher;
		ViewportController			*_cameraController;
*/

GameLevel::GameLevel():
	_terrain(NULL),
	_player(NULL),
	_acid(NULL),
	_cutEffect(NULL),
	_dustEffect(NULL),
	_fireEffect(NULL),
	_smokeEffect(NULL),
	_monsterInjuryEffect(NULL),
	_playerHudLayer(NULL),
	_notificationLayer(NULL),
	_actionDispatcher(new ActionDispatcher(this)),
	_cameraController(NULL)
{}

GameLevel::~GameLevel()
{
	delete _actionDispatcher;
}

void GameLevel::initialize( const init &initializer )
{
	Level::initialize(initializer);
	_initializer = initializer;
}

void GameLevel::addedToScenario( Scenario *scenario )
{}

void GameLevel::removedFromScenario( Scenario *s )
{
	if ( _playerHudLayer )
	{
		_playerHudLayer->dismiss(true);
		_playerHudLayer = NULL;
	}
	
	if ( _notificationLayer )
	{
		_notificationLayer->dismiss( true );
		_notificationLayer = NULL;
	}
}

cpBB GameLevel::bounds() const
{
	return _terrain ? _terrain->aabb() : Level::bounds();
}

void GameLevel::ready()
{
	_createEffectEmitters();
	Level::ready();

	_notificationLayer = new core::ui::Layer( "NotificationLayer" );
	_notificationLayer->setPadding(20);

	scenario()->uiStack()->pushTop( _notificationLayer );

	//
	//	Add gameplay behaviors
	//

	_cameraController = new ViewportController( camera(), ViewportController::Zeno );
	_cameraController->setConstraintMask( ViewportController::ConstrainPanning | ViewportController::ConstrainZooming );
	if ( player() ) 
	{
		_cameraController->setTrackingTarget( player() );
		_cameraController->setTrackingEnabled( true );
	}
	
	addBehavior( _cameraController );	
	addBehavior( new FluidInjuryDispatcher() );		
	addBehavior( new CrushingInjuryDispatcher() );	
	
	//
	//	Create default acid pool
	//
	
	Fluid::init acidInit;
	acidInit.attackStrength = AcidAttackStrength;
	acidInit.injuryType = InjuryType::ACID;
	acidInit.fluidParticleTexture = "AcidParticle.png";
	acidInit.fluidToneMapTexture = "AcidToneMap.png";
	acidInit.clumpingForce = 50;
			
	Fluid *acid = new Fluid();
	acid->initialize( acidInit );	
	setAcid( acid );	

	if ( terrain() )
	{
		collisionDispatcher()->contact( CollisionType::TERRAIN, CollisionType::TERRAIN )
			.connect( this, &GameLevel::_collision_Object_Terrain );

		collisionDispatcher()->contact( CollisionType::TERRAIN, CollisionType::DECORATION )
			.connect( this, &GameLevel::_collision_Object_Terrain );

		collisionDispatcher()->contact( CollisionType::MIRROR_CASING, CollisionType::TERRAIN )
			.connect( this, &GameLevel::_collision_Object_Terrain );

		collisionDispatcher()->contact( CollisionType::PLAYER, CollisionType::TERRAIN )
			.connect( this, &GameLevel::_collision_Object_Terrain );

		collisionDispatcher()->contact( CollisionType::MONSTER, CollisionType::MONSTER )
			.connect( this, &GameLevel::_collision_Monster_Monster );

		collisionDispatcher()->contact( CollisionType::MONSTER, CollisionType::DECORATION )
			.connect( this, &GameLevel::_collision_Monster_Object );

		terrain()->cutPerformed.connect( this, &GameLevel::_terrainWasCut );	
	}
}

void GameLevel::update( const core::time_state &time )
{
	//
	//	process any deferred geometry updates
	//

	if ( _terrain ) _terrain->updateGeometry( time );

	Level::update( time );	
}

void GameLevel::addObject( GameObject *object )
{
	switch( object->type() )
	{
		case GameObjectType::TERRAIN:
			_setTerrain( dynamic_cast<terrain::Terrain*>(object) );
			break;
			
		case GameObjectType::PLAYER:
			_setPlayer( dynamic_cast<Player*>(object));
			break;
			
		default:
			Level::addObject( object );
			break;
	}
}

void GameLevel::notificationReceived( const Notification &note )
{
	Level::notificationReceived(note);

	switch( note.identifier() )
	{
		case Notifications::SENSOR_TRIGGERED:
		{
			const sensor_event &Event = boost::any_cast< sensor_event >( note.message() );
			//app::console() << "GameLevel::notificationReceived - Notifications::SENSOR_TRIGGERED Event.eventName: " << Event.eventName << std::endl;

			if ( Event.triggered )
			{
				_actionDispatcher->eventStart( Event.eventName );
			}
			else
			{
				_actionDispatcher->eventEnd( Event.eventName );
			}
			
			break;
		}
	}	
} 

void GameLevel::setAcid( Fluid *acid )
{
	if ( _acid )
	{
		removeObject( _acid );
		delete _acid;
	}
	
	_acid = acid;
	
	if ( _acid )
	{
		addObject( _acid );
	}
}

void GameLevel::emitAcid( const Vec2 &position, const Vec2 &vel )
{
	Fluid::particle_init AcidParticle;
	AcidParticle.radius = 0.3;
	AcidParticle.density = 20;
	AcidParticle.visualRadiusScale = 2;
	AcidParticle.entranceDuration = 2;
	AcidParticle.exitDuration = 6;
	AcidParticle.lifespan = 10;
	AcidParticle.position = position;
	AcidParticle.initialVelocity = vel;

	_acid->emit( AcidParticle );
}

void GameLevel::emitAcidInCircularVolume( const Vec2 &position, real radius, const Vec2 &vel )
{
	Fluid::particle_init AcidParticle;
	AcidParticle.radius = 0.3;
	AcidParticle.density = 20;
	AcidParticle.visualRadiusScale = 2;
	AcidParticle.entranceDuration = 2;
	AcidParticle.exitDuration = 6;
	AcidParticle.lifespan = 10;
	AcidParticle.initialVelocity = vel;

	_acid->emitCircularVolume( AcidParticle, position, radius );
}

core::Object *GameLevel::create( const ci::JsonTree &recipe )
{
	const ci::JsonTree 
		&ObjectClass = recipe["class"],
		&ObjectInitializer = recipe["initializer"];

	bool enabled = true;
	core::util::read( recipe, "enabled", enabled );

	if ( enabled )
	{
		if ( core::util::isNull(ObjectClass) )
		{
			app::console() << "GameLevel::create - recipe has no 'class' for object to create;" 
				<< recipe << std::endl;

			return NULL;
		}

		if ( core::util::isNull(ObjectInitializer) )
		{
			app::console() << "GameLevel::create - recipe has no 'initializer' for object class: "
				<< ObjectClass.getValue() << std::endl;
			
			return NULL;
		}

		try {
		
			Object *obj = core::classload<Object>( ObjectClass.getValue(), ObjectInitializer );

			if ( obj )
			{
				GameObject *gameObj = dynamic_cast<core::GameObject*>(obj);				
				Behavior *behavior = dynamic_cast<core::Behavior*>(obj);

				if ( gameObj ) 
				{
					addObject( gameObj );
					return obj;
				}
				else if ( behavior ) 
				{
					addBehavior( behavior );
					return obj;
				}
				else
				{
					app::console() << "GameLevel::create - object class: " << ObjectClass.getValue() 
						<< " is not a GameObject or a Behavior, cannot add to Level"
						<< std::endl;
				}
			}
			else
			{
				app::console() << "GameLevel::create - unable to classload object \"" << ObjectClass.getValue() << "\"" << std::endl;
			}
		}
		catch( const std::exception &e )
		{
			app::console() << "GameLevel::create - Failure loading object " << ObjectClass.getValue() << "; exception: " << e.what() << std::endl;
		}
	}
	
	return NULL;
}

#pragma mark - Special-cases

void GameLevel::_setTerrain( terrain::Terrain *t )
{
	if ( _terrain )
	{
		Level::removeObject( _terrain );
		delete _terrain;
	}
	
	_terrain = t;
	
	if ( _terrain )
	{
		Level::addObject( _terrain );
	}
}

void GameLevel::_setPlayer( Player *p )
{
	if ( _player )
	{
		Level::removeObject( _player );
		delete _player;
	}
	
	_player = p;
	if ( _player )
	{
		Level::addObject( _player );

		//
		//	Create gameplay overlay
		//
		
		if ( !_playerHudLayer )
		{
			_playerHudLayer = new PlayerHud( scenario() );
			scenario()->uiStack()->pushBottom( _playerHudLayer );
		}
		
		//
		//	Inform camera controller to track player
		//

		if ( _cameraController ) _cameraController->setTrackingTarget( _player );		
	}
	else if ( _playerHudLayer )
	{
		_playerHudLayer->dismiss(true);
		_playerHudLayer = NULL;
	}
}

#pragma mark - Particle Emissions

void GameLevel::_collision_Object_Terrain( const collision_info &info, bool &discard )
{
	const real 
		MaxVel = 10,
		BodyRelVel = info.relativeVelocity(),
		SlideVel = info.slideVelocity().length(),
		Vel = std::min( BodyRelVel + SlideVel, MaxVel );
		
	if ( Vel < 0.25 ) return;
		
	const int 
		Count = static_cast<int>(std::ceil(4 * Vel / MaxVel));

	const Vec2 
		Right( rotateCW(info.normal()) * Vel ),
		Left( -Right.x, -Right.y );
				
	for ( int i = 0; i < Count; i++ )
	{
		_dustEffect->defaultEmitter()->emit( 1, info.position(), Right );
		_dustEffect->defaultEmitter()->emit( 1, info.position(), Left );
	}
}

void GameLevel::_collision_Monster_Monster( const collision_info &info, bool &discard )
{
	const int
		aType = info.a->type(),
		bType = info.b->type();
		
	//
	//	PRevent grubs from colliding with any kind of monster
	//
	
	if ( ((aType == GameObjectType::GRUB) || (bType == GameObjectType::GRUB)) && 
	     ( aType != bType ))
	{
		discard = true;
		return;
	}

	//
	//	Prevent tentacles from colliding with any kind of monster -- tentacles are complex, and collisions with them can be unstable
	//

	if ( aType == GameObjectType::SHUB_NIGGURATH_TENTACLE || bType == GameObjectType::SHUB_NIGGURATH_TENTACLE )
	{
		discard = true;
		return;
	}
	
	//
	//	Prevent barnacles from colliding, for same reason
	//
	
	if ( aType == GameObjectType::BARNACLE || bType == GameObjectType::BARNACLE )
	{
		discard = true;
		return;
	}
	
	//
	//	Prevent collisions between fluid monsters
	//
	
	const bool 
		aIsFluidMonster = 
			aType == GameObjectType::SHOGGOTH || 
			aType == GameObjectType::YOG_SOTHOTH ||
			aType == GameObjectType::SHUB_NIGGURATH,

		bIsFluidMonster = 
			bType == GameObjectType::SHOGGOTH || 
			bType == GameObjectType::YOG_SOTHOTH ||
			bType == GameObjectType::SHUB_NIGGURATH;
		
	
	if ( aIsFluidMonster && bIsFluidMonster &&
	     info.a != info.b )
	{
		discard = true;
		return;
	}	
}

void GameLevel::_collision_Monster_Object( const core::collision_info &info, bool &discard )
{
	switch ( info.b->type() )
	{
		case GameObjectType::POWERUP:
			discard = true;
			break;
			
		default: break;
	}
}

void GameLevel::_terrainWasCut( const Vec2rVec &positions, TerrainCutType::cut_type cutType )
{
	int count = 1;
	UniversalParticleSystemController::Emitter *emitter = _cutEffect->defaultEmitter();

	switch (cutType) 
	{
		case TerrainCutType::FRACTURE:
			emitter = _dustEffect->defaultEmitter();
			count = 64;
			break;

		default:
			break;
	}

	foreach( const Vec2 &position, positions )
	{
		emitter->emit( count, position );
	}
}

void GameLevel::_createEffectEmitters()
{
	const real 
		Mutability = 0.25,
		CutEffectSize = 0.5,
		DustEffectSize = 1,
		FireEffectSize = 1,
		SmokeEffectSize = 2,
		MonsterInjuryEffectSize = 1;
		
	//
	//	create the cut effect
	//

	{
		UniversalParticleSystemController::particle_state sparkParticle;
		sparkParticle.orientsToVelocity = true;
		sparkParticle.velocityScaling = 3;
		sparkParticle.gravity.min = -1;
		sparkParticle.gravity.max = +10;
		sparkParticle.radius = CutEffectSize * 0.1;
		sparkParticle.additivity = 1;
		sparkParticle.lifespan = 2;
		sparkParticle.color = Vec4( 1.0,0.6,1.0,1.0 );
		sparkParticle.color.max.w = 0;
		sparkParticle.initialVelocity = sparkInitialVelocity;
		sparkParticle.atlasIdx = 0;

		UniversalParticleSystemController::particle_state fireParticle;
		fireParticle.orientsToVelocity = false;
		fireParticle.damping = 0.9;
		fireParticle.gravity.min = 0;
		fireParticle.gravity.max = 0;
		fireParticle.radius.min = CutEffectSize * 1;
		fireParticle.radius.max = CutEffectSize * 3;
		fireParticle.additivity.min = 1;
		fireParticle.additivity.max = 1;
		fireParticle.lifespan = 2;
		fireParticle.color = Vec4( 0.8,0.5,1.0,0.2 );
		fireParticle.color.max.w = 0;
		fireParticle.atlasIdx = 3;

		UniversalParticleSystemController::particle_state smokeParticle;
		smokeParticle.orientsToVelocity = false;
		smokeParticle.damping = 0.9;
		smokeParticle.gravity.min = 0;
		smokeParticle.gravity.max = 0;
		smokeParticle.radius.min = CutEffectSize * 1;
		smokeParticle.radius.max = CutEffectSize * 6;
		fireParticle.additivity.min = 0.5;
		fireParticle.additivity.max = 0;
		smokeParticle.lifespan = 4;
		smokeParticle.color.min = Vec4( 0.5,0.5,0.5,0.125 );
		smokeParticle.color.max = Vec4( 0.2,0.2,0.2,0 );
		smokeParticle.angularVelocity = M_PI / 8;
		smokeParticle.atlasIdx = 3;


		const real Mutability = 0.25;
		UniversalParticleSystemController::init controllerInit;
		controllerInit.templates.push_back( UniversalParticleSystemController::particle_template( sparkParticle,128, Mutability ));
		controllerInit.templates.push_back( UniversalParticleSystemController::particle_template( fireParticle, 128, Mutability ));
		controllerInit.templates.push_back( UniversalParticleSystemController::particle_template( smokeParticle, 16, Mutability ));
			
		UniversalParticleSystemController *controller = new UniversalParticleSystemController();
		controller->initialize( controllerInit );

		gl::Texture::Format particleFormat;
		particleFormat.enableMipmapping(true);
		particleFormat.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );
		particleFormat.setMagFilter( GL_LINEAR );	
		particleFormat.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );

		ParticleSystem::init systemInit;
		systemInit.addTexture( resourceManager()->getTexture( "CutEffectParticleAtlas.png", particleFormat )); 
		systemInit.atlasType = ParticleAtlasType::TwoByTwo;

		ParticleSystem *system = new ParticleSystem();
		system->initialize( systemInit );
		system->setController( controller );
		addObject(system);
		
		_cutEffect = controller;
	}
	
	//
	//	create dust effect
	//
	
	{
		const Color TerrainMaterialColor = terrain() ? terrain()->initializer().materialColor : Color(1,0,1);
		const real Mutability = 0.25;
		const Vec3 TerrainColor( TerrainMaterialColor.r, TerrainMaterialColor.g, TerrainMaterialColor.b );
		UniversalParticleSystemController::init controllerInit;

		UniversalParticleSystemController::particle_state smokeParticle;
		smokeParticle.orientsToVelocity = false;
		smokeParticle.fade.min = 0.2;
		smokeParticle.fade.max = 0.2;
		smokeParticle.damping = 0.95;
		smokeParticle.gravity = 0;
		smokeParticle.radius.min = DustEffectSize * 2;
		smokeParticle.radius.max = DustEffectSize * 4;
		smokeParticle.additivity = 0;
		smokeParticle.lifespan = 4;
		smokeParticle.color.min = Vec4( TerrainColor,0.75 );
		smokeParticle.color.max = Vec4( TerrainColor,0.00 );		
		smokeParticle.atlasIdx = 0;
		smokeParticle.angularVelocity = (Rand::randBool() ? 1 : -1) * Rand::randFloat( -0.1, 0.2 );
		controllerInit.templates.push_back( UniversalParticleSystemController::particle_template( smokeParticle, 256, Mutability ));

		UniversalParticleSystemController::particle_state rubbleParticle;
		rubbleParticle.orientsToVelocity = false;
		rubbleParticle.fade.min = 0.1;
		rubbleParticle.fade.max = 0.9;
		rubbleParticle.damping = 0.99;
		rubbleParticle.gravity.min = 0;
		rubbleParticle.gravity.max = +1.0;
		rubbleParticle.radius = DustEffectSize * 0.25;
		rubbleParticle.additivity = 0;
		rubbleParticle.lifespan = 2;
		rubbleParticle.color = Vec4( TerrainColor,1 );
		rubbleParticle.atlasIdx = 1;
		rubbleParticle.angularVelocity = Rand::randFloat( -10, 10 );
		controllerInit.templates.push_back( UniversalParticleSystemController::particle_template( rubbleParticle, 85, Mutability ));

		rubbleParticle.atlasIdx = 2;
		controllerInit.templates.push_back( UniversalParticleSystemController::particle_template( rubbleParticle, 85, Mutability ));

		rubbleParticle.atlasIdx = 3;
		controllerInit.templates.push_back( UniversalParticleSystemController::particle_template( rubbleParticle, 86, Mutability ));


		UniversalParticleSystemController *controller = new UniversalParticleSystemController();
		controller->initialize( controllerInit );
		
		gl::Texture::Format particleFormat;
		particleFormat.enableMipmapping(true);
		particleFormat.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );
		particleFormat.setMagFilter( GL_LINEAR );	
		particleFormat.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );

		ParticleSystem::init systemInit;
		systemInit.addTexture( resourceManager()->getTexture( "SmokeAndRubbleAtlas.png", particleFormat )); 
		systemInit.atlasType = ParticleAtlasType::TwoByTwo;

		ParticleSystem *system = new ParticleSystem();
		system->initialize( systemInit );
		system->setController( controller );
		addObject(system);


		_dustEffect = controller;
	}
	
	//
	//	Create fire effect
	//

	{
		UniversalParticleSystemController::particle_state fireParticle;

		fireParticle.orientsToVelocity = false;
		fireParticle.radius.min = FireEffectSize * 2;
		fireParticle.radius.max = 0;
		fireParticle.damping = 0.9;
		fireParticle.gravity.min = 0;
		fireParticle.gravity.max = 0;
		fireParticle.additivity.min = 1;
		fireParticle.additivity.max = 1;
		fireParticle.lifespan = 1;
		fireParticle.color.min = Vec4( 1.0,0.9,0.7,0.25 );
		fireParticle.color.max = Vec4( 1.0,0.2,0.0,0.5 );
		fireParticle.atlasIdx = 1;		

		UniversalParticleSystemController::particle_state smokeParticle;
		smokeParticle.orientsToVelocity = false;
		smokeParticle.radius.min = FireEffectSize;
		smokeParticle.radius.max = FireEffectSize * 4;
		smokeParticle.damping = 0.99;
		smokeParticle.gravity.min = -0.1;
		smokeParticle.gravity.max = -1.0;
		smokeParticle.additivity = 0;
		smokeParticle.lifespan = 2;
		smokeParticle.color.min = Vec4( 0.1,0.1,0.1,0.2 );
		smokeParticle.color.max = Vec4( 0.4,0.4,0.4,0.0  );
		smokeParticle.angularVelocity = M_PI / 8;
		smokeParticle.atlasIdx = 0;


		const real Mutability = 0.25;
		UniversalParticleSystemController::init controllerInit;
		controllerInit.templates.push_back( UniversalParticleSystemController::particle_template( fireParticle, 128, Mutability ));
		controllerInit.templates.push_back( UniversalParticleSystemController::particle_template( smokeParticle, 128, Mutability ));
			
		UniversalParticleSystemController *controller = new UniversalParticleSystemController();
		controller->initialize( controllerInit );

		gl::Texture::Format particleFormat;
		particleFormat.enableMipmapping(true);
		particleFormat.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );
		particleFormat.setMagFilter( GL_LINEAR );	
		particleFormat.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );

		ParticleSystem::init systemInit;
		systemInit.addTexture( resourceManager()->getTexture( "InjuryEffectParticleAtlas.png", particleFormat )); 
		systemInit.atlasType = ParticleAtlasType::TwoByTwo;

		ParticleSystem *system = new ParticleSystem();
		system->initialize( systemInit );
		system->setController( controller );
		addObject(system);
		
		_fireEffect = controller;
	}
	
	//
	//	Create smoke effect
	//

	{
		const real Mutability = 0.25;
		UniversalParticleSystemController::init controllerInit;

		UniversalParticleSystemController::particle_state smokeParticle;
		smokeParticle.orientsToVelocity = false;
		smokeParticle.damping = 0.9;
		smokeParticle.gravity.min = 0;
		smokeParticle.gravity.max = 0;
		smokeParticle.radius.min = SmokeEffectSize * 0.25;
		smokeParticle.radius.max = SmokeEffectSize * 1;
		smokeParticle.additivity = 0;
		smokeParticle.lifespan = 3;
		smokeParticle.color.min = Vec4( 0.5,0.5,0.5,0.5 );
		smokeParticle.color.max = Vec4( 0.2,0.2,0.2,0 );
		smokeParticle.angularVelocity = M_PI / 8;
		smokeParticle.atlasIdx = 0;


		controllerInit.templates.push_back( UniversalParticleSystemController::particle_template( smokeParticle, 128, Mutability ));

		smokeParticle.damping = 0.8;
		smokeParticle.radius.min = SmokeEffectSize * 1;
		smokeParticle.radius.max = SmokeEffectSize * 2;
		smokeParticle.lifespan = 2;
		smokeParticle.color.min = Vec4( 0.3,0.3,0.3,0.5 );
		smokeParticle.color.max = Vec4( 0,0,0,0 );
		smokeParticle.angularVelocity = -M_PI / 4;
		controllerInit.templates.push_back( UniversalParticleSystemController::particle_template( smokeParticle, 128, Mutability ));

			
		UniversalParticleSystemController *controller = new UniversalParticleSystemController();
		controller->initialize( controllerInit );

		gl::Texture::Format particleFormat;
		particleFormat.enableMipmapping(true);
		particleFormat.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );
		particleFormat.setMagFilter( GL_LINEAR );	
		particleFormat.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );

		ParticleSystem::init systemInit;
		systemInit.addTexture( resourceManager()->getTexture( "InjuryEffectParticleAtlas.png", particleFormat )); 
		systemInit.atlasType = ParticleAtlasType::TwoByTwo;

		ParticleSystem *system = new ParticleSystem();
		system->initialize( systemInit );
		system->setController( controller );
		addObject(system);
		
		_smokeEffect = controller;
	}
	
	//
	//	Create monster injury effect
	//

	{
		UniversalParticleSystemController::init controllerInit;

		UniversalParticleSystemController::particle_state spatterParticle;
		spatterParticle.orientsToVelocity = false;
		spatterParticle.damping = 0.9;
		spatterParticle.gravity.min = 0;
		spatterParticle.gravity.max = 1;
		spatterParticle.radius.min = MonsterInjuryEffectSize * 1;
		spatterParticle.radius.max = MonsterInjuryEffectSize * 2;
		spatterParticle.additivity = 0;
		spatterParticle.lifespan = 2;
		spatterParticle.color.min = Vec4( 0.1,0.5,0.1,0.5 );
		spatterParticle.color.max = Vec4( 0,0.25,0,0 );
		spatterParticle.angularVelocity = M_PI;
		spatterParticle.atlasIdx = 3;

		controllerInit.templates.push_back( UniversalParticleSystemController::particle_template( spatterParticle, 128, Mutability ));

		spatterParticle.damping = 0.8;
		spatterParticle.radius.min = MonsterInjuryEffectSize * 0.5;
		spatterParticle.radius.max = MonsterInjuryEffectSize * 1;
		spatterParticle.lifespan = 1;
		spatterParticle.color.min = Vec4( 0,0.35,0,0.75 );
		spatterParticle.color.max = Vec4( 0,0.125,0,0 );
		spatterParticle.angularVelocity = -M_PI / 2;
		controllerInit.templates.push_back( UniversalParticleSystemController::particle_template( spatterParticle, 128, Mutability ));

			
		UniversalParticleSystemController *controller = new UniversalParticleSystemController();
		controller->initialize( controllerInit );

		gl::Texture::Format particleFormat;
		particleFormat.enableMipmapping(true);
		particleFormat.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );
		particleFormat.setMagFilter( GL_LINEAR );	
		particleFormat.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );

		ParticleSystem::init systemInit;
		systemInit.addTexture( resourceManager()->getTexture( "InjuryEffectParticleAtlas.png", particleFormat )); 
		systemInit.atlasType = ParticleAtlasType::TwoByTwo;

		ParticleSystem *system = new ParticleSystem();
		system->initialize( systemInit );
		system->setController( controller );
		addObject(system);
		
		_monsterInjuryEffect = controller;
	}
}


} //end namespace game
