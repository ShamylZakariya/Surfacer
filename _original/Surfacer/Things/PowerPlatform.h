#pragma once

//
//  PowerPlatform.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/4/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Core.h"

namespace game {

class PowerCell;

class PowerPlatform : public core::GameObject
{
	public:
	
		struct init : public core::GameObject::init
		{
			Vec2 position;
			Vec2 size;
			int chargeLevel;
			int maxChargeLevel;
			
			init():
				position(0,0),
				size(4,2),
				chargeLevel(0),
				maxChargeLevel(10)
			{}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				core::GameObject::init::initialize(v);
				JSON_READ(v,position);
				JSON_READ(v,size);
				JSON_READ(v,chargeLevel);
				JSON_READ(v,maxChargeLevel);
			}
									
		};
		
		signals::signal< void(PowerPlatform*) > chargeChanged;
		signals::signal< void(PowerPlatform*) > fullyCharged;

	public:
	
		PowerPlatform();
		virtual ~PowerPlatform();

		JSON_INITIALIZABLE_INITIALIZE();
		
		virtual void initialize( const init &i );
		const init &initializer() const { return _initializer; }

		// GameObject
		virtual void addedToLevel( core::Level *level );
		virtual void update( const core::time_state &time );
		virtual Vec2 position() const { return v2(cpBodyGetPos(_body)); }

		// PowerPlatform
		cpBody *body() const { return _body; }
		cpShape *shape() const { return _shape; }
		

		int chargeLevel() const { return static_cast<int>(_chargeLevel); }
		virtual void setChargeLevel( int cl );

		int maxChargeLevel() const { return static_cast<int>(_maxChargeLevel); }
		virtual void setMaxChargeLevel( int mcl );

		// returns true if this PowerPlatform is fully charged
		bool charged() const { return distance( _chargeLevel, _maxChargeLevel ) < Epsilon; } 

	protected:
	
		void _contactPowerCell( const core::collision_info &info, bool &discard );
		void _separatePowerCell( const core::collision_info &info );
		void _removePowerCellFromTouchingSet( core::GameObject * );

		void _touchingPowerCell( const core::collision_info &info );
		void _releaseCurrentPowerCell( core::GameObject * );

	private:
	
		init _initializer;
		cpBody *_body;
		cpShape *_shape;
		real _chargeLevel, _maxChargeLevel;
		
		PowerCell *_currentPowerCell;
		std::set< PowerCell* > _touchingPowerCells;
		int _injestionPhase;

};

class PowerPlatformRenderer : public core::DrawComponent
{
	public:
	
		PowerPlatformRenderer();
		virtual ~PowerPlatformRenderer();
		
		PowerPlatform *platform() const { return (PowerPlatform*) owner(); }
		
		void addedToGameObject( core::GameObject * );
		void update( const core::time_state &time );
		
	protected:

		virtual void _drawGame( const core::render_state & );
		virtual void _drawDebug( const core::render_state & );

				
	private:

		ci::gl::GlslProg _svgShader;
		core::SvgObject _svg, _lights, _halo;
		std::vector< core::SvgObject > _lightRings;
		
		real _chargeLevel;
};


}
