//
//  Viewport.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/13/17.
//
//

#include "Viewport.hpp"
#include <cinder/gl/gl.h>

namespace core {

	/*
	 dmat4				_modelViewMatrix, _inverseModelViewMatrix;
	 dvec2				_pan;
	 double				_scale, _rScale;
	 AreaID				_bounds;
	 */

	Viewport::Viewport():
	_pan( 0,0 ),
	_scale(1),
	_rScale(1),
	_bounds(0,0,0,0)
	{
		setViewport( app::getWindowWidth(), app::getWindowHeight() );
		setPanAndScale( dvec2(0,0), 1 );
	}

	Viewport::~Viewport()
	{}

	void Viewport::set() const
	{
		//
		//	set viewport, projection for a bottom-left coordinate system and modelview
		//

		gl::viewport( _bounds.getX1(), _bounds.getY1(), _bounds.getWidth(), _bounds.getHeight() );
		gl::setMatricesWindow( getWidth(), getHeight(), false );
		gl::multViewMatrix(_modelViewMatrix);
	}

	void Viewport::setViewport( int width, int height )
	{
		if (width != _bounds.getWidth() || height != _bounds.getHeight()) {
			_bounds.set( 0,0, width, height );
			_update();

			boundsChanged( *this, _bounds );
		}
	}

	void Viewport::setScale( double z )
	{
		_scale = z;
		_update();
	}

	void Viewport::setScale( double s, const dvec2 &aboutScreen )
	{
		dmat4 mv, imv;
		createModelViewMatrix( getPan(), getScale(), mv );
		imv = inverse(mv);

		dvec2 aboutWorld = imv * aboutScreen;

		createModelViewMatrix( getPan(), s, mv );

		dvec2 postScaleAboutScreen = mv * aboutWorld;

		_pan += aboutScreen - postScaleAboutScreen;
		_scale = s;
		_update();
	}

	void Viewport::setPan( const dvec2 &p )
	{
		_pan = p;
		_update();
	}

	void Viewport::lookAt( const dvec2 &world, double scale, const dvec2 &screen )
	{
		//
		//	Update scale and compute appropriate modelview
		//

		_scale = scale;

		dmat4 mv;
		Viewport::createModelViewMatrix( _pan, _scale, mv );

		// update pan accordingly
		_pan = _pan + (screen - (mv * world));

		_update();
	}

	void Viewport::setPanAndScale( const dvec2 &pan, double scale )
	{
		_pan = pan;
		_scale = scale;
		_update();
	}

	void Viewport::_update()
	{
		_rScale = 1.0 / _scale;
		createModelViewMatrix( _pan, _scale, _modelViewMatrix );
		_inverseModelViewMatrix = inverse(_modelViewMatrix);
		
		motion( *this );
	}
	
}
