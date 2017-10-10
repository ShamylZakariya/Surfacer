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
#include <cinder/app/App.h>

namespace core {

	typedef double seconds_t;
	using std::size_t;

	/**
	 @struct time_state
	 time_state represents the current game time.
	 It is passed to Object::update.
	 */
	struct time_state {

		seconds_t		time;
		seconds_t		deltaT;
		size_t		step;

		time_state( seconds_t t, seconds_t dt, size_t s ):
		time( t ),
		deltaT( dt ),
		step(s)
		{}

		static inline seconds_t now() {
			return cinder::app::getElapsedSeconds();
		}

	};

}

#endif /* TimeState_h */
