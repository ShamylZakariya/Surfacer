//
//  GameConstants.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 6/14/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Common.h"
#include "GameConstants.h"
#include "MathHelpers.h"
#include "StringLib.h"

using namespace core;
namespace game {

	namespace InjuryType {
	
		/*
		NONE,
		CUTTING_BEAM_WEAPON,
		LAVA,
		ACID,
		CRUSH,
		STOMP,
		MONSTER_CONTACT
		*/
	
		type fromString( const std::string &typeName )
		{
			const std::string TypeNameLC = util::stringlib::uppercase( typeName );
			if ( TypeNameLC == "CUTTING_BEAM_WEAPON" ) return CUTTING_BEAM_WEAPON;
			if ( TypeNameLC == "LAVA" ) return LAVA;
			if ( TypeNameLC == "ACID" ) return ACID;
			if ( TypeNameLC == "CRUSH" ) return CRUSH;
			if ( TypeNameLC == "STOMP" ) return STOMP;
			if ( TypeNameLC == "MONSTER_CONTACT" ) return MONSTER_CONTACT;
			
			return NONE;
		}
		
		std::string toString( type t )
		{
			switch( t )
			{
				case NONE: return "NONE";
				case CUTTING_BEAM_WEAPON: return "CUTTING_BEAM_WEAPON";
				case LAVA: return "LAVA";
				case ACID: return "ACID";
				case CRUSH: return "CRUSH";
				case STOMP: return "STOMP";
				case MONSTER_CONTACT: return "MONSTER_CONTACT";
			}
			
			return "UNKNOWN";
		}
	
	}


}