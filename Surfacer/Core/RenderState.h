#pragma once

//
//  RenderState.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/11/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "Common.h"
#include "Viewport.h"

namespace core {

#pragma mark - RenderMode

namespace RenderMode {

	enum mode 
	{
		GAME,
		DEVELOPMENT,
		DEBUG,
		COUNT	
	};
	
	inline std::string toString( RenderMode::mode mode )
	{
		switch( mode )
		{
			case GAME: return "RenderMode::GAME"; break;
			case DEVELOPMENT: return "RenderMode::DEVELOPMENT"; break;
			case DEBUG: return "RenderMode::DEBUG"; break;
			case COUNT: return "RenderMode::COUNT"; break;
		}
		
		return "RenderMode::Unknown";
	}

}

#pragma mark - render_state

/**
	@struct render_state
	This is passed to objects which are rendering, and is intended to be a 
	repository for per-frame information needed to draw the scene.
*/
struct render_state {

	const Viewport &viewport;
	RenderMode::mode mode;
	std::size_t frame, pass;
	seconds_t time, deltaT;

	/**
		create a render_state
		@param vp The current viewport
		@param m The current RenderMode::mode
		@param f The current frame
		@param p The current pass
		@param t The current time
		
		Note that pass is optional. It's reserved for special situations where a composite GameObject
		draws multiple child game objects, and might need to render them multiple times, with them
		rendering effects after solid geometry, etc. Pass will generally be zero.		
	*/
	render_state( const Viewport &vp, RenderMode::mode m, std::size_t f, std::size_t p, seconds_t t, seconds_t dt ):
		viewport(vp),
		mode(m),
		frame(f),
		pass(p),
		time(t),
		deltaT(dt)
	{}
	
};

}