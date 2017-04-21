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

#pragma mark - Camera2DInterface

	dmat4 Camera2DInterface::createModelViewMatrix( const dvec2 &pan, double scale, double rotation) {
		/*
		 Pan is describing the location of the origin in screen space. Does this make sense????
		 
		 1) Perhaps I need to drop ViewportController for a while, so I don't have to write code twice. 
		 2) Consider a re-ordering of operations, - this will affect lookAt, etc

		 */
		auto t = glm::translate(dvec3(pan.x, pan.y, 0));
		//auto it = glm::translate(dvec3(-pan.x, -pan.y, 0));
		auto s = glm::scale(dvec3(scale, scale, 1));
		//auto r = glm::rotate(rotation, dvec3(0,0,1));

		return t * s;
	}

	cpBB Camera2DInterface::getFrustum() const {
		const dmat4 imv = getInverseModelViewMatrix();
		const double width = getWidth(), height = getHeight();
		cpBB bb = cpBBInvalid;
		cpBBExpand(bb, imv * dvec2( 0,0 ));
		cpBBExpand(bb, imv * dvec2( width,0 ));
		cpBBExpand(bb, imv * dvec2( width,height ));
		cpBBExpand(bb, imv * dvec2( 0,height ));
		return bb;
	}

#pragma mark - Viewport

	/*
		dmat4 _modelViewMatrix, _inverseModelViewMatrix;
		dvec2 _pan;
		double _scale, _rScale, _rotation;
		ci::Area _bounds;
	 */

	Viewport::Viewport():
	_pan( 0,0 ),
	_scale(1),
	_rScale(1),
	_rotation(0),
	_bounds(0,0,0,0)
	{
		setViewport( app::getWindowWidth(), app::getWindowHeight() );
		set( dvec2(0,0), 1, 0 );
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
		dmat4 mv = createModelViewMatrix(getPan(), getScale(), getRotation());
		dmat4 imv = inverse(mv);

		dvec2 aboutWorld = imv * aboutScreen;
		mv = createModelViewMatrix( getPan(), s, getRotation());

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

	void Viewport::setRotation(double rads) {
		_rotation = rads;
		_update();
	}

	void Viewport::lookAt( const dvec2 &world, double scale, const dvec2 &screen )
	{
		//
		//	Update scale and compute appropriate modelview
		//

		_scale = scale;
		dmat4 mv = createModelViewMatrix( _pan, _scale, _rotation );

		// update pan accordingly
		_pan = _pan + (screen - (mv * world));

		_update();
	}

	void Viewport::set( const dvec2 &pan, double scale, double rotationRads )
	{
		_pan = pan;
		_scale = scale;
		_rotation = rotationRads;
		_update();
	}

	void Viewport::_update()
	{
		_rScale = 1.0 / _scale;
		_modelViewMatrix = createModelViewMatrix( _pan, _scale, _rotation );
		_inverseModelViewMatrix = inverse(_modelViewMatrix);
		
		motion( *this );
	}
	
}
