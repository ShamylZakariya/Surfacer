#pragma once

//
//  TimeState.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/11/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "Common.h"

namespace core {

/**
	@struct time_state
	time_state represents the current game time. 
	It is passed to GameObject::update.
*/
struct time_state {

	seconds_t		time;
	seconds_t		deltaT;
	std::size_t		step;
	
	time_state( seconds_t t, seconds_t dt, std::size_t s ):
		time( t ), 
		deltaT( dt ),
		step(s)
	{}
	
};



}