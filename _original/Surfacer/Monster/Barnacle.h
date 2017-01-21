#pragma once

//
//  Barnacle.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 1/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Monster.h"
#include "Tongue.h"

namespace game {

class FrondEngine;



class Barnacle : public Monster
{
	public:
	
		struct init : public Monster::init {
		
			//	Size of the barnacle's body
			real size;						
			
			//	Speed of barnacle's lateral motion
			real speed;
			
			ci::ColorA color;
			
			init():
				size(1),
				speed(3),
				color(0.1,0.1,0.1,1)
			{
				health.initialHealth = health.maxHealth = 10;
				health.resistance = 0.1;
				attackStrength = 10;
			}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				Monster::init::initialize(v);
				JSON_READ(v,size);
				JSON_READ(v,speed);
				JSON_READ(v,color);
			}
			
		};

	public:
	
		Barnacle();
		virtual ~Barnacle();

		JSON_INITIALIZABLE_INITIALIZE();
				
		// Barnacle
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
		
		// GameObject
		virtual void addedToLevel( core::Level *level );
		virtual void update( const core::time_state & );
		
		// Monster
		virtual void died( const HealthComponent::injury_info &info );
		virtual Vec2 position() const { return _position; }
		virtual Vec2 up() const { return _up; }
		virtual Vec2 eyePosition() const { return _position; }
		virtual cpBody *body() const { return _body; }
		virtual void playerContactAttack( const Vec2 &locationWorld, real injuryScale ){} // nothing, we have a more complex approach

		// Barnacle
		real size() const { return _initializer.size; }
		const cpShapeSet &shapes() const { return _shapes; }
		const cpConstraintSet &constraints() const { return _constraints; }
		const cpBodySet &bodies() const { return _bodies; }
		
		// Tongue
		Tongue *tongue() const { return _tongue; }
			
	private:
	
		friend class BarnacleRenderer;

		init _initializer;

		cpBody *_body;
		cpBodySet _bodies;
		cpShapeSet _shapes;
		cpConstraintSet _constraints;

		Vec2 _position, _up;
		real _speed, _footRadius;
		bool _deathAcidDispatchNeeded;

		FrondEngine *_whiskers;
		Tongue *_tongue;
};

class BarnacleRenderer : public core::DrawComponent
{
	public:
	
		BarnacleRenderer();
		virtual ~BarnacleRenderer();
		
		virtual void draw( const core::render_state &state );
		
		Barnacle *barnacle() const { return (Barnacle*) owner(); }

	protected:
	
		void _drawGame( const core::render_state &state );
		void _drawDebug( const core::render_state &state );
		
		void _drawBody( const core::render_state &state );

	private:

		GLuint _vbo;
		ci::gl::GlslProg _shader;
		core::SvgObject _svg, _bulb, _base;
		std::vector< core::SvgObject > _eyes;

		Vec2 _svgScale;		
};

class BarnacleController : public GroundBasedMonsterController
{
	public:
	
		BarnacleController();
		virtual ~BarnacleController(){}
		
		Barnacle *barnacle() const { return (Barnacle*) owner(); }		
		virtual void update( const core::time_state &time );
		
	protected:

		virtual void _getMotionProbeRays( cpVect &leftOrigin, cpVect &leftEnd, cpVect &rightOrigin, cpVect &rightEnd );

};


} // end namespace game
