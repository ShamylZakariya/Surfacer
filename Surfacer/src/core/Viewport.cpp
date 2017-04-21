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
		int _width, _height;
		double _scale;
		look _look;
		dmat4 _viewMatrix, _inverseViewMatrix, _projectionMatrix, _inverseProjectionMatrix, _viewProjectionMatrix, _inverseViewProjectionMatrix;
	 */

	Viewport::Viewport():
	_width(0),
	_height(0),
	_scale(1)
	{
		setSize( app::getWindowWidth(), app::getWindowHeight() );
		setLook(dvec2(0,0));
	}

	void Viewport::setSize(int width, int height) {
		if (_width != width || _height != height) {
			_width = width;
			_height = height;
			_calcMatrices();
			boundsChanged(*this);
		}
	}

	void Viewport::setScale( double z )
	{
		_scale = z;
		_calcMatrices();
	}

	void Viewport::setLook( const look &l ) {
		_look.world = l.world;

		// normalize up, falling back to (0,1) for invalid
		double len = glm::length(l.up);
		if (len > 1e-3) {
			_look.up = l.up / len;
		} else {
			_look.up = dvec2(0,1);
		}
		_calcMatrices();
	}

	cpBB Viewport::getFrustum() const {

		// frustum is axis-aligned box in world space wrapping the edges of the screen

		const double width = getWidth(), height = getHeight();
		cpBB bb = cpBBInvalid;
		cpBBExpand(bb, _inverseViewProjectionMatrix * dvec2( 0,0 ));
		cpBBExpand(bb, _inverseViewProjectionMatrix * dvec2( width,0 ));
		cpBBExpand(bb, _inverseViewProjectionMatrix * dvec2( width,height ));
		cpBBExpand(bb, _inverseViewProjectionMatrix * dvec2( 0,height ));

		return bb;
	}

	void Viewport::applyGLMatrices() {
		gl::viewport( 0, 0, _width, _height );
		gl::multProjectionMatrix(_projectionMatrix);
		gl::multViewMatrix(_viewMatrix);
	}

	void Viewport::_calcMatrices() {
		// HERE THERE BE DRAGONS - use glm::ortho and glm::lookAt

		dvec2 center = getCenter();
		double halfWidth = _width/2.0;
		double halfHeight = _height/2.0;
		_projectionMatrix = glm::ortho(center.x - _scale * halfWidth, center.x + _scale * halfWidth, center.y - _scale * halfHeight, center.y + _scale * halfHeight, -0.1, 100.0);
		_inverseProjectionMatrix = glm::inverse(_projectionMatrix);

		_viewMatrix = glm::lookAt(dvec3(_look.world, -1), dvec3(_look.world,0), dvec3(_look.up,0));
		_inverseViewMatrix = glm::inverse(_viewMatrix);

		_viewProjectionMatrix = _projectionMatrix * _viewMatrix;
		_inverseViewProjectionMatrix = glm::inverse(_viewProjectionMatrix);
	}


	
}
