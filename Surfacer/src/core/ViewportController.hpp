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

	SMART_PTR(ViewportController);

	class ViewportController : public Camera2DInterface, public core::signals::receiver {
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

			A panFactor of 0.9 will, for example, cause the error between the controller
			set point and the current viewport value to be reduced by 90% each second.

		 */
		struct zeno_config {
			double panFactor, scaleFactor, rotationFactor;

			zeno_config():
			panFactor(0.98),
			scaleFactor(0.98),
			rotationFactor(0.98)
			{}

			zeno_config( double pf, double sf, double rf ):
			panFactor(pf),
			scaleFactor(sf),
			rotationFactor(rf)
			{}

		};

	public:

		ViewportController(control_method cm = Zeno );
		ViewportController( core::ViewportRef vp, control_method cm = Zeno );
		virtual ~ViewportController();

		///////////////////////////////////////////////////////////////////////////

		virtual void update( const core::time_state &time ) override;

		ViewportRef getViewport() const { return _viewport; }
		void setViewport(ViewportRef vp);


		void setControlMethod( control_method m ) { _controlMethod = m; }
		control_method getControlMethod() const { return _controlMethod; }

		void setZenoConfig( zeno_config zc ) { _zenoConfig = zc; }
		zeno_config &getZenoConfig() { return _zenoConfig; }
		const zeno_config &getZenoConfig() const { return _zenoConfig; }

		void setConstraintMask( unsigned int cmask );
		unsigned int getConstraintMask() const { return _constraintMask; }

		// Camera2DInterface

		void setViewport( int width, int height ) override;
		ci::Area getBounds() const override;

		void setScale( double z ) override { _scale = _constrainScale(z); }
		void setScale( double z, const dvec2 &about ) override;
		double getScale() const override { return _scale; }
		void setRotation(double rads) override;
		double getRotation() const override { return _rotation; }

		void setPan( const dvec2 &p ) override { _pan = _constrainPan( p ); }
		dvec2 getPan() const override { return _pan; }

		void lookAt(const dvec2 &world, double scale, const dvec2 &screen ) override;

		void lookAt(const dvec2 &world, double scale) override {
			Camera2DInterface::lookAt(world, scale);
		}

		void lookAt(const dvec2 &world) override {
			Camera2DInterface::lookAt(world);
		}

		dmat4 getModelViewMatrix() const override;
		dmat4 getInverseModelViewMatrix() const override;


	protected:

		dvec2 _constrainPan( dvec2 p ) const;
		double _constrainScale( double z ) const;

		virtual void _updateZeno( const core::time_state &time );
		virtual void _updatePID( const core::time_state &time );

		void _viewportInitiatedChange( const core::Viewport &vp );
		void _viewportBoundsChanged( const core::Viewport &vp, const ci::Area &bounds );

	private:

		ViewportRef _viewport;
		unsigned int _constraintMask;
		control_method _controlMethod;
		cpBB _levelBounds;
		double _scale, _rotation;
		dvec2 _pan;
		zeno_config _zenoConfig;
		bool _disregardViewportMotion;
	};
	
	
} // end namespace core

#endif /* ViewportController_hpp */
