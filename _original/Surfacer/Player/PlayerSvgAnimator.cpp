//
//  PlayerAnimator.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "PlayerSvgAnimator.h"
#include "Player.h"

using namespace ci;
using namespace core;
namespace game {

/*
		struct player_parts {
			core::SvgObject
				body,
				hindLeg,
				hindShin,
				helmet,
				torso,
				foreLeg,
				foreShin,
				gun,
				barrelEndpoint,
				arm;				
		};

	private:

		Player				*_player;
	
		core::SvgObject		_svg;
		player_parts		_parts;
		Vec2				_groundSlope, _weaponAim;
		real				_facing, _dir, _motion, _crouch, _inAir, _jump, _angle;
		seconds_t			_jumpTime;
*/

PlayerSvgAnimator::PlayerSvgAnimator( Player *player, SvgObject playerSvg ):
	_player(player),
	_svg(playerSvg),
	_groundSlope(0,0),
	_weaponAim(0,0),
	_facing(0),
	_dir(0),
	_motion(0),
	_crouch(0),
	_inAir(0),
	_jump(0),
	_angle(0),
	_jumpTime(0)
{
	_parts.body = _svg.find( "Body" );
	_parts.hindLeg = _svg.find( "Body/Hindleg" );
	_parts.hindShin = _svg.find( "Body/Hindleg/Hindshin" );
	_parts.helmet = _svg.find( "Body/Torso/Helmet" );
	_parts.foreLeg = _svg.find( "Body/Foreleg" );
	_parts.foreShin = _svg.find( "Body/Foreleg/Foreshin" );
	_parts.torso = _svg.find( "Body/Torso" );
	_parts.gun = _svg.find( "Body/Gun" );
	_parts.barrelEndpoint = _svg.find( "Body/Gun/BarrelEndpoint" );
	_parts.arm = _svg.find( "Body/Arm" );
	
	assert( _parts.body );
	assert( _parts.hindLeg );
	assert( _parts.hindShin );
	assert( _parts.helmet );
	assert( _parts.torso );
	assert( _parts.foreLeg );
	assert( _parts.foreShin );
	assert( _parts.gun );
	assert( _parts.barrelEndpoint );
	assert( _parts.arm );	
}

PlayerSvgAnimator::~PlayerSvgAnimator()
{}

void PlayerSvgAnimator::update( const time_state &time )
{
	_sync( time );

	//
	//	Update _facing to match the Player's. Flip character image accordingly
	//

	const bool 
		FacingRight = _facing >= 0,
		Alive = _player->alive();

	const real 
		FacingSign = sign(_facing);
	
	//real XScale = std::max( std::abs(_facing), real(0.75) ) * FacingSign;
	const real XScale = FacingSign * (0.5 + (0.5 * std::abs( _facing )));
	_parts.body.setScale( XScale, 1 );	
	_parts.body.setAngle( FacingSign * _angle );

	//
	//	Make gun point in direction of weapon - note when facing left we need to swozzle the angle
	//

	const real 
		AimSlope = std::atan2( _weaponAim.y, std::abs(_weaponAim.x) ),
		GunAngle = AimSlope,
		HelmetAngle = (AimSlope*0.4);

	_parts.gun.setAngle( GunAngle );
	_parts.helmet.setAngle( HelmetAngle );
	
	//
	//	Angle torso and move head to lean into forward motion and back
	//
	
	real 
		// scaled from 0.25 when crouching to 1 when not crouching - used to scale leg motion extent
		crouchExtentScale = 0.25 + 0.75 * ( 1 - _crouch ),

		// angle offset for legs to match ground slope
		slopeOffsetAngle = std::atan2( _groundSlope.y, _groundSlope.x ) * (FacingRight ? 1 : -1),

		leanAngle = _dir * M_PI * 0.05 * -FacingSign * crouchExtentScale;

	_parts.torso.setAngle( leanAngle + (0.25 * slopeOffsetAngle * (1-_inAir)));
	
	
		
	//
	//	First, compute tw sin waves, one for walking one for crouching, we'll
	//	then blend them based on whether walking or crouching.
	//
		
	real 
		walkPeriod = 4 * M_PI,
		crouchPeriod = 6 * M_PI,
		walkPhase = std::sin( time.time * walkPeriod ),				// [-1,1]
		crouchPhase = std::sin( time.time * crouchPeriod );				// [-1,1]

	walkPhase = lrp<real>(_crouch, walkPhase, crouchPhase );

	real
		// walkCycle is a sin wave scaled to flat when not moving
		walkCycle = lrp<real>( _motion, 0, walkPhase ),
				
		// angle of thighs
		foreLegAngle = slopeOffsetAngle + (walkCycle * M_PI * 0.125 * crouchExtentScale),
		hindLegAngle = slopeOffsetAngle + (-walkCycle * M_PI * 0.125 * crouchExtentScale),

		// max shin deflection
		foreShinExtent = -M_PI_4 * crouchExtentScale,
		hindShinExtent = -M_PI_4 * crouchExtentScale,
		
		// scaling of shin deflection based on walkCycle
		kneeCycle = 0.5 + 0.5 * walkCycle,
		
		// angle of shins notice that we're not bending past full extension, because they're *knees*
		foreShinAngle = lrp<real>( kneeCycle, foreShinExtent, 0 ),
		hindShinAngle = lrp<real>( 1-kneeCycle, hindShinExtent, 0 );
				
	//
	//	When player is in air, remove walk cycle and bend legs into flying pose
	//
	
	foreLegAngle = lrp<real>( _inAir, foreLegAngle, -M_PI * 0.125 );
	hindLegAngle = lrp<real>( _inAir, hindLegAngle, -M_PI * 0.125 );
	foreShinAngle = lrp<real>( _inAir, foreShinAngle, -M_PI * 0.25 );
	hindShinAngle = lrp<real>( _inAir, hindShinAngle, -M_PI * 0.25 );


	//
	//	Now position legs
	//
	
	_parts.foreLeg.setAngle( foreLegAngle );
	_parts.foreShin.setAngle( foreShinAngle );
	
	_parts.hindLeg.setAngle( hindLegAngle );
	_parts.hindShin.setAngle( hindShinAngle );
	
	
	//
	//	Apply bob-cycles to body parts to make things jostly
	//

	const real 
		Scale = _svg.scale().x,
		PlayerHeight = _player->initializer().height,
		BobPeriod = walkPeriod * 0.8,
		BobPhase0 = std::cos( (time.time + 0.5 ) * BobPeriod * 1.4 ),
		BobPhase1 = std::sin( (time.time + 1.0 ) * BobPeriod * 1.3 ),
		BobPhase2 = std::cos( (time.time + 1.5 ) * BobPeriod * 1.2 ),
		BobPhase3 = std::sin( (time.time + 2.0 ) * BobPeriod * 1.1 ),
		BobScale = Alive ? ((1 - _inAir) * (0.5 + 0.5 * ( 1 - _crouch ))) : 0;

	const Vec2 
		CrouchOffset	= _crouch * -Vec2( 0, PlayerHeight * 0.125 ) / Scale,
		TorsoBobOffset	= BobScale * _motion * Vec2( 0, PlayerHeight * 0.0125 * BobPhase0 ) / Scale,
		HelmetBobOffset = BobScale * _motion * Vec2( 0, PlayerHeight * 0.0250 * BobPhase1 ) / Scale,
		GunBobOffset	= BobScale * _motion * Vec2( 0, PlayerHeight * 0.0125 * BobPhase2 ) / Scale,
		ArmBobOffset	= BobScale * _motion * Vec2( 0, PlayerHeight * 0.0250 * BobPhase3 ) / Scale;

	_parts.torso.setPosition( CrouchOffset + TorsoBobOffset );
	_parts.helmet.setPosition( CrouchOffset * 0.5 + HelmetBobOffset );
	_parts.gun.setPosition( CrouchOffset + GunBobOffset );
	_parts.arm.setPosition( CrouchOffset + ArmBobOffset );

	//
	//	Finally, position Svg
	//

	_svg.setPosition( _player->position() );	
}

Vec2 PlayerSvgAnimator::barrelEndpointWorld()
{
	return _parts.barrelEndpoint.localToGlobal( Vec2(0,0));
}

void PlayerSvgAnimator::_sync( const time_state &time )
{
	_groundSlope = _player->groundSlope();
	_weaponAim = _player->weapon()->direction();	
	
	const bool 
		FacingRight = _player->weapon()->direction().x > 0,
		Motion = std::abs( _player->motion() ) > Epsilon,
		Crouching = _player->crouching(),
		InAir = !_player->touchingGround(),
		Jumped = _player->jumping();
		
	_facing = lrp<real>( 0.10, _facing, FacingRight ? 1 : -1 );
	_dir	= lrp<real>( 0.10, _dir, _player->motion() );
	_motion = lrp<real>( 0.20, _motion, Motion ? 1 : 0 );
	_crouch = lrp<real>( 0.20, _crouch, Crouching ? 1 : 0 );
	_inAir  = lrp<real>( 0.05, _inAir,  InAir ? 1 : 0 );	
	
//	_angle = lrp<real>(0.2, _angle, _player->alive() ? 0 : cpBodyGetAngle( _player->body() ));
	_angle = lrp<real>(0.2, _angle, cpBodyGetAngle( _player->body() ));	
	
	if ( Jumped )
	{
		// a jump just happened
		_jumpTime = time.time;
	}
		
	if ( _jumpTime > 0 )
	{
		const seconds_t 
			Elapsed = time.time - _jumpTime,
			JumpAnimationTime = 0.25;
			
		// jump cycle goes from 0->1->0 and will be used to 'kick' legs
		if ( Elapsed <= JumpAnimationTime )
		{
			_jump = std::sqrt(std::sin( (Elapsed / JumpAnimationTime) * M_PI));
		}
		else
		{
			_jump = 0;
			_jumpTime = 0;
		}
	}
	else
	{
		_jump = 0;
	}
	
	
}






}
