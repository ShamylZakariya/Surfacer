//
//  GameApp.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/14/17.
//
//

#include "GameApp.hpp"

#define USE_PHYSICS_LOOP 0

namespace core {


	void GameApp::prepareSettings( Settings *settings ) {
		settings->setTitle( "Controlled Demolition" );
		settings->setWindowSize( 800, 600 );
		settings->setFrameRate( 60 );

		// TODO: turning on HiDPI requires translation of mouse coords and viewport
		settings->setHighDensityDisplayEnabled(false);
	}

	GameApp::GameApp():
	_stepCount(0),
	_stepCountTime(0),
	_averageStepsPerSecond(0)
	{}

	GameApp::~GameApp()
	{}

	void GameApp::setup() {
		InputDispatcher::set(make_shared<InputDispatcher>(getWindow()));
	}

	void GameApp::cleanup() {
		CI_LOG_D("GameApp::cleanup - shutting down active scenario...");
		setScenario(nullptr);
	}

	void GameApp::update() {
		// run physics step()
#if USE_PHYSICS_LOOP
		_physicsLoop->step();
#else
		step();
#endif

		// update is always called after physics step()
		_scenario->_dispatchUpdate();
	}

	void GameApp::draw() {
		_scenario->_dispatchDraw();
	}

	void GameApp::resize() {
		_scenario->_dispatchResize(getWindowSize());
	}

	void GameApp::step() {
#if USE_PHYSICS_LOOP
		_stepCount++;
		if ( _stepCount > 60 )
		{
			const seconds_t
			Now = app::getElapsedSeconds(),
			Elapsed = Now - _stepCountTime;

			_averageStepsPerSecond = double( seconds_t(_stepCount) / Elapsed );
			_stepCount = 0;
			_stepCountTime = Now;
		}

		_scenario->_dispatchStep( _physicsLoop->interval() );

#else

		_scenario->_dispatchStep();
		_averageStepsPerSecond = lrp<double>(0.1, _averageStepsPerSecond, 1.0/_scenario->getStepTime().deltaT );

#endif
	}

	void GameApp::setScenario( ScenarioRef scenario ) {
		if (_scenario) {
			_scenario->_dispatchCleanup();
		}

		_scenario = scenario;

		if (_scenario) {
			_scenario->_dispatchSetup();
			_scenario->_dispatchResize(getWindowSize());
		}
	}

#pragma mark - PhysicsLoop

	/*
	 GameApp				*_app;
	 int						_rate;
	 seconds_t				_interval;
	 */

	PhysicsLoop::PhysicsLoop( GameApp *app, int rate ):
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

	AdaptivePhysicsLoop::AdaptivePhysicsLoop( GameApp *app, int rate ):
	PhysicsLoop(app, rate),
	_timeRemainder(0)
	{
		_currentTime = _lastTime = app->getElapsedSeconds();
	}

	AdaptivePhysicsLoop::~AdaptivePhysicsLoop( void )
	{}

	void
	AdaptivePhysicsLoop::step( void ) {
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
			size_t numSteps = lrint( steps );

			for ( size_t i=0; i < numSteps; i++ )
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

	SacredSoftwarePhysicsLoop::SacredSoftwarePhysicsLoop( GameApp *app, int rate ):
	PhysicsLoop(app,rate),
	_timeRemainder(0),
	_cyclesLeftOver(0)
	{
		_currentTime = _lastTime = app->getElapsedSeconds();
	}

	SacredSoftwarePhysicsLoop::~SacredSoftwarePhysicsLoop( void ) {}

	void
	SacredSoftwarePhysicsLoop::step( void ) {
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
