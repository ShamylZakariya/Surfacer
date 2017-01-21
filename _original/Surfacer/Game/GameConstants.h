#pragma once

//
//  GameConstants.h
//  Surfacer
//
//  Created by Zakariya Shamyl on 9/24/11.
//  Copyright 2011 TomorrowPlusX. All rights reserved.
//


namespace game {

namespace RenderLayer {

	enum layer_id { 
	
		BACKGROUND		= 0,

		SENSORS			= 1000,
		MONSTER			= 2000,
		CHARACTER		= 3000,
		ENVIRONMENT		= 4000,
		TERRAIN			= 5000,
		EFFECTS			= 6000,

		FOREGROUND		= 10000
		
	};


}

namespace CollisionLayerMask { 

	namespace Layers {
		enum layer_id {
		
			TERRAIN_BIT			= 1 << 2,
			PLAYER_BIT			= 1 << 3,
			MONSTER_BIT			= 1 << 4,
			DECORATION_BIT		= 1 << 5,
			FLUID_BIT			= 1 << 6,
			TENTACLE_BIT		= 1 << 7
		};
	}


	enum masks {

		ALL = CP_ALL_LAYERS,
		
		TERRAIN	= 
			Layers::TERRAIN_BIT | 
			Layers::MONSTER_BIT | 
			Layers::PLAYER_BIT | 
			Layers::TENTACLE_BIT,
		
		PLAYER = 
			Layers::PLAYER_BIT | 
			Layers::MONSTER_BIT | 
			Layers::TENTACLE_BIT,
		
		MONSTER = 
			Layers::MONSTER_BIT | 
			Layers::TERRAIN_BIT | 
			Layers::PLAYER_BIT,
			
		DEAD_MONSTER =
			Layers::TERRAIN_BIT,
		
		DECORATION = 
			Layers::DECORATION_BIT | 
			Layers::TERRAIN_BIT,
		
		FLUID = 
			Layers::FLUID_BIT | 
			Layers::TERRAIN_BIT | 
			Layers::PLAYER_BIT | 
			Layers::MONSTER_BIT,
		
		TENTACLE = 
			Layers::TENTACLE_BIT,
			
		POWERUP = 
			Layers::TERRAIN_BIT |
			Layers::PLAYER_BIT,
			
		TRIGGERED_POWERUP =
			0,
			
		POWER_PLATFORM =
			Layers::TERRAIN_BIT |
			Layers::PLAYER_BIT,			
			
		MIRROR = 
			Layers::TERRAIN_BIT |
			Layers::PLAYER_BIT
		
	};

}

namespace CollisionGroup {

	enum group_id {
	
		NONE = CP_NO_GROUP
		
	};

}


namespace CollisionType {

	/*
		The high 16 bits are a mask, the low are a type_id, the actual type is the | of the two.
	*/

	namespace is {
		enum bits {
			SHOOTABLE   = 1 << 16,
			TOWABLE		= 1 << 17
		};
	}

	enum type_id {

		TERRAIN				= 1 | is::SHOOTABLE | is::TOWABLE,	// 196609
		MONSTER				= 2 | is::SHOOTABLE,				// 65538
		PLAYER				= 3 | is::SHOOTABLE,				// 65539
		WEAPON				= 4,								// 4
		FLUID				= 5,								// 5
		DECORATION			= 6 | is::TOWABLE | is::SHOOTABLE,	// 196614
		STATIC_DECORATION	= 7 | is::SHOOTABLE,				// 65543
		MIRROR_CASING		= 8 | is::TOWABLE,					// 131080
		MIRROR				= 9 | is::SHOOTABLE,				// 65545
		SENSOR				= 10								// 10

	};	
}

namespace GameObjectType {

	enum type_id {
	
		BACKGROUND				= 0,
		PARTICLE_SYSTEM			= 1,

		ISLAND					= 10,
		ISLAND_DYNAMIC_GROUP	= 11,
		ISLAND_STATIC_GROUP		= 12,
		TERRAIN					= 13,

		BARNACLE				= 20,
		URCHIN					= 21,
		SHOGGOTH				= 22,
		SHUB_NIGGURATH			= 23,
		SHUB_NIGGURATH_TENTACLE	= 24,
		YOG_SOTHOTH				= 25,
		GRUB					= 26,
		CENTIPEDE				= 27,
		TONGUE					= 28,
				
		PLAYER					= 40,
		PLAYER_WEAPON			= 41,
		
		FLUID					= 50,	
		DECORATION				= 51,
		POWERUP					= 52,
		POWERCELL				= 53,
		
		SENSOR					= 100
	
	};
	
	inline bool isMonster( int tid )
	{
		return tid >= 20 && tid < 40;
	}
	
	inline bool isEntity( int tid )
	{
		return isMonster(tid) || tid == PLAYER;
	}
	
}

namespace TerrainCutType {
	enum cut_type {
	
		/*
			in-game cutting type with fancy effects, sounds, etc
		*/
		GAMEPLAY,
		
		/*
			development/debugging cutting type
		*/
		MANUAL,
		
		/*
			cut caused by a gameplay avalanche event
		*/
		AVALANCHE,

		/*
			cut caused by a gameplay fracture event
		*/
		FRACTURE
		
	};
}

namespace InjuryType {
	enum type {
	
		NONE,
	
		/**
			Injury caused by player's cutting beam
		*/
		CUTTING_BEAM_WEAPON,
		
		/**
			Injury caused by contact with lava
		*/
		LAVA,
		
		/**
			Injury caused by contact with acid
		*/
		ACID,
		
		/**
			Injury caused by being crushed
		*/
		CRUSH,
		
		/**
			Killing blow caused by player "stomping" a monster
			Only Entities who's heath component has 'stompable' set to true can be stomped.
		*/
		STOMP,
		
		/**
			Injury to player caused by touching a Monster
		*/
		MONSTER_CONTACT
	
	};
	
	type fromString( const std::string & );
	std::string toString( type );
}

const real AcidAttackStrength = 1;


} // end namespace game