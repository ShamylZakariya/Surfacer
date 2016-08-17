#pragma once

/*
 *  Viewport.h
 *  LevelLoading
 *
 *  Created by Shamyl Zakariya on 2/6/11.
 *  Copyright 2011 Shamyl Zakariya. All rights reserved.
 *
 */
 
#include "Common.h"
#include "SignalsAndSlots.h"

#include <cinder/Area.h>

namespace core {

struct time_state;

class Viewport 
{
	public:

		static inline void modelviewFor( const Vec2r &pan, real zoom, Mat4r &mv )
		{			
			mv.setToIdentity();
			mv.translate( Vec3r( pan.x, pan.y, 0.0) );
			mv.scale( Vec3r( zoom, zoom, zoom ) );
		}
		
		signals::signal< void( const Viewport & ) > motion;
		signals::signal< void( const Viewport &, const ci::Area & ) > boundsChanged;
				
	public:
	
		Viewport();
		~Viewport();

		/**
			assign the current modelview for rendering
		*/
		void set() const;
		
		/**
			set the current viewport
		*/
		void setViewport( int width, int height );
		
		/**
			get the current viewport
		*/
		
		ci::Area bounds() const { return _bounds; }
		Vec2i size() const { return _bounds.getSize(); }
		int width() const { return _bounds.getWidth(); }
		int height() const { return _bounds.getHeight(); }
				
		/**
			get the center of the current viewport
		*/
		Vec2r viewportCenter() const { return Vec2r(real(width())/2,real(height())/2); }
		
		/**
			set the zoom
		*/
		void setZoom( real z );
		
		/**
			set the zoom, and pan such that @a about stays in the same spot on screen
		*/
		void setZoom( real z, const Vec2r &about );

		/**
			get the current zoom
		*/
		real zoom() const { return _zoom; }
		
		/**
			get 1/current zoom
		*/
		real reciprocalZoom() const { return _rZoom; }
		
		/**
			set the pan
		*/
		void setPan( const Vec2r &p );

		/**
			get the current pan
		*/
		Vec2r pan() const { 
			return _pan;
		}
				
		/**
			cause the viewport to pan such that @a world is at location @a screen with @a zoom
			@a world Location of interest in world coordinates
			@a zoom desired level of zoom
			@a screen focal point in screen coordinates
			
			This is the function to use if, for example, you want the camera to pan to some
			object or character of interest.			
		*/
		void lookAt( const Vec2r &world, real zoom, Vec2r screen );

		/**
			cause the viewport to pan such that @a world is at the center of the viewport
			@a world Location of interest in world coordinates
			@a zoom desired level of zoom
			
			This is the function to use if, for example, you want the camera to pan to some
			object or character of interest.			
		*/
		void lookAt( const Vec2r &world, real zoom )
		{
			lookAt( world, zoom, viewportCenter() );
		}
		
		/**
			cause the viewport to pan such that @a world is at the center of the viewport using the current zoom
			@a world Location of interest in world coordinates

			This is the function to use if, for example, you want the camera to pan to some
			object or character of interest.			
		*/
		void lookAt( const Vec2r &world )
		{
			lookAt( world, zoom(), viewportCenter() );
		}
		
		/**
			set the pan and zoom
			@a pan the pan
			@a zoom the zoom
		*/
		void setPanAndZoom( const Vec2r &pan, real zoom );

		/**
			get the current modelview matrix
		*/
		const Mat4r &modelview() const { return _modelview; }
		
		/**
			get the current inverse modelview matrix
		*/
		const Mat4r &inverseModelview() const { return _inverseModelview; }

		/**
			convert a point in world coordinate system to screen
			@note Assumes lower-left origin screen coordinate system
		*/
		Vec2r worldToScreen( const Vec2r &world ) const
		{
			return _modelview * world;
		}
		
		/**
			convert a point in screen coordinate system to world.
			@note Assumes lower-left origin screen coordinate system
		*/
		Vec2r screenToWorld( const Vec2r &screen ) const
		{
			return _inverseModelview * screen;
		}

		/**
			get the current viewport frustum in world coordinates
		*/
		cpBB frustum() const;

	protected:
			
		void _update();
	
	protected:

		Mat4r				_modelview, _inverseModelview;
		Vec2r				_pan;
		real				_zoom, _rZoom;
		ci::Area			_bounds;

};

} // namespace core
