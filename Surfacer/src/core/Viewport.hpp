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

#include "MathHelpers.hpp"
#include "Signals.hpp"

using namespace ci;
using namespace std;


namespace core {


	class Viewport {
	public:

		static inline void modelviewFor( const dvec2 &pan, double scale, dmat4 &mv )
		{
			mv = translate(dvec3(pan.x, pan.y, 0)) * ::scale(dvec3(scale, scale, scale));
		}

		signals::signal< void( const Viewport & ) > motion;
		signals::signal< void( const Viewport &, const ci::Area & ) > boundsChanged;

		class ScopedState : private Noncopyable {
		public:
			ScopedState(const Viewport &vp):
				_svp(vp._bounds.getX1(), vp._bounds.getY1(), vp._bounds.getWidth(), vp._bounds.getHeight())
			{
				gl::pushMatrices();
				gl::setMatricesWindow( vp._bounds.getWidth(), vp._bounds.getHeight(), false );
				gl::multViewMatrix(vp._modelview);
			}

			~ScopedState() {
				gl::popMatrices();
			}

		private:
			gl::ScopedViewport _svp;
		};

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

		ci::Area getBounds() const { return _bounds; }
		ivec2 getSize() const { return _bounds.getSize(); }
		int getWidth() const { return _bounds.getWidth(); }
		int getHeight() const { return _bounds.getHeight(); }

		/**
		 get the center of the current viewport
		 */
		dvec2 getViewportCenter() const { return dvec2(double(getWidth())/2,double(getHeight())/2); }

		/**
		 set the scale
		 */
		void setScale( double z );

		/**
		 set the scale, and pan such that @a about stays in the same spot on screen
		 */
		void setScale( double z, const dvec2 &about );

		/**
		 get the current scale
		 */
		double getScale() const { return _scale; }

		/**
		 get 1/current scale
		 */
		double getReciprocalScale() const { return _rScale; }

		/**
		 set the pan
		 */
		void setPan( const dvec2 &p );

		/**
		 get the current pan
		 */
		dvec2 getPan() const {
			return _pan;
		}

		/**
		 cause the viewport to pan such that @a world is at location @a screen with @a scale
		 @a world Location of interest in world coordinates
		 @a scale desired level of scale
		 @a screen focal point in screen coordinates

		 This is the function to use if, for example, you want the camera to pan to some
		 object or character of interest.
		 */
		void lookAt( const dvec2 &world, double scale, dvec2 screen );

		/**
		 cause the viewport to pan such that @a world is at the center of the viewport
		 @a world Location of interest in world coordinates
		 @a scale desired level of scale

		 This is the function to use if, for example, you want the camera to pan to some
		 object or character of interest.
		 */
		void lookAt( const dvec2 &world, double scale )
		{
			lookAt( world, scale, getViewportCenter() );
		}

		/**
		 cause the viewport to pan such that @a world is at the center of the viewport using the current scale
		 @a world Location of interest in world coordinates

		 This is the function to use if, for example, you want the camera to pan to some
		 object or character of interest.
		 */
		void lookAt( const dvec2 &world )
		{
			lookAt( world, getScale(), getViewportCenter() );
		}

		/**
		 set the pan and scale
		 @a pan the pan
		 @a scale the scale
		 */
		void setPanAndScale( const dvec2 &pan, double scale );

		/**
		 get the current modelview matrix
		 */
		const dmat4 &getModelview() const { return _modelview; }

		/**
		 get the current inverse modelview matrix
		 */
		const dmat4 &getInverseModelview() const { return _inverseModelview; }

		/**
		 convert a point in world coordinate system to screen
		 @note Assumes lower-left origin screen coordinate system
		 */
		dvec2 worldToScreen( const dvec2 &world ) const
		{
			return _modelview * world;
		}

		/**
		 convert a point in screen coordinate system to world.
		 @note Assumes lower-left origin screen coordinate system
		 */
		dvec2 screenToWorld( const dvec2 &screen ) const
		{
			return _inverseModelview * screen;
		}

		/**
		 get the current viewport frustum in world coordinates
		 */
		cpBB getFrustum() const;

	protected:

		void _update();
		
	protected:
		
		dmat4				_modelview, _inverseModelview;
		dvec2				_pan;
		double				_scale, _rScale;
		ci::Area			_bounds;
		
	};
	
} // namespace core


#endif /* Viewport_hpp */
