//
//  Scenario.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/14/17.
//
//

#include "Scenario.hpp"

#include <cinder/gl/gl.h>

#include "Strings.h"

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
		Viewport _camera;
		time_state _time, _stepTime;
		render_state _renderState;
	 */

	Scenario::Scenario():
	_time(app::getElapsedSeconds(), 1.0/60.0, 0),
	_stepTime(app::getElapsedSeconds(), 1.0/60.0, 0),
	_renderState(_camera, RenderMode::GAME, 0,0,0,0 )
	{}

	Scenario::~Scenario()
	{}

	void Scenario::resize( ivec2 size )
	{}

	void Scenario::step( const time_state &time )
	{}

	void Scenario::update( const time_state &time )
	{}

	void Scenario::draw( const render_state &state )
	{}

	void Scenario::setRenderMode( RenderMode::mode mode )
	{
		_renderState.mode = mode;
		app::console() << "Scenario[" << this << "]::setRenderMode: " << RenderMode::toString( renderMode() ) << std::endl;
	}

	void Scenario::screenshot( const ci::fs::path &folderPath, const std::string &namingPrefix, const std::string format )
	{
		std::size_t index = 0;
		ci::fs::path fullPath;

		do {
			fullPath = folderPath / (namingPrefix + strings::str(index++) + "." + format );
		} while( ci::fs::exists( fullPath ));

		Surface s = app::copyWindowSurface();
		writeImage( fullPath.string(), s, ImageTarget::Options(), format );
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
		_camera.setViewport(size.x, size.y);
		resize(size);
	}

	void Scenario::_dispatchStep()
	{
		update_time( _stepTime );
		_stepTime.deltaT = clamp<seconds_t>(_stepTime.deltaT, STEP_INTERVAL * 0.9, STEP_INTERVAL * 1.1 );
		step( _stepTime );
	}

	void Scenario::_dispatchUpdate()
	{
		update_time( _time );
		update( _time );
	}

	void Scenario::_dispatchDraw()
	{
		_renderState.frame = app::getElapsedFrames();
		_renderState.pass = 0;
		_renderState.time = _time.time;
		_renderState.deltaT = _time.deltaT;
		draw( _renderState );
	}


}
