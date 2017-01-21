#pragma once

//
//  PlayerAnimator.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Core.h"

namespace game {

class Player;

class PlayerSvgAnimator
{
	public:
	
		PlayerSvgAnimator( Player *player, core::SvgObject playerSvg );
		virtual ~PlayerSvgAnimator();
		
		virtual void update( const core::time_state &time );

		/**
			Returns the location of the tip of the player's barrel in world units.
			This is necessary because the artwork defines where the weapon beams
			should be emitted.
		*/
		Vec2 barrelEndpointWorld();

	private:
	
		virtual void _sync( const core::time_state &time );
	
	
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

};

}
