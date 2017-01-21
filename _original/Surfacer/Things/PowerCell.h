#pragma once

//
//  PwerCell.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 6/15/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Core.h"
#include "ParticleSystem.h"

namespace game {

class PowerCell : public core::GameObject
{
	public:
	
	
		struct init : public core::GameObject::init
		{
			int charge;
			real size;
			Vec2 position;
			
			init():
				charge(10),
				size(2),
				position(-100,-100)
			{}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				core::GameObject::init::initialize(v);
				JSON_READ(v,charge);
				JSON_READ(v,size);
				JSON_READ(v,position);
			}
		};
		
		/**
			Signal emitted when a powerup discharges into PowerPlatform
		*/
		signals::signal< void( PowerCell* ) > onDischarge;

		/**
			Signal emitted when PowerPlatform grabs hold of this PowerCell
		*/
		signals::signal< void( PowerCell* ) > onInjestedByPowerPlatform;

	public:
	
		PowerCell();
		virtual ~PowerCell();

		JSON_INITIALIZABLE_INITIALIZE();
		
		// initialization
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
		
		// GameObject
		virtual void addedToLevel( core::Level *level );
		virtual void ready();
		virtual void update( const core::time_state &time );
		
		// PowerCell
		cpBody *body() const { return _body; }
		cpShape *shape() const { return _shape; }
		
		/**
			discharge this PowerCell, returning how much power/health was removed if the PowerCell was active,
			or zero if it was walready empty.
		*/
		int discharge();

		/**
			Return the amount of charge that this PowerCell has.
		*/
		int charge() const { return std::max( static_cast<int>(_charge), 0 ); }
		
		bool empty() const { return charge() == 0; }
		
		
	protected:

		virtual void _contactFluid( const core::collision_info &info );
		
	private:
	
		friend class PowerCellRenderer;
	
		init _initializer;
		cpBody *_body;
		cpShape *_shape;
		bool _burned;
		real _charge, _opacity;

		UniversalParticleSystemController::Emitter *_fireEmitter;
};

class PowerCellRenderer : public core::DrawComponent
{
	public:
	
		PowerCellRenderer();
		virtual ~PowerCellRenderer();
		
		PowerCell *powerCell() const { return (PowerCell*) owner(); }
		
		void addedToGameObject( core::GameObject * );
		void update( const core::time_state &time );
		
	protected:

		virtual void _drawGame( const core::render_state & );
		virtual void _drawDebug( const core::render_state & );
		
		void _powerCellDischarged( PowerCell * );
		real _currentFlickerValue();
		
	private:

		ci::gl::GlslProg _svgShader;
		core::SvgObject _svg, _flickeringFace, _flickeringHalo;
		Vec2 _svgScale;
		real _dischargeFlare, _pulseOffset, _pulseFrequency;
		seconds_t _dischargeTime;
};



} // end namespace game
