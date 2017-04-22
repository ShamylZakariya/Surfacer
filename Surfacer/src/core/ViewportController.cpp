//
//  ViewportController.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/16/17.
//
//

#include "ViewportController.hpp"

namespace core {

	/*
		ViewportRef _viewport;
		Viewport::look _look;
		double _scale;

		zeno_config _zenoConfig;
		bool _disregardViewportMotion;
	 */

	ViewportController::ViewportController( ViewportRef vp):
	_viewport(nullptr),
	_disregardViewportMotion(false)
	{
		setViewport(vp);
	}

	void ViewportController::update( const time_state &time )
	{
		_disregardViewportMotion = true;

		const double Rate = 1.0 / time.deltaT;
		Viewport::look look = _viewport->getLook();

		const dvec2 lookWorldError = _look.world - look.world;
		const dvec2 lookUpError = _look.up - look.up;
		const double scaleError = _scale - _viewport->getScale();

		look.world = look.world + lookWorldError * std::pow(_zenoConfig.lookTargetFactor, Rate);
		look.up = look.up + lookUpError * std::pow(_zenoConfig.lookUpFactor, Rate);
		const double newScale = _viewport->getScale() + scaleError * std::pow(_zenoConfig.scaleFactor, Rate);

		_viewport->setScale(newScale);
		_viewport->setLook(look);

		_disregardViewportMotion = false;
	}

	void ViewportController::setViewport(ViewportRef vp) {
		if (_viewport) {
			_viewport->motion.disconnect(this);
			_viewport->boundsChanged.disconnect(this);
		}
		_viewport = vp;
		if (_viewport) {
			_scale = _viewport->getScale();
			_look = _viewport->getLook();
			_viewport->motion.connect(this, &ViewportController::_viewportInitiatedChange );
			_viewport->boundsChanged.connect(this, &ViewportController::_viewportBoundsChanged );
		} else {
			_scale = 1;
			_look = { dvec2(0,0), dvec2(0,1) };
		}
	}

	void ViewportController::setLook( const Viewport::look l ) {
		_look.world = l.world;

		double len = glm::length(l.up);
		if (len > 1e-3) {
			_look.up = l.up / len;
		} else {
			_look.up = dvec2(0,1);
		}
	}

	void ViewportController::setScale(double scale) {
		_scale = clamp(scale, 0.05, 2000.0);
	}

#pragma mark -
#pragma mark Private

	void ViewportController::_viewportInitiatedChange( const Viewport &vp )
	{
		if ( !_disregardViewportMotion )
		{
			_look = vp.getLook();
			_scale = vp.getScale();
		}
	}

	void ViewportController::_viewportBoundsChanged( const Viewport &vp )
	{
		if (!_disregardViewportMotion) {
			_look = vp.getLook();
			_scale = vp.getScale();
		}
	}
	
	
} // end namespace core
