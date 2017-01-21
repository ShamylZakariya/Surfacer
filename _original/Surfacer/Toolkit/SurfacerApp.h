#pragma once

//
//  SurfacerApp.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 2/6/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <cinder/app/App.h>
#include "UIStack.h"

namespace game {

class PhysicsLoop;
class GameScenario;

class SurfacerApp : public ci::app::App
{
	public:
	
		static SurfacerApp *instance();
		
	public:
	
		SurfacerApp();		
		virtual ~SurfacerApp();

		// AppBasic
		virtual void setup();
		virtual void shutdown();
		virtual void prepareSettings( Settings *settings );
		virtual void update();
		virtual void draw();
		virtual void resize( ci::app::ResizeEvent evt );
				
		// SurfacerApp
		virtual void step();
		
		GameScenario *gameScenario() const { return _scenario; }
		void setGameScenario( GameScenario * );
		
		real getAverageSps() const { return _averageStepsPerSecond; }
				
	private:
	
		PhysicsLoop				*_physicsLoop;
		GameScenario			*_scenario;

		unsigned int			_stepCount;
		seconds_t				_stepCountTime;
		real					_averageStepsPerSecond;

};

/**
	@class PhysicsLoop
	@brief PhysicsLoop is responsible for dispatching calls to step() on SurfacerGame,
	doing its best over time to maintain a target update rate
*/
class PhysicsLoop
{
	public:
	
		PhysicsLoop( SurfacerApp *app, int rate );
		virtual ~PhysicsLoop( void );
				
		inline SurfacerApp *app( void ) const { return _app; }
		int rate() const { return _rate; }
		seconds_t interval() const { return _interval; }
		
		/**
			@brief Called by main event loop
			Subclasses will invoke dispatchDisplay() and dispatchStep()
		*/
		virtual void step( void ) = 0;
				
	protected:
	
		inline void dispatchStep() { _app->step(); }
				
	private:
	
		SurfacerApp				*_app;
		int						_rate;
		seconds_t				_interval;
		

};

class AdaptivePhysicsLoop : public PhysicsLoop
{
	private:
	
		seconds_t _currentTime, _lastTime, _timeRemainder;

	public:
	
		AdaptivePhysicsLoop( SurfacerApp *app, int rate );
		virtual ~AdaptivePhysicsLoop( void );
		
		virtual void step( void );
};

class SacredSoftwarePhysicsLoop : public PhysicsLoop
{
	private:
	
		seconds_t _currentTime, _lastTime, _timeRemainder, _cyclesLeftOver;

	public:
	
		SacredSoftwarePhysicsLoop( SurfacerApp *app, int rate );
		virtual ~SacredSoftwarePhysicsLoop( void );
		
		virtual void step( void );
};

} // end namespace game
