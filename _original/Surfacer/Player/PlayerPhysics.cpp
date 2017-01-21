//
//  PlayerPhysics.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/19/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "PlayerPhysics.h"
#include "Player.h"
#include "GameConstants.h"

#include <chipmunk/chipmunk_unsafe.h>

using namespace ci;
namespace game {

#define CRUSH_TOPPLES_PLAYER true

namespace {

	void groundContactQuery( cpShape *shape, cpContactPointSet *points, void *data )
	{
		if ( cType != CollisionType::PLAYER && !cpShapeGetSensor(shape) )
		{
			bool *didContact = (bool*) data;
			*didContact = true;
		}
	}
	
	struct ground_slope_handler_data
	{
		real dist;
		cpVect normal, start, end;
		
		ground_slope_handler_data():
			dist( FLT_MAX ),
			normal(cpv(0,1))
		{}		
	};
	
	void groundSlopeRaycastHandler(cpShape *shape, cpFloat t, cpVect n, void *data)
	{
		if ( cpShapeGetCollisionType( shape ) != CollisionType::PLAYER )
		{
			ground_slope_handler_data *handlerData = (ground_slope_handler_data*)data;
			cpSegmentQueryInfo info;
			info.shape = shape;
			info.t = t;
			info.n = n;
			
			real dist = cpSegmentQueryHitDist( handlerData->start, handlerData->end, info );
			if ( dist < handlerData->dist )
			{
				handlerData->normal = n;
				handlerData->dist = dist;
			}
		}
	}

}


#pragma mark - PlayerPhysics


/*
		cpShapeSet _shapes;
		cpConstraintSet _constraints;
		cpBodySet _bodies;
		bool _crouching, _jumping;
		real _speed;
*/

PlayerPhysics::PlayerPhysics():
	_crouching(false),
	_jumping(false),
	_speed(0)
{}

PlayerPhysics::~PlayerPhysics()
{
	cpCleanupAndFree( _constraints );
	cpCleanupAndFree( _shapes );
	cpCleanupAndFree( _bodies );
}

cpSpace *PlayerPhysics::space() const
{
	return player()->level()->space();
}

Vec2 PlayerPhysics::_groundSlope() const
{
	const real HalfWidth = player()->initializer().width * 0.5;

//	const Vec2 
//		Position(position()),
//		Up(up()),
//		Right = rotateCW( Up ) * HalfWidth;
//		
//	const cpVect
//		Down = cpv( -Up * 5000 );

	const Vec2 
		Position(position()),
		Right( HalfWidth, 0 );
		
	const cpVect
		Down = cpv( 0, -5000 );
		
	ground_slope_handler_data handlerData;
	
	handlerData.start = cpv(Position + Right);
	handlerData.end = cpvadd( handlerData.start, Down );	
	cpSpaceSegmentQuery( space(), handlerData.start, handlerData.end, CP_ALL_LAYERS, CP_NO_GROUP, groundSlopeRaycastHandler, &handlerData );		
	cpVect normal = handlerData.normal;

	handlerData.start = cpv(Position);
	handlerData.end = cpvadd( handlerData.start, Down );	
	cpSpaceSegmentQuery( space(), handlerData.start, handlerData.end, CP_ALL_LAYERS, CP_NO_GROUP, groundSlopeRaycastHandler, &handlerData );		
	normal = cpvadd( normal, handlerData.normal );

	handlerData.start = cpv(Position-Right);
	handlerData.end = cpvadd( handlerData.start, Down );	
	cpSpaceSegmentQuery( space(), handlerData.start, handlerData.end, CP_ALL_LAYERS, CP_NO_GROUP, groundSlopeRaycastHandler, &handlerData );		
	normal = cpvadd( normal, handlerData.normal );

	return normalize( rotateCW( v2(normal) ));
}

bool PlayerPhysics::_touchingGround( cpShape *shape ) const
{
	bool touchingGroundQuery = false;
	cpSpaceShapeQuery( space(), shape, groundContactQuery, &touchingGroundQuery );

	return touchingGroundQuery;
}

#pragma mark - PogocyclePlayerPhysics

/*
		cpGroup _group;
		cpBody *_body, *_wheelBody;
		cpShape *_bodyShape, *_wheelShape, *_groundContactSensorShape;
		cpConstraint *_wheelMotor, *_jumpSpring, *_jumpGroove, *_orientationGear;
		real _wheelRadius, _wheelFriction, _crouch, _touchingGroundAcc;
		jump_spring_params _springParams;
		Vec2 _up, _groundSlope, _positionOffset;
*/

PogocyclePlayerPhysics::PogocyclePlayerPhysics():
	_group(0),
	_body(NULL),
	_wheelBody(NULL),
	_bodyShape(NULL),
	_wheelShape(NULL),
	_groundContactSensorShape(NULL),
	_wheelMotor(NULL),
	_jumpSpring(NULL),
	_jumpGroove(NULL),
	_orientationGear(NULL),
	_wheelRadius(0),
	_wheelFriction(0),
	_crouch(0),
	_touchingGroundAcc(0),
	_up(0,1),
	_groundSlope(1,0),
	_positionOffset(0,0)
{
	setName( "PogocyclePlayerPhysics" );
}

PogocyclePlayerPhysics::~PogocyclePlayerPhysics()
{}

void PogocyclePlayerPhysics::ready()
{
	Player *player = this->player();
	cpSpace *space = this->space();

	_group = reinterpret_cast<cpGroup>(player);

	const real 
		Width = player->initializer().width,
		Height = player->initializer().height, 
		Density = player->initializer().density,
		HalfWidth = Width / 2,
		HalfHeight = Height/ 2,
		Mass = Width * Height * Density,
		Moment = cpMomentForBox( Mass, Width, Height ),
		WheelRadius = HalfWidth * 0.85,
		WheelSensorRadius = WheelRadius * 1.1,
		WheelMass = Density * M_PI * WheelRadius * WheelRadius;
		
	const cpVect	
		WheelPositionOffset = cpv(0,-(HalfHeight - HalfWidth));
		
	_body = _add(cpBodyNew( Mass, Moment ));
	cpBodySetPos( _body, cpv(player->initializer().position) );
		
	// lozenge shape for body
	_bodyShape = _add(cpSegmentShapeNew( _body, cpv(0,-(HalfHeight-HalfWidth)), cpv(0,HalfHeight-HalfWidth), Width/2 ));
	cpShapeSetFriction( _bodyShape, 0.1 );
	
	// make wheel at bottom of body
	_wheelRadius = WheelRadius;
	
	_wheelBody = _add(cpBodyNew( WheelMass, cpMomentForCircle( WheelMass, 0, _wheelRadius, cpvzero )));	
	cpBodySetPos( _wheelBody, cpvadd( cpBodyGetPos( _body ), WheelPositionOffset ));
	_positionOffset = v2( cpvsub( cpBodyGetPos( _body ), cpBodyGetPos(_wheelBody)));
	
	_wheelShape = _add(cpCircleShapeNew( _wheelBody, _wheelRadius, cpvzero ));
	
	_wheelMotor = _add(cpSimpleMotorNew( _body, _wheelBody, 0 ));	
	
	_springParams.restLength = cpvlength(cpvsub(cpBodyGetPos(_body), cpBodyGetPos(_wheelBody)));
	_springParams.stiffness = 250 * Mass;
	_springParams.damping = 1000;

	_jumpSpring = _add( cpDampedSpringNew( _body, _wheelBody, cpvzero, cpvzero, _springParams.restLength, _springParams.stiffness, _springParams.damping ));	
	_jumpGroove = _add( cpGrooveJointNew( _body, _wheelBody, cpv(0,-Height * 0.125), cpv(0,-Height), cpvzero));


	//
	//	create anti-roation constraint
	//

	_orientationGear = _add(cpGearJointNew( cpSpaceGetStaticBody(space), _body, 0, 1));
	
	//
	//	create jump sensor shape
	//
	
	_groundContactSensorShape = _add(cpCircleShapeNew( _wheelBody, WheelSensorRadius, cpvzero ));
	cpShapeSetSensor( _groundContactSensorShape, true );
	
	//
	//	Finalize
	//
	
	foreach( cpShape *s, shapes() )
	{
		cpShapeSetUserData( s, player );	
		cpShapeSetLayers( s, CollisionLayerMask::PLAYER );
		cpShapeSetCollisionType( s, CollisionType::PLAYER );
		cpShapeSetGroup( s, _group );
		cpShapeSetElasticity( s, 0 );
		cpSpaceAddShape( space, s );	
	}
	
	foreach( cpBody *b, bodies() )
	{
		cpBodySetUserData( b, player );
		cpSpaceAddBody( space, b );
	}
	
	foreach( cpConstraint *c, constraints() )
	{
		cpConstraintSetUserData( c, player );
		cpSpaceAddConstraint( space, c );
	}
}

void PogocyclePlayerPhysics::update( const core::time_state &time )
{
	Player *player = this->player();
	
	const bool
		Alive = player->alive(),
		Restrained = player->restrained();

	const real
		Vel = speed() * (crouching() ? 0.25 : 1),
		Dir = sign( speed() );
		
	//
	//	Update touchingGround average (to smooth out vibrations) and ground slope
	//

	{
		_touchingGroundAcc = lrp<real>(0.2, 
			_touchingGroundAcc, 
			_touchingGround(_groundContactSensorShape) ? 1 : 0);
		
		_groundSlope = normalize( lrp( 0.2, _groundSlope, PlayerPhysics::_groundSlope() ));
	
		const real
			SlopeWeight = 0.5,
			UpWeight = 1.0,
			DirWeight = 0.5;
	
		const Vec2 NewUp = (SlopeWeight * rotateCCW(_groundSlope)) + Vec2(0, UpWeight ) + Vec2( DirWeight * Dir,0);
		_up = normalize( lrp( 0.2, _up, NewUp ));
				
		cpGearJointSetPhase( _orientationGear, std::atan2( _up.y, _up.x ) - M_PI_2 );
	}

	//
	//	Apply crouching by shrinking body line segment top to match head-height. Also, since
	//	jumping moves foot downwards, keep body segment bottom synced to foot position.
	//

	{
		_crouch = lrp<real>( 0.1, _crouch, ((Alive && crouching()) ? 1 : 0 ));
		const real 
			HalfWidth = player->initializer().width/2,
			HalfHeight = player->initializer().height/2,
			HeightExtent = HalfHeight - HalfWidth,
			UpperBodyHeightExtent = lrp<real>( _crouch, HeightExtent, -HeightExtent*0 ),
			LowerBodyHeightExtent = (cpBodyGetPos( _wheelBody ).y - cpBodyGetPos( _body ).y) + HalfWidth;

		cpSegmentShapeSetEndpoints( _bodyShape, cpv(0,UpperBodyHeightExtent), cpv(0,LowerBodyHeightExtent));
	}
	
	
	//
	// rotate the wheel motor
	//

	if ( Alive && !Restrained )
	{
		const real 
			WheelCircumference = 2 * _wheelRadius * M_PI,
			RequiredTurns = Vel / WheelCircumference,
			DesiredMotorRate = 2 * M_PI * RequiredTurns,
			CurrentMotorRate = cpSimpleMotorGetRate( _wheelMotor ),
			TransitionRate = 0.1,
			MotorRate = lrp<real>( TransitionRate, CurrentMotorRate, DesiredMotorRate );
		
		cpSimpleMotorSetRate( _wheelMotor, MotorRate );
	}
	else
	{
		cpSimpleMotorSetRate( _wheelMotor, 0 );
	}
	
		
	//
	//	Kick out unicycle wheel to jump
	//
	
	bool kicking = false;
	
	if ( Alive && !Restrained && jumping() && touchingGround() )
	{	
		const real 
			IncreaseRate = 0.2,
			RestLength = lrp<real>(IncreaseRate, cpDampedSpringGetRestLength(_jumpSpring), _springParams.restLength * 3 ),
			Damping = lrp<real>(IncreaseRate, cpDampedSpringGetDamping( _jumpSpring ), _springParams.damping * 0.01 ),
			Stiffness = lrp<real>(IncreaseRate, cpDampedSpringGetStiffness( _jumpSpring ), _springParams.stiffness * 2 );
	
		cpDampedSpringSetRestLength( _jumpSpring, RestLength );
		cpDampedSpringSetDamping( _jumpSpring, Damping );
		cpDampedSpringSetStiffness( _jumpSpring, Stiffness );
		kicking = true;
	}
	else
	{
		//
		//	So long as the jump key is still pressed, use a lighter regression rate. This allows taller jumps
		//	when the jump key is held longer.
		//

		const real 
			ReturnRate = 0.1,
			RestLength = lrp<real>(ReturnRate, cpDampedSpringGetRestLength(_jumpSpring), _springParams.restLength ),
			Damping = lrp<real>(ReturnRate, cpDampedSpringGetDamping( _jumpSpring ), _springParams.damping ),
			Stiffness = lrp<real>(ReturnRate, cpDampedSpringGetStiffness( _jumpSpring ), _springParams.stiffness );
			
		cpDampedSpringSetRestLength( _jumpSpring, RestLength );
		cpDampedSpringSetDamping( _jumpSpring, Damping );
		cpDampedSpringSetStiffness( _jumpSpring, Stiffness );
	}
	
	//
	//	In video games characters can 'steer' in mid-air. It's expected, even if unrealistic.
	//	So we do it here, too.
	//
	
	if ( !Restrained && !touchingGround() )
	{
		const real 
			ActualVel = cpBodyGetVel( _body ).x;
			
		real
			baseImpulseToApply = 0,
			baseImpulse = 0.15;
			 
		//
		//	We want to apply force so long as applying force won't take us faster
		//	than our maximum walking speed
		//
			 
		if ( int(sign(ActualVel)) == int(sign(Vel)))
		{
			if ( std::abs( ActualVel ) < std::abs( Vel ))
			{
				baseImpulseToApply = baseImpulse;
			}
		}
		else
		{
			baseImpulseToApply = baseImpulse;
		}
		
		if ( std::abs( baseImpulseToApply > Epsilon ))
		{
			cpBodyApplyImpulse( _body, cpv( Dir * baseImpulseToApply * cpBodyGetMass(_body),0), cpvzero );
		}	
	}
	
		
	//
	//	Update wheel friction - when crouching, increase friction. When not walking & crouching, increase further.
	//	Note, when kicking off ground we reduce friction. This is to prevent the super-hill-jump effect where running
	//	up a steep slope and jumping causes superman-like flight.	
	//
	
	if ( !Restrained )
	{
		const real 
			KickingFriction = 0.25,
			DefaultWheelFriction = 3,
			CrouchingWheelFriction = 4,
			LockdownWheelFrictionMultiplier = 4,
			
			BasicFriction = lrp<real>( _crouch, DefaultWheelFriction, CrouchingWheelFriction ),
			Stillness = (1-std::abs( sign( Vel ))), 
			Lockdown = _crouch * Stillness,
			LockdownFriction = lrp<real>( Lockdown, BasicFriction, BasicFriction * LockdownWheelFrictionMultiplier );

		_wheelFriction = lrp<real>( 0.35, _wheelFriction, kicking ? KickingFriction : LockdownFriction );
				
		cpShapeSetFriction( _wheelShape, _wheelFriction );
	}
	else
	{
		cpShapeSetFriction( _wheelShape, 0 );
	}
	
	//
	//	If player has died, release rotation constraint
	//
	
	if ( !Alive )
	{
		cpConstraintSetMaxForce( _orientationGear, 0 );
		cpConstraintSetMaxBias( _orientationGear, 0 );
		cpConstraintSetErrorBias( _orientationGear, 0 );
	}	
	else
	{
		#if CRUSH_TOPPLES_PLAYER

			// orientation constraint may have been lessened when player was crushed, we always want to ramp it back up
			const real MaxForce = cpConstraintGetMaxForce(_orientationGear);
			if ( MaxForce < 10000 )
			{
				cpConstraintSetMaxForce( _orientationGear, std::max<real>(MaxForce,10) * 1.1 );
			}
			else
			{
				cpConstraintSetMaxForce( _orientationGear, INFINITY );
			}
		#endif
	}
}

void PogocyclePlayerPhysics::injured( const HealthComponent::injury_info &info )
{
	#if CRUSH_TOPPLES_PLAYER

	switch( info.type )
	{
		case InjuryType::CRUSH:
		{
			const real Scale = 1;
			cpConstraintSetMaxForce( _orientationGear, 10000 * (1 - (Scale * info.grievousness)) );
			break;
		}
		
		default: break;
	}
	#endif
}

void PogocyclePlayerPhysics::died( const HealthComponent::injury_info &info )
{}

Vec2 PogocyclePlayerPhysics::position() const
{
	return v2(cpBodyGetPos( _wheelBody )) + _positionOffset;
}

Vec2 PogocyclePlayerPhysics::up() const
{
	return _up;
}

Vec2 PogocyclePlayerPhysics::groundSlope() const
{
	return _groundSlope;
}

bool PogocyclePlayerPhysics::touchingGround() const
{
	return _touchingGroundAcc >= 0.5;
}

cpBody *PogocyclePlayerPhysics::body() const
{
	return _body;
}

cpBody *PogocyclePlayerPhysics::footBody() const
{
	return _wheelBody;
}



}
