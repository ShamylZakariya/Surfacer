//
//  PowerUp.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 2/24/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "PowerUp.h"

#include "Fluid.h"
#include "GameLevel.h"
#include "GameConstants.h"
#include "GameNotifications.h"
#include "PerlinNoise.h"
#include "Player.h"

#include <cinder/gl/gl.h>


using namespace ci;
using namespace core;
namespace game {

namespace PowerUpType 
{

	std::string toString( type t )
	{
		switch( t )
		{
			case MEDIKIT: return "MEDIKIT"; break;
			case BATTERY: return "BATTERY"; break;
		}
		
		return "UNKNOWN";
	}
	
	type fromString( const std::string &t )
	{
		if ( util::stringlib::lowercase(t) == "medikit" ) return MEDIKIT;
		return BATTERY;
	}

}

CLASSLOAD(PowerUp);

/*
		init _initializer;
		cpBody *_body;
		cpShape *_shape;
		cpConstraint *_antiRotationConstraint;
		real _amount, _opacity;

		UniversalParticleSystemController *_effectPsc;
		UniversalParticleSystemController::Emitter *_effectEmitter;
*/

PowerUp::PowerUp():
	GameObject( GameObjectType::POWERUP ),
	_body(NULL),
	_shape(NULL),
	_amount(0),
	_opacity(1),
	_fireEmitter(NULL)
{
	setName( "Uninitialized PowerUp" );
	setLayer(RenderLayer::ENVIRONMENT);
	addComponent( new PowerUpRenderer());
	setVisibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING );
}

PowerUp::~PowerUp()
{
	aboutToDestroyPhysics(this);
	cpConstraintCleanupAndFree( &_antiRotationConstraint );
	cpShapeCleanupAndFree( &_shape );
	cpBodyCleanupAndFree( &_body );
}
		
void PowerUp::initialize( const init &i )
{
	GameObject::initialize(i);
	_initializer = i;
	_amount = _initializer.amount;

	switch( _initializer.type )
	{
		case PowerUpType::MEDIKIT:
			setName( "Medikit" );
			break;
			
		case PowerUpType::BATTERY:
			setName( "Battery" );
			break;
			
		defult:
			break;
	}
}
		
void PowerUp::addedToLevel( Level *level )
{
	const real 
		Radius = _initializer.size * 0.5 * 2, // extra padding!
		Density = 500, 
		Mass = M_PI * Radius * Radius * Density,
		Moment = cpMomentForCircle(Mass, 0, Radius, cpvzero);
		
	cpSpace *space = level->space();

	_body = cpBodyNew(Mass, Moment);
	cpBodySetPos( _body, cpv(_initializer.position) );	
	
	_shape = cpCircleShapeNew( _body, Radius, cpvzero );
	cpShapeSetUserData( _shape, this );	
	cpShapeSetCollisionType( _shape, CollisionType::DECORATION );
	cpShapeSetLayers( _shape, CollisionLayerMask::POWERUP );
	cpShapeSetFriction( _shape, 1 );
	cpShapeSetElasticity( _shape, 0 );

	_antiRotationConstraint = cpGearJointNew(cpSpaceGetStaticBody(space), _body, 0, 1);

	cpSpaceAddBody( space, _body );
	cpSpaceAddShape( space, _shape );
	cpSpaceAddConstraint( space, _antiRotationConstraint );
	
	level->collisionDispatcher()->contact(CollisionType::PLAYER, CollisionType::DECORATION, NULL, this )
		.connect( this, &PowerUp::_contactPlayer );

	level->collisionDispatcher()->postSolve(CollisionType::FLUID, CollisionType::DECORATION, NULL, this )
		.connect( this, &PowerUp::_contactFluid );
}

void PowerUp::ready()
{
	//
	//	Get an emitter for the global fire effect
	//
	
	GameLevel *gameLevel = static_cast<GameLevel*>(level());
	_fireEmitter = gameLevel->fireEffect()->emitter(16);	
}

void PowerUp::update( const time_state &time )
{
	if ( _amount < Epsilon )
	{
		if ( _opacity < ALPHA_EPSILON )
		{
			setFinished(true);
		}
		else
		{
			_opacity *= 0.9;
		}	
	}

	if ( empty() )
	{
		// dampen body movement
		const real Damp = 0.9;
		cpBodySetVel( _body, cpvmult(cpBodyGetVel(_body), Damp ));
		cpBodySetAngVel( _body, cpBodyGetAngVel(_body) * Damp );
	}

	setAabb( cpShapeGetBB( _shape ));
}

real PowerUp::discharge()
{
	if ( _amount > 0 )
	{
		onDischarge(this);
		
		real amount = _amount;
		_amount = 0;

		return amount;
	}
	
	return 0;
}

void PowerUp::_contactPlayer( const collision_info &info, bool &discard )
{
	if ( amount() > 0 )
	{
		Player *player = (Player*) info.a;
		assert( player == ((GameLevel*)level())->player() );
		assert( (PowerUp*)info.b == this );

		cpShapeSetLayers( _shape, CollisionLayerMask::TRIGGERED_POWERUP );
			
		if ( player->alive() )
		{
			discharge();
				
			switch( powerupType() )
			{
				case PowerUpType::MEDIKIT:
				{
					HealthComponent *health = player->health();
					health->heal( amount() );			
					notificationDispatcher()->broadcast(Notification(Notifications::PLAYER_HEALTH_BOOST));

					break;
				}

				case PowerUpType::BATTERY:
				{
					Battery *battery = player->battery();
					battery->setPower( battery->power() + amount() );
					notificationDispatcher()->broadcast(Notification(Notifications::PLAYER_POWER_BOOST));

					break;
				}
			}
		}
		
		discard = true;
	}
}

