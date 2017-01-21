//
//  SurfacerApp.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 2/6/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "SurfacerApp.h"
#include "GameScenario.h"

using namespace ci;
namespace game {

namespace {

	SurfacerApp *__app = NULL;

}

/*
		PhysicsLoop				*_physicsLoop;
		GameScenario			*_scenario;
		
		unsigned int			_stepCount;
		seconds_t				_stepCountTime;
		real					_averageStepsPerSecond;
*/

SurfacerApp *SurfacerApp::instance()
{
	return __app;
}

/*
	So, the PhysicsLoop experiment didn't pan out. It works, but severely drops the framerate when
	the player is cutting terrain. For some reason this doesn't happen when using the cinder
	update loop, so -- for now -- we're not using PhysicsLoop to dispatch physics updates.
*/

#define USE_PHYSICS_LOOP 0

SurfacerApp::SurfacerApp():
	#if USE_PHYSICS_LOOP
	_physicsLoop( new AdaptivePhysicsLoop(this,60)),
	#else
	_physicsLoop(NULL),
	#endif
	_scenario(NULL),
	_stepCount(0),
	_stepCountTime(0),
	_averageStepsPerSecond(0)
{
	assert( !__app );
	__app = this;
}

SurfacerApp::~SurfacerApp()
{}

void SurfacerApp::setup()
{
	_scenario->dispatchSetup();
	_scenario->dispatchResize( getWindowSize() );
}

void SurfacerApp::shutdown()
{
	app::console() << "SurfacerApp::shutdown - shutting down active scenario and deleting it..." << std::endl;
	_scenario->dispatchShutdown();
	delete _scenario;
	_scenario = NULL;
}

void SurfacerApp::prepareSettings( Settings *settings )
{
	settings->setTitle( "Surfacer" );
	settings->setWindowSize( 800, 600 );
	
	// 
	//	we're going to attempt to draw as fast as posible. Rendering blocks on VBL,
	//	and we dispatch steps as needed
	//

	settings->setFrameRate( 1000 );
}

void SurfacerApp::update()
{
	#if USE_PHYSICS_LOOP
		_physicsLoop->step();
	#else
		step();
	#endif
	
	_scenario->dispatchUpdate();
}

void SurfacerApp::draw()
{
	_scenario->dispatchDraw();
}

void SurfacerApp::resize( ci::app::ResizeEvent evt )
{
	_scenario->dispatchResize( evt.getSize() );
}

void SurfacerApp::step()
{
	#if USE_PHYSICS_LOOP
		_stepCount++;
		if ( _stepCount > 60 )
		{
			const seconds_t 
				Now = app::getElapsedSeconds(),
				Elapsed = Now - _stepCountTime;

			_averageStepsPerSecond = real( seconds_t(_stepCount) / Elapsed );
			_stepCount = 0;
			_stepCountTime = Now; 		
		}

		_scenario->dispatchStep( _physicsLoop->interval() );

	#else

		_scenario->dispatchStep();
		_averageStepsPerSecond = lrp<real>(0.1, _averageStepsPerSecond, 1.0/_scenario->stepTime().deltaT );

	#endif
}

void SurfacerApp::setGameScenario( GameScenario *s )
{
	if ( _scenario ) 
	{
		delete _scenario;
		_scenario = NULL;
	}
	
	_scenario = s;
}

#pragma mark - PhysicsLoop

/*
		SurfacerApp				*_app;
		int						_rate;
		seconds_t				_interval;
*/

PhysicsLoop::PhysicsLoop( SurfacerApp *app, int rate ):
	_app(app),
	_rate(rate),
	_interval( seconds_t(1) / seconds_t(rate))
{}

PhysicsLoop::~PhysicsLoop( void )
{}
	
#pragma mark - AdaptivePhysicsLoop

/*
		seconds_t _currentTime, _lastTime, _timeRemainder;
*/

AdaptivePhysicsLoop::AdaptivePhysicsLoop( SurfacerApp *app, int rate ):
	PhysicsLoop(app, rate),
	_timeRemainder(0)
{
	_currentTime = _lastTime = app->getElapsedSeconds();
}

AdaptivePhysicsLoop::~AdaptivePhysicsLoop( void )
{}
		
void 
AdaptivePhysicsLoop::step( void )
{
	//
	//	Update time and find out how much time
	//	has elapsed since last call to ::update
	//	
	//	Then add whatever was left over from last iteration.
	//

	_currentTime = app()->getElapsedSeconds();
	
	seconds_t 
		dt = _currentTime - _lastTime,
		interval = this->interval();
		   
	_lastTime = _currentTime;

	seconds_t availableTime = dt + _timeRemainder;
	
	if ( availableTime < interval )
	{
		_timeRemainder += dt;
	}
	else
	{
		seconds_t steps = availableTime / interval;
		int numSteps = lrint( steps );
				
		for ( int i=0; i < numSteps; i++ )
		{
			dispatchStep();
		}

		_timeRemainder = seconds_t(steps - seconds_t( numSteps )) * interval;		
	}
}

#pragma mark - SacredSoftwarePhysicsLoop

/*
		seconds_t _currentTime, _lastTime, _timeRemainderm, _cyclesLeftOver;
*/

SacredSoftwarePhysicsLoop::SacredSoftwarePhysicsLoop( SurfacerApp *app, int rate ):
	PhysicsLoop(app,rate),
	_timeRemainder(0),
	_cyclesLeftOver(0)
{
	_currentTime = _lastTime = app->getElapsedSeconds();
}

SacredSoftwarePhysicsLoop::~SacredSoftwarePhysicsLoop( void )
{}
		
void 
SacredSoftwarePhysicsLoop::step( void )
{
	/*	
		From:
		http://www.sacredsoftware.net/tutorials/Animation/TimeBasedAnimation.xhtml
					
		#define MAXIMUM_FRAME_RATE 120
		#define MINIMUM_FRAME_RATE 15
		#define UPDATE_INTERVAL (1.0 / MAXIMUM_FRAME_RATE)
		#define MAX_CYCLES_PER_FRAME (MAXIMUM_FRAME_RATE / MINIMUM_FRAME_RATE)
		
		void runGame() {
			static seconds_t lastFrameTime = 0.0;
			static seconds_t cyclesLeftOver = 0.0;
			seconds_t currentTime;
			seconds_t updateIterations;
			
			currentTime = GetCurrentTime();
			updateIterations = ((currentTime - lastFrameTime) + cyclesLeftOver);
			
			if (updateIterations > (MAX_CYCLES_PER_FRAME * UPDATE_INTERVAL)) {
				updateIterations = (MAX_CYCLES_PER_FRAME * UPDATE_INTERVAL);
			}
			
			while (updateIterations > UPDATE_INTERVAL) {
				updateIterations -= UPDATE_INTERVAL;
				
				updateGame(); // Update game state a variable number of times
			}
			
			cyclesLeftOver = updateIterations;
			lastFrameTime = currentTime;
			
			drawScene(); // Draw the scene only once
		}				
	*/
	
	const seconds_t MaxFrameRate = this->rate(),
				 MinFrameRate = 4,
				 UpdateInterval = this->interval(),
				 MaxCyclesPerFrame = ( MaxFrameRate / MinFrameRate );
	
	_currentTime = app()->getElapsedSeconds();

	seconds_t updateIterations = ((_currentTime - _lastTime) + _cyclesLeftOver);		
	updateIterations = std::min( updateIterations, MaxCyclesPerFrame * UpdateInterval );

	while( updateIterations > UpdateInterval )
	{
		updateIterations -= UpdateInterval;
		dispatchStep();
	}
	
	_cyclesLeftOver = updateIterations;
	_lastTime = _currentTime;
}

} // end namespace game