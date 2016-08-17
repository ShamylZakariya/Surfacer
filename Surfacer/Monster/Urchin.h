#pragma once

//
//  Urchin.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 12/10/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "Monster.h"
#include "SvgObject.h"


namespace game {

class FrondEngine;

class Urchin : public Monster
{
	public:
	
		struct init : public Monster::init {
			real size;
			real speed;
			
			init():
				size(1),
				speed(3)
			{
				health.initialHealth = health.maxHealth = 10;
				health.resistance = 0.1;
				health.crushingInjuryMultiplier = 2;
				health.stompable = false;
				
				attackStrength = 1;
			}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				Monster::init::initialize(v);
				JSON_READ(v,size);
				JSON_READ(v,speed);
			}


		};

	public:
	
		Urchin();
		virtual ~Urchin();

		JSON_INITIALIZABLE_INITIALIZE();
		
		// Initialization
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
		
		// GameObject
		virtual void addedToLevel( core::Level *level );
		virtual void update( const core::time_state & );
		
		// Monster
		virtual void died( const HealthComponent::injury_info &info );
		virtual Vec2r position() const { return _position; }
		virtual Vec2r up() const { return _up; }
		virtual Vec2r eyePosition() const { return _position + _up * _initializer.size; }
		
		// Urchin
		real size() const { return _initializer.size; }		
		virtual cpBody *body() const { return _body; }
		const cpShapeSet &shapes() const { return _shapes; }

		// is the Urchin flipped over onto back?
		bool isFlipped() const { return _flipped; }
		
	private:

		cpBody *_body;

		cpShape *_segmentShape;
		cpShapeSet _shapes;

		init _initializer;
		Vec2r _position, _up, _centerOfMassOffset;	
		real _speed, _radius;

		bool _flipped;
		seconds_t _timeFlipped;	
		
		FrondEngine *_whiskers;

};

class UrchinRenderer : public core::DrawComponent
{
	public:
	
		UrchinRenderer();
		virtual ~UrchinRenderer(){}
		
		Urchin *urchin() const { return static_cast<Urchin*>(owner()); }
		
	protected:
	
		void _drawGame( const core::render_state &state );
		void _drawDebug( const core::render_state &state );
	
	private:

		ci::gl::GlslProg _shader;
		core::SvgObject _svg, _urchin;
		std::vector< core::SvgObject > _eyes;
		Vec2r _svgScale;
		real _velocityAccumulator, _aggressionAccumulator, _fearAccumulator;
};

class UrchinController : public GroundBasedMonsterController
{
	public:
	
		UrchinController();
		virtual ~UrchinController(){}
		
		Urchin *urchin() const { return (Urchin*) owner(); }		
		virtual void update( const core::time_state &time );

};


} // end namespace game