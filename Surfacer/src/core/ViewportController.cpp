//
//  ViewportController.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/16/17.
//
//

#include "ViewportController.hpp"

#include <algorithm>

#include "ChipmunkHelpers.hpp"

namespace {

	const double Epsilon = 1e-3;

}

namespace core {

	/*
		core::Viewport		&_viewport;
		unsigned int		_constraintMask;
		control_method		_controlMethod;
		cpBB				_levelBounds;
		double				_scale;
		dvec2				_pan;
		zeno_config			_zenoConfig;
		bool				_disregardViewportMotion;
	 */

	ViewportController::ViewportController( ViewportRef vp, control_method cm ):
	_viewport(vp),
	_constraintMask(NoConstraint),
	_controlMethod(cm),
	_levelBounds( cpBBInfinity ),
	_scale(vp->getScale()),
	_pan(vp->getPan()),
	_disregardViewportMotion(false)
	{
		_viewport->motion.connect(this, &ViewportController::_viewportInitiatedChange );
		_viewport->boundsChanged.connect(this, &ViewportController::_viewportBoundsChanged );
	}

	ViewportController::~ViewportController()
	{}

	void ViewportController::update( const time_state &time )
	{
		//
		//	smoothly apply the changes
		//

		switch( _controlMethod )
		{
			case Zeno:
				_updateZeno( time );
				break;

			case PID:
				_updatePID( time );
				break;
		}
	}

	void ViewportController::setConstraintMask( unsigned int cmask )
	{
		_constraintMask = cmask;
		if ( _constraintMask )
		{
			_pan = _constrainPan( _pan );
			_scale = _constrainScale( _scale );
		}
	}

	void ViewportController::setViewport( int width, int height ) {
		_viewport->setViewport(width, height);
	}

	ci::Area ViewportController::getBounds() const {
		return _viewport->getBounds();
	}

	void ViewportController::setScale( double z, const dvec2 &aboutScreen )
	{
		dmat4 mv, imv;
		Viewport::modelviewFor( _pan, _scale, mv );

		imv = inverse(mv);
		dvec2 aboutWorld = imv * aboutScreen;

		Viewport::modelviewFor( _pan, _constrainScale(z), mv );

		dvec2 postScaleAboutScreen = mv * aboutWorld;

		_pan = _constrainPan( _pan + ( aboutScreen - postScaleAboutScreen ) );
		_scale = _constrainScale( z );
	}

	void ViewportController::lookAt( const dvec2 &world, double scale, const dvec2 &screen )
	{
		_scale = _constrainScale(scale);

		dmat4 mv;
		Viewport::modelviewFor( _pan, _scale, mv );

		_pan = _constrainPan( _pan + (screen - (mv*world)));
	}

	dmat4 ViewportController::getModelview() const {
		return _viewport->getModelview();
	}

	dmat4 ViewportController::getInverseModelview() const {
		return _viewport->getInverseModelview();
	}

#pragma mark -
#pragma mark Private

	dvec2 ViewportController::_constrainPan( dvec2 pan ) const
	{
		if ( _levelBounds == cpBBInfinity ) return pan;

		if ( _constraintMask & ConstrainPan )
		{
			pan.x = std::min( pan.x, _levelBounds.l * _scale);
			pan.y = std::min( pan.y, _levelBounds.b * _scale);

			double right = (_levelBounds.r * _scale) - _viewport->getWidth(),
			top = (_levelBounds.t * _scale) - _viewport->getHeight();

			pan.x = std::max( pan.x, -right );
			pan.y = std::max( pan.y, -top );
		}

		return pan;
	}

	double ViewportController::_constrainScale( double scale ) const
	{
		if ( _levelBounds == cpBBInfinity ) return scale;

		if ( _constraintMask & ConstrainScale )
		{
			double minScaleWidth = _viewport->getWidth() / (_levelBounds.r - _levelBounds.l),
			minScaleHeight = _viewport->getHeight() / (_levelBounds.t - _levelBounds.b );

			scale = std::max( scale, std::max( minScaleWidth, minScaleHeight ));
		}
		else
		{
			scale = std::max( scale, Epsilon );
		}

		return scale;
	}


	void ViewportController::_updateZeno( const time_state &time )
	{
		_disregardViewportMotion = true;

		const dvec2 PanError = _pan - _viewport->getPan();
		const double ScaleError = _scale - _viewport->getScale();
		const double Rate = 1.0 / time.deltaT;
		const dvec2 Pan = _viewport->getPan() + PanError * std::pow(_zenoConfig.panFactor, Rate);
		const double Scale = _viewport->getScale() + ScaleError * std::pow(_zenoConfig.scaleFactor, Rate );
		_viewport->setPanAndScale(Pan, Scale);

		_disregardViewportMotion = false;
	}

	void ViewportController::_updatePID( const time_state &time )
	{
		_disregardViewportMotion = true;

		// TODO: unimplemented...

		_disregardViewportMotion = false;
	}

	void ViewportController::_viewportInitiatedChange( const Viewport &vp )
	{
		if ( !_disregardViewportMotion )
		{
			_pan = vp.getPan();
			_scale = vp.getScale();
		}
	}

	void ViewportController::_viewportBoundsChanged( const Viewport &vp, const Area &bounds )
	{
		_pan = _constrainPan( _pan );
		_scale = _constrainScale( _scale );
	}
	
	
} // end namespace core
