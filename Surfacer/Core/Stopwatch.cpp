//
//  Stopwatch.cpp
//  Surfacer
//
//  Created by Zakariya Shamyl on 8/24/11.
//  Copyright 2011 TomorrowPlusX. All rights reserved.
//

#include "Stopwatch.h"
#include <cinder/app/App.h>


// clang doesn't like boost::xtime!
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wparentheses"
#pragma clang diagnostic ignored "-Wconversion"
#include <boost/thread/xtime.hpp>
#pragma clang diagnostic pop

#pragma mark StopWatchImpl

class StopwatchImpl
{
	public:

		StopwatchImpl():
			_total(0)
		{
			start();
		}

		inline void start()
		{
			boost::xtime_get( &_start, boost::TIME_UTC );
			_total = 0;
		}	

		inline seconds_t mark()
		{
			boost::xtime_get( &_stop, boost::TIME_UTC );

			const seconds_t RecipNano = 1.0 / 1e9;
			seconds_t elapsed = (_stop.sec - _start.sec) + (( _stop.nsec - _start.nsec ) * RecipNano );
			_total += elapsed;
			
			boost::xtime_get( &_start, boost::TIME_UTC );

			return elapsed;
		}

		inline seconds_t total()
		{
			mark();
			return _total;
		}

	private:

		boost::xtime _start, _stop;
		seconds_t _total;

};


#pragma mark -
#pragma mark StopWatch

/*
	StopwatchImpl *_impl;
*/

Stopwatch::Stopwatch():
	_impl( new StopwatchImpl() )
{}

Stopwatch::Stopwatch( const std::string &eventName ):
	_impl( new StopwatchImpl() ),
	_eventName(eventName)
{}

Stopwatch::~Stopwatch()
{
	if ( !_eventName.empty())
	{
		seconds_t m = _impl->mark();
		ci::app::console() << "Stopwatch[" << _eventName << "] TIME: " << m << std::endl;
	}

	delete _impl;
}
		
void Stopwatch::start()
{
	_impl->start();
}

seconds_t Stopwatch::mark()
{
	return _impl->mark();
}

seconds_t Stopwatch::total()
{
	return _impl->total();
}
