#pragma once

#include "Common.h"

//
//  Stopwatch.h
//  Surfacer
//
//  Created by Zakariya Shamyl on 8/24/11.
//  Copyright 2011 TomorrowPlusX. All rights reserved.
//

class StopwatchImpl;
class Stopwatch
{
	public:
	
		Stopwatch();
		Stopwatch( const std::string &eventName );

		~Stopwatch();
		
		/**
			(re)-start stopwatch 
		*/
		void start();
		
		/**
			return how many seconds have elapsed since last call to start() or mark().
			restarts stopwatch to measure from time this is called.
		*/
		seconds_t mark();
		
		/**
			same as mark(), but returns total time elapsed since last call to start.
		*/
		seconds_t total();

	private:
    
        StopwatchImpl *_impl;
		std::string _eventName;
    
};

