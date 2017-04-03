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
#include "MathHelpers.hpp"
#include "Signals.hpp"
#include "TimeState.hpp"

using namespace ci;
using namespace std;


namespace core {

	SMART_PTR(Camera2DInterface);
	SMART_PTR(Viewport);

	class Camera2DInterface {
	public:

		static inline void modelviewFor( const dvec2 &pan, double scale, dmat4 &mv )
		{
			mv = translate(dvec3(pan.x, pan.y, 0)) * ::scale(dvec3(scale, scale, 1));
		}


	public:

		Camera2DInterface(){}

		virtual ~Camera2DInterface(){}

		/**
		 manage the current viewport
		 */

		virtual void setViewport( int width, int height ) = 0;
		virtual ci::Area getBounds() const = 0;
		virtual ivec2 getSize() const { return getBounds().getSize(); }
		virtual int getWidth() const { return getBounds().getWidth(); }
		virtual int getHeight() const { return getBounds().getHeight(); }
		virtual dvec2 getViewportCenter() const { return dvec2(getWidth()/2.0, getHeight()/2.0); }

		/**
			Set the scale
		 */
		virtual void setScale( double z ) = 0;

		/**
			set the scale and pan such that @a about stays in the same spot on screen
		 */
		virtual void setScale( double z, const dvec2 &about ) = 0;

		/**
			get the current scale
		 */
		virtual double getScale() const = 0;

		/**
			get the reciprocal of current scale
		 */
		virtual double getReciprocalScale() const { return 1.0 / getScale(); }

		/**
			set the pan
		 */
		virtual void setPan( const dvec2 &p ) = 0;

		/**
			get the current pan
		 */
		virtual dvec2 getPan() const = 0;

		/**
			cause the viewport to pan such that @a world is at location @a screen with @a scale
			@a world Location of interest in world coordinates
			@a scale desired level of scale
			@a screen focal point in screen coordinates

			This is the function to use if, for example, you want the camera to pan to some
			object or character of interest.
		 */
		virtual void lookAt( const dvec2 &world, double scale, const dvec2 &screen ) = 0;

		/**
			Shortcut to lookAt( const dvec2 &world, double scale, const dvec2 &screen )
		 */
		virtual void lookAt( const dvec2 &world, double scale ) { lookAt( world, scale, getViewportCenter() ); }

		/**
			Shortcut to lookAt( const dvec2 &world, double scale, const dvec2 &screen )
			Causes the camera to center @a world in the screen
		 */
		virtual void lookAt( const dvec2 &world ) { lookAt( world, getScale(), getViewportCenter() ); }

		/**
		 set the pan and scale
		 @a pan the pan
		 @a scale the scale
		 */
		virtual void setPanAndScale( const dvec2 &pan, double scale ) {
			setPan(pan);
			setScale(scale);
		}

		/**
		 get the current modelview matrix
		 */
		virtual dmat4 getModelview() const = 0;

		/**
		 get the current inverse modelview matrix
		 */
		virtual dmat4 getInverseModelview() const = 0;

		/**
		 convert a point in world coordinate system to screen
		 @note Assumes lower-left origin screen coordinate system
		 */
		virtual dvec2 worldToScreen( const dvec2 &world ) const
		{
			return getModelview() * world;
		}

		/**
		 convert a point in screen coordinate system to world.
		 @note Assumes lower-left origin screen coordinate system
		 */
		virtual dvec2 screenToWorld( const dvec2 &screen ) const
		{
			return getInverseModelview() * screen;
		}

		/**
		 get the current viewport frustum in world coordinates
		 */
		virtual cpBB getFrustum() const {
			const dmat4 imv = getInverseModelview();
			const dvec2 lb = imv * dvec2( 0,0 );
			const dvec2 tr = imv * dvec2( getWidth(), getHeight() );
			return cpBBNew( lb.x, lb.y, tr.x, tr.y );
		}

		/**	
		 If a camera needs to be time stepped
		 */
		virtual void step(const time_state &time){}
		virtual void update(const time_state &time){}

	};

	class Viewport : public Camera2DInterface {
	public:
		signals::signal< void( const Viewport & ) > motion;
		signals::signal< void( const Viewport &, const ci::Area & ) > boundsChanged;

		class ScopedState : private Noncopyable {
		public:
			ScopedState(const ViewportRef &vp):
				_svp(vp->getBounds().getX1(), vp->getBounds().getY1(), vp->getBounds().getWidth(), vp->getBounds().getHeight())
			{
				gl::pushMatrices();
				gl::setMatricesWindow( vp->getBounds().getWidth(), vp->getBounds().getHeight(), false );
				gl::multViewMatrix(vp->getModelview());
			}

			~ScopedState() {
				gl::popMatrices();
			}

		private:
			gl::ScopedViewport _svp;
		};

	public:

		Viewport();
		virtual ~Viewport();

		/**
		 assign the current modelview for rendering
		 */
		void set() const;

		//
		//	Camera2DInterface
		//

		void setViewport( int width, int height ) override;
		ci::Area getBounds() const override { return _bounds; }
		void setScale( double z ) override;
		void setScale( double z, const dvec2 &about ) override;
		double getScale() const override { return _scale; }
		void setPan( const dvec2 &p ) override;
		dvec2 getPan() const override { return _pan; }
		void lookAt( const dvec2 &world, double scale, const dvec2 &screen ) override;

		// TODO: Figure out why I have to explicitly define these overrides, when they should be found automatically
		void lookAt(const dvec2 &world, double scale) override {
			Camera2DInterface::lookAt(world, scale);
		}

		void lookAt(const dvec2 &world) override {
			Camera2DInterface::lookAt(world);
		}

		void setPanAndScale( const dvec2 &pan, double scale ) override;
		dmat4 getModelview() const override { return _modelview; }
		dmat4 getInverseModelview() const override { return _inverseModelview; }

	protected:

		void _update();
		
	protected:
		
		dmat4 _modelview, _inverseModelview;
		dvec2 _pan;
		double _scale, _rScale;
		ci::Area _bounds;
		
	};
	
} // namespace core


#endif /* Viewport_hpp */
