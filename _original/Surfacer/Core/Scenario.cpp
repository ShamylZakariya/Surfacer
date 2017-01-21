//
//  Scenario.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/14/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "Scenario.h"
#include <cinder/gl/gl.h>
#include <cinder/gl/Texture.h>
#include <cinder/app/App.h>
#include <cinder/ImageIo.h>

#include "RichText.h"

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
		ResourceManager				*_resourceManager;
		NotificationDispatcher		*_notificationDispatcher;
		util::rich_text::Renderer	*_richTextRenderer;
		ui::Stack					*_uiStack;
		Level						*_level;
		FilterStack					*_filters;

		Viewport					_camera;
		time_state					_time, _stepTime;
		render_state				_renderState;
*/

Scenario::Scenario():
	_resourceManager( new ResourceManager( NULL )),
	_notificationDispatcher(new NotificationDispatcher( this )),
	_richTextRenderer( new util::rich_text::Renderer( _resourceManager )),
	_uiStack( new ui::Stack( this ) ),
	_level(NULL),
	_filters( new FilterStack( _resourceManager )),
	_time(app::getElapsedSeconds(), 1.0/60.0, 0),
	_stepTime(app::getElapsedSeconds(), 1.0/60.0, 0),
	_renderState(_camera, RenderMode::GAME, 0,0,0,0 )
{
	//
	//	Set a default texture format which has mipmapping
	//

	gl::Texture::Format fmt;
	fmt.enableMipmapping();
	fmt.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );
	fmt.setMagFilter( GL_LINEAR );
	ResourceManager::setDefaultTextureFormat( fmt );
	
	_filters->setBlendMode( BlendMode( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA ));
	_filters->setCompositeColor( ci::Color::white() );
	
	_uiStack->resize( app::getWindowSize() );
}

Scenario::~Scenario()
{
	delete _richTextRenderer;
	delete _filters;
	delete _level;
	delete _uiStack;
	delete _resourceManager;
	delete _notificationDispatcher;
}

void Scenario::step( const time_state &time )
{
	if ( _level ) _level->step( time.deltaT );
}

void Scenario::update( const time_state &time )
{
	if ( _level ) _level->update( _time );
	_uiStack->update( _time );
	_filters->update( _time );
}

void Scenario::draw( const render_state &state )
{
	const bool
		ExecuteFilters = _renderState.mode == RenderMode::GAME,
		LevelBlitRespectsAlpha = false;

	if ( _level )
	{
		gl::pushModelView();
		{
			gl::SaveFramebufferBinding bindingSaver;
			_filters->beginCapture( Color::black() );

			_camera.set();

			//
			//	Render level
			//

			_level->draw( _renderState );

			//
			//	now run filters
			//
			
			_filters->endCapture();
			
			if ( ExecuteFilters )
			{
				_filters->execute( _renderState, true, LevelBlitRespectsAlpha );
			}
			else
			{
				_filters->blitToScreen(LevelBlitRespectsAlpha);
			}
		}
		gl::popModelView();
	}

	if ( !_uiStack->empty())
	{
		_uiStack->draw( _renderState );
	}
}

void Scenario::dispatchSetup()
{
	setup();
}

void Scenario::dispatchShutdown()
{
	shutdown();
}

void Scenario::dispatchResize( const Vec2i &size )
{
	_filters->resize( size );
	_camera.setViewport( size.x, size.y );
	_uiStack->resize( size );
	if ( _level ) _level->resize( size );

	resize(size);
}

void Scenario::dispatchStep()
{
	update_time( _stepTime );
	_stepTime.deltaT = clamp<seconds_t>(_stepTime.deltaT, STEP_INTERVAL * 0.9, STEP_INTERVAL * 1.1 );
	step( _stepTime );
}

void Scenario::dispatchUpdate()
{
	update_time( _time );
	update( _time );
}

void Scenario::dispatchDraw()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	gl::clear( Color(1,0,1) );
	
	gl::disable( GL_CULL_FACE );
	glDisable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glAlphaFunc( GL_GREATER, ALPHA_EPSILON );
	

	_renderState.frame = app::getElapsedFrames();
	_renderState.pass = 0;
	_renderState.time = _time.time;
	_renderState.deltaT = _time.deltaT;

	draw( _renderState );
}

void Scenario::setLevel( Level *l )
{
	if ( _level )
	{
		// restore rich text renderer's resource manager to this Scenario's
		_richTextRenderer->setResourceManager( _resourceManager );
		
		_level->_removedFromScenario(this);
		delete _level;
	}
	
	_level = l;
	
	if ( _level )
	{
		_level->_addedToScenario(this);
		
		// assign level's resource manager
		_richTextRenderer->setResourceManager( _level->resourceManager() );
	}
}

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
		fullPath = folderPath / (namingPrefix + str(index++) + "." + format );
	} while( ci::fs::exists( fullPath ));
	
	Surface s = app::copyWindowSurface();
	writeImage( fullPath.string(), s, ImageTarget::Options(), format );
}

}