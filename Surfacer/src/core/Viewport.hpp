//
//  Viewport.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/13/17.
//
//

#ifndef Viewport_hpp
#define Viewport_hpp

#include <cinder/Area.h>
#include <chipmunk/chipmunk.h>
#include <cinder/gl/scoped.h>

#include "Common.hpp"
#include "ChipmunkHelpers.hpp"
#include "MathHelpers.hpp"
#include "Signals.hpp"
#include "TimeState.hpp"

using namespace ci;
using namespace std;


namespace core {

	SMART_PTR(Viewport);

	class Viewport {
	public:

		signals::signal< void( const Viewport & ) > motion;
		signals::signal< void( const Viewport & ) > boundsChanged;

		struct look {
			dvec2 world;
			dvec2 up;

			look():
			world(0,0),
			up(0,1)
			{}

			look(dvec2 w, dvec2 u):
			world(w),
			up(u)
			{}

			look(const look &c):
			world(c.world),
			up(c.up)
			{}
		};


	public:

		Viewport();

		/**
		 manage the current viewport
		 */

		void setSize( int width, int height );
		ivec2 getSize() const { return ivec2(_width, _height); }
		int getWidth() const { return _width; }
		int getHeight() const { return _height; }
		dvec2 getCenter() const { return dvec2(_width/2.0, _height/2.0); }

		/**
			Set the scale - this effectively zooms in and out of whatever you're looking at
		 */
		void setScale( double z );

		/**
			get the current scale
		 */
		double getScale() const { return _scale; }

		/**
			get the reciprocal of current scale
		 */
		double getReciprocalScale() const { return 1.0 / _scale; }

		/**
		 center viewport on a position in the world, with a given up-vector
		 */
		void setLook(const look &l);

		/**
		 center viewport on a position in the world, with a given up-vector
		 */
		void setLook( const dvec2 &world, const dvec2 &up ) {
			setLook(look(world,up));
		}

		/**
		 center viewport on a position in the world
		 */
		void setLook( const dvec2 &world ) {
			setLook(look(world,dvec2(0,1)));
		}

		look getLook() const { return _look; }

		dmat4 getViewMatrix() const { return _viewMatrix; }
		dmat4 getInverseViewMatrix() const { return _inverseViewMatrix; }
		dmat4 getProjectionMatrix() const { return _projectionMatrix; }
		dmat4 getInverseProjectionMatrix() const { return _inverseProjectionMatrix; }
		dmat4 getViewProjectionMatrix() const { return _viewProjectionMatrix; }
		dmat4 getInverseViewProjectionMatrix() const { return _inverseViewProjectionMatrix; }

		/**
		 convert a point in world coordinate system to screen
		 */
		dvec2 worldToScreen( const dvec2 &world ) const
		{
			return _viewProjectionMatrix * world;
		}

		/**
		 convert a direction in world coordinate system to screen
		 */
		dvec2 worldToScreenDir( const dvec2 &world ) const
		{
			return dvec2(_viewProjectionMatrix * dvec4(world.x, world.y, 0.0, 0.0));
		}

		/**
		 convert a point in screen coordinate system to world.
		 */
		dvec2 screenToWorld( const dvec2 &screen ) const
		{
			return _inverseViewProjectionMatrix * screen;
		}

		/**
		 convert a direction in screen coordinate system to world.
		 */
		dvec2 screenToWorldDir( const dvec2 &screen ) const
		{
			return dvec2(_inverseViewProjectionMatrix * dvec4(screen.x, screen.y, 0.0, 0.0));
		}

		/**
		 get the current viewport frustum in world coordinates
		 */
		cpBB getFrustum() const;

		/**
		 apply the camera for rendering. sets gl::viewport, projection and view matrices
		 */
		void set();

	private:

		void _calcMatrices();

	private:

		int _width, _height;
		double _scale;
		look _look;
		dmat4 _viewMatrix, _inverseViewMatrix, _projectionMatrix, _inverseProjectionMatrix, _viewProjectionMatrix, _inverseViewProjectionMatrix;

	};

	
} // namespace core


#endif /* Viewport_hpp */
