//
//  Scenario.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/14/17.
//
//

#include "Scenario.hpp"

#include <cinder/gl/gl.h>

#include "Level.hpp"
#include "Strings.hpp"

using namespace ci;
namespace core {

	namespace {

		const seconds_t STEP_INTERVAL( 1.0 / 60.0 );

		void update_time( time_state &time )
		{
			const seconds_t
				Now = app::getElapsedSeconds(),
				Elapsed = Now - time.time;

			//
			// We're using a variable timestep, but I'm damping its changes heavily
			//

			time.time = Now;
			time.deltaT = lrp<seconds_t>( 0.05, time.deltaT, Elapsed );
			time.step++;
		}

	}

	/*
		ViewportRef _viewport;
		time_state _time, _stepTime;
		render_state _renderState;
	 */

	Scenario::Scenario():
	_viewport(make_shared<Viewport>()),
	_viewportController(make_shared<ViewportController>(_viewport)),
	_time(app::getElapsedSeconds(), 1.0/60.0, 0),
	_stepTime(app::getElapsedSeconds(), 1.0/60.0, 0),
	_renderState(_viewport, RenderMode::GAME, 0,0,0,0 )
	{}

	Scenario::~Scenario()
	{}

	void Scenario::resize( ivec2 size )
	{}

	void Scenario::step( const time_state &time )
	{}

	void Scenario::update( const time_state &time )
	{}

	void Scenario::clear( const render_state &state )
	{}

	void Scenario::draw( const render_state &state )
	{}

	void Scenario::setViewportController(ViewportControllerRef vp) {
		_viewportController = vp;
		_viewportController->setViewport(_viewport);
	}

	void Scenario::setRenderMode( RenderMode::mode mode )
	{
		_renderState.mode = mode;
		app::console() << "Scenario[" << this << "]::setRenderMode: " << RenderMode::toString( getRenderMode() ) << endl;
	}

	void Scenario::screenshot( const ci::fs::path &folderPath, const string &namingPrefix, const string format )
	{
		size_t index = 0;
		ci::fs::path fullPath;

		do {
			fullPath = folderPath / (namingPrefix + str(index++) + "." + format );
		} while( ci::fs::exists( fullPath ));

		Surface s = app::copyWindowSurface();
		writeImage( fullPath.string(), s, ImageTarget::Options(), format );
	}

	void Scenario::setLevel(LevelRef level) {
		if (_level) {
			_level->removeFromScenario();
		}
		_level = level;
		if (_level) {
			_level->addedToScenario(shared_from_this());
		}
	}

	void Scenario::_dispatchSetup()
	{
		setup();
	}

	void Scenario::_dispatchCleanup()
	{
		cleanup();
	}

	void Scenario::_dispatchResize( const ivec2 &size )
	{
		_viewport->setViewport(size.x, size.y);
		if (_level) {
			_level->resize(size);
		}
		resize(size);
	}

	void Scenario::_dispatchStep()
	{
		update_time( _stepTime );

		// we don't wantphysics steps to vary wildly from the target
		_stepTime.deltaT = clamp<seconds_t>(_stepTime.deltaT, STEP_INTERVAL * 0.9, STEP_INTERVAL * 1.1 );

		step( _stepTime );

		if (_level) {
			_level->step(_stepTime);
		}

		_viewport->step(_stepTime);
		_viewportController->step(_stepTime);
	}

	void Scenario::_dispatchUpdate()
	{
		update_time( _time );

		update( _time );

		if (_level) {
			_level->update(_time);
		}

		_viewport->update(_time);
		_viewportController->update(_time);
	}

	void Scenario::_dispatchDraw()
	{
		_renderState.frame = app::getElapsedFrames();
		_renderState.pass = 0;
		_renderState.time = _time.time;
		_renderState.deltaT = _time.deltaT;

		clear(_renderState);

		if (_level) {
			Viewport::ScopedState vs(getViewport());
			_level->draw(_renderState);
		}

		draw( _renderState );
	}


}
