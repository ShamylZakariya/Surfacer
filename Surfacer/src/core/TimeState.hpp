//
//  TimeState.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/13/17.
//
//

#ifndef TimeState_h
#define TimeState_h

#include <cstddef>

namespace core {

	typedef double seconds_t;

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

#endif /* TimeState_h */
