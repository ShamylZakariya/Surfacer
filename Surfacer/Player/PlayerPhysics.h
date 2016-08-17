#pragma once

//
//  PlayerPhysics.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/19/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Core.h"
#include "Components.h"
#include "GameComponents.h"

namespace game {

class Player;

class PlayerPhysics : public core::Component
{
	public:
	
		PlayerPhysics();
		virtual ~PlayerPhysics();
		
		// Component		
		virtual void ready(){}
		virtual void update( const core::time_state &time ){}
		virtual void draw( const core::render_state &state ){}
		
		// Entity forwarding
		virtual void injured( const HealthComponent::injury_info &info ){}
		virtual void died( const HealthComponent::injury_info &info ){}	
		
		// PlayerPhysics
		Player *player() const { return (Player*)owner(); }
		cpSpace *space() const;
				
		virtual Vec2r position() const = 0;
		virtual Vec2r up() const = 0;
		virtual Vec2r groundSlope() const = 0;
		virtual bool touchingGround() const = 0;
		virtual cpBody *body() const = 0;
		virtual cpBody *footBody() const = 0;
		virtual cpGroup group() const = 0;
				
		const cpShapeSet &shapes() const { return _shapes; }	
		const cpConstraintSet &constraints() const { return _constraints; }
		const cpBodySet &bodies() const { return _bodies; }
		

		// Control inputs, called by Player in Player::update
		
		virtual void setSpeed( real vel ) { _speed = vel; }
		real speed() const { return _speed; }
		
		virtual void setCrouching( bool crouching ) { _crouching = crouching; }
		bool crouching() { return _crouching; }
		
		virtual void setJumping( bool j ) { _jumping = j; }
		bool jumping() const { return _jumping; }

	protected:
		
		cpShape *_add( cpShape *shape ) { _shapes.insert(shape); return shape; }
		cpConstraint *_add( cpConstraint *constraint ) { _constraints.insert(constraint); return constraint; }
		cpBody *_add( cpBody *body ) { _bodies.insert(body); return body; }
		
		Vec2r _groundSlope() const;
		bool _touchingGround( cpShape *shape ) const;
		
	private:
	
		cpShapeSet _shapes;
		cpConstraintSet _constraints;
		cpBodySet _bodies;
		bool _crouching, _jumping;
		real _speed;

};

class PogocyclePlayerPhysics : public PlayerPhysics
{
	public:
	
		PogocyclePlayerPhysics();
		virtual ~PogocyclePlayerPhysics();
		
		// Component
		virtual void ready();
		virtual void update( const core::time_state &time );

		// Entity forwarding
		virtual void injured( const HealthComponent::injury_info &info );
		virtual void died( const HealthComponent::injury_info &info );
		
		// PlayerPhysics
		virtual Vec2r position() const;
		virtual Vec2r up() const;
		virtual Vec2r groundSlope() const;
		virtual bool touchingGround() const;
		virtual cpBody *body() const;
		virtual cpBody *footBody() const;
		virtual cpGroup group() const { return _group; }
		
	private:
	
		struct jump_spring_params
		{
			real restLength, stiffness, damping;
		};
	
		cpGroup _group;
		cpBody *_body, *_wheelBody;
		cpShape *_bodyShape, *_wheelShape, *_groundContactSensorShape;
		cpConstraint *_wheelMotor, *_jumpSpring, *_jumpGroove, *_orientationGear;
		real _wheelRadius, _wheelFriction, _crouch, _touchingGroundAcc;
		jump_spring_params _springParams;
		Vec2r _up, _groundSlope, _positionOffset;
};


}