void PowerUp::_contactFluid( const collision_info &info )
{
	if ( _amount > 0 )
	{
		assert( (PowerUp*)info.b == this );
		Fluid *fluid = (Fluid*) info.a;
		
		if ( (fluid->attackStrength() > 0) && (fluid->injuryType() == InjuryType::LAVA))
		{
			const real 
				Injury = level()->time().deltaT * fluid->attackStrength();

			_amount = std::max<real>( _amount - Injury, 0 );
			
			const Vec2r Pos = v2r(cpBodyGetPos( _body ));
			const real Size = _initializer.size * 0.75;
			
			for ( int i = 0, N = info.contacts(); i < N; i++ )
			{
				_fireEmitter->emit( 1, Pos + Vec2r( Rand::randFloat( -Size , Size ), Rand::randFloat( -Size, Size )));
			}
		}
	}
}

#pragma mark - PowerUpRenderer

/*
		ci::gl::GlslProg _svgShader;
		core::SvgObject _svg, _powerup, _glow;
		Vec2r _svgScale;
		real _dischargeFlare, _pulseOffset, _pulseFrequency;
		seconds_t _dischargeTime;
*/

PowerUpRenderer::PowerUpRenderer():
	_dischargeFlare(0),
	_pulseOffset( Rand::randFloat(M_PI) ),
	_pulseFrequency( 0.25 ),
	_dischargeTime(-1)
{}

PowerUpRenderer::~PowerUpRenderer()
{}

void PowerUpRenderer::addedToGameObject( GameObject *obj )
{
	powerUp()->onDischarge.connect( this, &PowerUpRenderer::_powerUpDischarged );
}

void PowerUpRenderer::update( const time_state &time )
{
	if ( _dischargeTime > 0 )
	{
		_dischargeFlare = lrp<real>( 0.2, _dischargeFlare, 1 );
	}
	else
	{
		_dischargeFlare = 0;
	}	
}

void PowerUpRenderer::_drawGame( const render_state &state )
{
	PowerUp *powerup = this->powerUp();

	if ( !_svgShader )
	{
		_svgShader = level()->resourceManager()->getShader( "SvgPassthroughShader" );
		
		switch( powerup->powerupType() )
		{
			case PowerUpType::MEDIKIT:
				_svg = level()->resourceManager()->getSvg( "Medikit.svg" );
				break;

			case PowerUpType::BATTERY:
				_svg = level()->resourceManager()->getSvg( "Battery.svg" );
				break;
		}
		
		_powerup = _svg.find("PowerUp");
		assert( _powerup );

		_glow = _svg.find("Glow");
		//_glow.setBlendMode( BlendMode( GL_SRC_ALPHA, GL_ONE ));
		assert( _glow );
		

		// medikit and battery cell are both square
		Vec2r documentSize = _svg.documentSize();
		_svgScale.x = _svgScale.y = powerup->initializer().size / documentSize.x;
	}


	_svgShader.bind();
	
	_svg.setPosition( v2r( cpBodyGetPos(powerup->body()) ));
	_svg.setAngle( cpBodyGetAngle(powerup->body()));
	_svg.setScale( _svgScale );

	_powerup.setOpacity( powerup->_opacity );
	
	const real 
		Charge = powerup->charge(),
		Flicker = _currentFlickerValue();
		
	real
		glowOpacity = Charge * lrp<real>(Flicker, 0.125,0.25),
		glowScale = Charge * lrp<real>( Flicker, 1, 1.75 );
		
	if ( _dischargeFlare < 0.5 ) glowOpacity = lrp<real>( _dischargeFlare / 0.5, glowOpacity, 0.5 );
	else glowOpacity = lrp<real>( (_dischargeFlare - 0.5)/0.5, 0.5, 0 );

	glowScale = lrp<real>( _dischargeFlare, glowScale, 8 );

	_glow.setOpacity( glowOpacity );
	_glow.setScale( glowScale );
	
	gl::enableAlphaBlending();
	_svg.draw( state );
	
	
	_svgShader.unbind();
}

void PowerUpRenderer::_drawDebug( const render_state &state )
{
	PowerUp *powerup = this->powerUp();
	Mat4r R;
	cpBody_ToMatrix( powerup->body(), R );
	
	real hs = powerup->initializer().size * 0.5;
	
	gl::pushMatrices();
	gl::multModelView( R );
	
		gl::color( powerup->debugColor() );
		gl::drawSolidCircle( Vec2r(0,0), powerup->initializer().size * 0.5, 12 );
		
	gl::popMatrices();
}

void PowerUpRenderer::_powerUpDischarged( PowerUp * )
{
	_dischargeTime = level()->time().time;
}

real PowerUpRenderer::_currentFlickerValue()
{
	real flicker = std::abs( std::sin( _pulseOffset + (level()->time().time * M_PI * _pulseFrequency)));
	return flicker * flicker * flicker;
}

} // end namespace game
