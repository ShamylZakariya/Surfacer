#pragma once

//
//  FilterStack.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/14/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "BlendMode.h"
#include "Common.h"
#include "TimeState.h"
#include "RenderState.h"
#include "ResourceManager.h"

#include <cinder/gl/gl.h>
#include <cinder/gl/Fbo.h>
#include <cinder/gl/GlslProg.h>

namespace core {

class FilterStack;

class Filter 
{
	public:
	
		Filter();
		virtual ~Filter();
		
		void setName( const std::string &name ) { _name = name; }
		const std::string &name() const { return _name; }
		
		/**
			set the strength of this filter.
			it is the responsibility of Filter implementations to 
			blend their effect in based on strength, where a strength of 0
			has no effect, and 1 is the entire effect, with a smooth interpolation between.
			
			note: filters with strength of 0 will be skipped by the stack.
		*/
		virtual void setStrength( real s ) { _strength = saturate(s); }
		real strength() const { return _strength; }
		
		FilterStack *stack() const { return _stack; }
		ResourceManager *resourceManager() const;
		ci::Area bounds() const { return _bounds; }
		
		virtual void resize( const Vec2i &newSize ){}
		virtual void update( const time_state &time ){}
		
		/**
			Run the filter. Return true if final output is in @a output. 
			If your filter ping pongs, return false if your final output is in @a input.
			Default implementation binds output FBO, sets up a default viewport and ortho projection
			for bottom-left screen rendering, and then calls _render.
		*/

		virtual bool execute( ci::gl::Fbo input, ci::gl::Fbo output, const render_state &state, const ci::ColorA &clearColor );
		
	protected:
	
		/**
			Perform filter rendering here
			input is the input image
		*/
		virtual void _render( ci::gl::Fbo input, const render_state &state ){};
	
		/**
			Draws a screen-aligned quad in ortho projection.
			basically, you'd bind your textures, bind your target FBO, bind your shaders
			and call _blit(). Then cleanup.
		*/
		void _blit();
		

	private:

		friend class FilterStack;
		void _resize( const Vec2i &newSize );
		void _setStack( FilterStack *s ) { _stack = s; }

	private:
		
		real _strength;
		std::string _name;
		ci::Area _bounds;
		FilterStack *_stack;
		Vec2r _vertices[4], _texCoords[4];
		

};

class FilterStack 
{
	public:
	
		FilterStack( ResourceManager *rm );
		virtual ~FilterStack();
		
		void push( Filter * );
		void pop();
		Filter *top() const;
		void remove( Filter * );
		const std::vector< Filter* > &filters() const { return _filters; }
		
		/** The blend mode used when compositing */
		void setBlendMode( const BlendMode &bm ) { _blendMode = bm; }
		const BlendMode &blendMode() const { return _blendMode; }
		
		void setCompositeColor( const ci::ColorA compositeColor ) { _compositeColor = compositeColor; }
		ci::ColorA compositeColor() const { return _compositeColor; }
		
		virtual void beginCapture();
		virtual void beginCapture( const ci::ColorA &clearColor );
		virtual void endCapture();
		virtual void execute( const render_state &state, bool blitToScreen, bool blitRespectsAlpha );
		virtual void blitToScreen( bool respectingAlpha );

		void beginCapture( const ci::Color &clearColor )
		{
			beginCapture(ci::ColorA(clearColor,1));
		}
		
		/**
			Execute a custom set of filters, returning the result
		*/
		virtual ci::gl::Fbo execute( const render_state &state, const std::vector<Filter*> &filters );
		
		const ci::gl::Fbo output() const { return _a; }
		ci::Area bounds() const { return _bounds; }
		
		virtual void resize( const Vec2i &newSize );
		virtual void update( const time_state &time );
		
		/**
			Convenience function for drawing an FBO to screen. 
			You will probably want to bind a shader before calling this.
			Uses FilterStack's current BlendMode.
		*/
		virtual void draw( ci::gl::Fbo fbo );
		
		void setResourceManager( ResourceManager *rm ) { _resourceManager = rm; }
		ResourceManager *resourceManager() const { return _resourceManager; }

	private:
	
		void _blitToScreen( const ci::gl::Fbo &fbo, bool respectingAlpha );

	private:
	
	
		ci::gl::Fbo _a, _b;
		std::vector< Filter* > _filters;
		
		ResourceManager *_resourceManager;

		ci::Area _bounds;
		Vec2r _vertices[4], _texCoords[4];

		ci::gl::GlslProg _blitAlphaCompositor;
		BlendMode _blendMode;
		ci::ColorA _clearColor, _compositeColor;

};




} // end namespace core