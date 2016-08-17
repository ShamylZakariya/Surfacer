#pragma once

//
//  PowerUp.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 2/24/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Core.h"
#include "ParticleSystem.h"

namespace game {

namespace PowerUpType 
{
	enum type 
	{	
		MEDIKIT,
		BATTERY	
	};
	
	std::string toString( type );
	type fromString( const std::string & );
	
}

class PowerUp : public core::GameObject
{
	public:
	
	
		struct init : public core::GameObject::init
		{
			PowerUpType::type type;
			real amount;
			real size;
			Vec2r position;
			
			init():
				type( PowerUpType::MEDIKIT ),
				amount(1),
				size(1),
				position(-100,-100)
			{}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				core::GameObject::init::initialize(v);
				JSON_ENUM_READ(v, PowerUpType, type );
				JSON_READ(v,amount);
				JSON_READ(v,size);
				JSON_READ(v,position);
			}

			
						
		};
		
		/**
			Signal emitted when a powerup discharges into player
		*/
		signals::signal< void( PowerUp* ) > onDischarge;
		
	public:
	
		PowerUp();
		virtual ~PowerUp();

		JSON_INITIALIZABLE_INITIALIZE();
		
		// initialization
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
		
		// GameObject
		virtual void addedToLevel( core::Level *level );
		virtual void ready();
		virtual void update( const core::time_state &time );
		
		// PowerUp
		cpBody *body() const { return _body; }
		cpShape *shape() const { return _shape; }
		
		/**
			Return the type of powerup this is
		*/
		PowerUpType::type powerupType() const { return _initializer.type; }
		
		/**
			discharge this PowerUp, returning how much power/health was removed if the PowerUp was active,
			or zero if it was walready empty.
		*/
		real discharge();

		/**
			Return the amount of health/batterycharge that this PowerUp has. Note, the
			value remains after discharge.
		*/
		real amount() const { return _amount; }
		
		/**
			Return value from 0->1 showing how "charged" this powerup is.
		*/
		real charge() const { return _amount / _initializer.amount; }
		
		bool empty() const { return _amount < Epsilon; }
		
		
	protected:

		virtual void _contactPlayer( const core::collision_info &info, bool &discard );
		virtual void _contactFluid( const core::collision_info &info );
		
	private:
	
		friend class PowerUpRenderer;
	
		init _initializer;
		cpBody *_body;
		cpShape *_shape;
		cpConstraint *_antiRotationConstraint;

		bool _active;
		real _amount, _opacity;

		UniversalParticleSystemController::Emitter *_fireEmitter;

};

class PowerUpRenderer : public core::DrawComponent
{
	public:
	
		PowerUpRenderer();
		virtual ~PowerUpRenderer();
		
		PowerUp *powerUp() const { return (PowerUp*) owner(); }
		
		void addedToGameObject( core::GameObject * );
		void update( const core::time_state &time );
		
	protected:

		virtual void _drawGame( const core::render_state & );
		virtual void _drawDebug( const core::render_state & );
		
		void _powerUpDischarged( PowerUp * );
		real _currentFlickerValue();
		
	private:

		ci::gl::GlslProg _svgShader;
		core::SvgObject _svg, _powerup, _glow;
		Vec2r _svgScale;
		real _dischargeFlare, _pulseOffset, _pulseFrequency;
		seconds_t _dischargeTime;
};



} // end namespace game