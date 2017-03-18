//
//  ViewportController.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/16/17.
//
//

#ifndef ViewportController_hpp
#define ViewportController_hpp

#include "Signals.hpp"
#include "TimeState.hpp"
#include "Viewport.hpp"

namespace core {

	class ViewportController : public core::signals::receiver {
	public:

		enum constraint {
			NoConstraint = 0,
			ConstrainPan = 1,
			ConstrainScale = 2
		};

		enum control_method {
			Zeno,
			PID
		};

		/**
			Configurator structor for Zeno control method.
			There are two fields, panFactor and scaleFactor which are
			in the range of [0,1], representing the interpolation distance
			to take from the current viewport value to the controller's set point.

			A panFactor of 0.1 will, for example, cause the error between the controller
			set point and the current viewport value to be reduced by 90% (1-factor) each timestep.

		 */
		struct zeno_config {
			float panFactor, scaleFactor;

			zeno_config():
			panFactor(0.01),
			scaleFactor(0.01)
			{}

			zeno_config( float pf, float zf ):
			panFactor(pf),
			scaleFactor(zf)
			{}

		};

	public:

		ViewportController( core::Viewport &vp, control_method cm = Zeno );
		virtual ~ViewportController();

		///////////////////////////////////////////////////////////////////////////

		virtual void step( const core::time_state &time );

		core::Viewport &getViewport() const { return _viewport; }

		void setControlMethod( control_method m ) { _controlMethod = m; }
		control_method getControlMethod() const { return _controlMethod; }

		void setZenoConfig( zeno_config zc ) { _zenoConfig = zc; }
		zeno_config &getZenoConfig() { return _zenoConfig; }
		const zeno_config &getZenoConfig() const { return _zenoConfig; }

		void setConstraintMask( unsigned int cmask );
		unsigned int getConstraintMask() const { return _constraintMask; }

		/**
			Set the scale
		 */
		void setScale( float z ) { _scale = _constrainScale(z); }

		/**
			set the scale and pan such that @a about stays in the same spot on screen
		 */
		void setScale( float z, const vec2 &about );

		/**
			get the current scale
		 */
		float getScale() const { return _scale; }

		/**
			set the pan
		 */
		void setPan( const vec2 &p ) { _pan = _constrainPan( p ); }

		/**
			get the current pan
		 */
		vec2 getPan() const {
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
		void lookAt( const vec2 &world, float scale, const vec2 &screen );

		/**
			Shortcut to lookAt( const vec2 &world, float scale, const vec2 &screen )
		 */
		void lookAt( const vec2 &world, float scale ) { lookAt( world, scale, _viewport.getViewportCenter() ); }

		/**
			Shortcut to lookAt( const vec2 &world, float scale, const vec2 &screen )
			Causes the camera to center @a world in the screen
		 */
		void lookAt( const vec2 &world ) { lookAt( world, _scale, _viewport.getViewportCenter() ); }

	protected:

		vec2 _constrainPan( vec2 p ) const;
		float _constrainScale( float z ) const;

		virtual void _stepZeno( const core::time_state &time );
		virtual void _stepPID( const core::time_state &time );

		void _viewportInitiatedChange( const core::Viewport &vp );
		void _viewportBoundsChanged( const core::Viewport &vp, const ci::Area &bounds );

	private:

		core::Viewport		&_viewport;
		unsigned int		_constraintMask;
		control_method		_controlMethod;
		cpBB				_levelBounds;
		float				_scale;
		vec2				_pan;
		zeno_config			_zenoConfig;
		bool				_disregardViewportMotion;
	};
	
	
} // end namespace core

#endif /* ViewportController_hpp */
