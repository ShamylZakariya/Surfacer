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

        // current absolute time in seconds
        seconds_t time;
        
        // time elapsed
        seconds_t deltaT;
        
        // current time scale, where 1 is "real time" and a value like 0.1 would be "slow motion"
        double timeScale;
        
        // current phsyics frame step count
        size_t step;

        time_state(seconds_t t, seconds_t dt, double ts, size_t s) :
                time(t),
                deltaT(dt),
                timeScale(ts),
                step(s) {
        }

        static inline seconds_t now() {
            return cinder::app::getElapsedSeconds();
        }

    };

}

#endif /* TimeState_h */
