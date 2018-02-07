//
//  StopWatch.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 3/14/17.
//
//

#ifndef StopWatch_h
#define StopWatch_h

#include <chrono>
#include <cinder/Log.h>

namespace core {

    class StopWatch {
    public:

        typedef double seconds_t;

        StopWatch() :
                _total(0) {
            start();
        }

        StopWatch(const std::string &eventName) :
                _total(0),
                _eventName(eventName) {
            start();
        }

        ~StopWatch() {
            if (!_eventName.empty()) {
                auto t = mark();
                ci::app::console() << "StopWatch[" << _eventName << "] TIME: " << t << " seconds" << std::endl;
            }
        }

        /**
         (re)-start stopwatch
            */
        void start() {
            _start = chrono::steady_clock::now();
            _total = 0;
        }

        /**
         return how many seconds have elapsed since last call to start() or mark().
         restarts stopwatch to measure from time this is called.
            */
        seconds_t mark() {
            auto now = chrono::steady_clock::now();
            auto diff = now - _start;
            _total += chrono::duration<seconds_t, ratio<1, 1>>(diff).count();
            _start = now;
            return _total;
        }

        seconds_t total() const {
            return _total;
        }

    private:

        std::string _eventName;
        chrono::time_point<chrono::steady_clock> _start;
        seconds_t _total;
    };

} // end namespace core

#endif /* StopWatch_h */
