#pragma once

//
//  GameNotifications.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/2/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

namespace game {

namespace Notifications {

	enum notifications {

		PLAYER_INJURED = 1000,
		PLAYER_HEALTH_BOOST,
		PLAYER_POWER_BOOST,

		BARNACLE_GRABBED_PLAYER,
		BARNACLE_RELEASED_PLAYER,
		
		SENSOR_TRIGGERED	
	};
}

}