/*
 *  Viewport.cpp
 *  LevelLoading
 *
 *  Created by Shamyl Zakariya on 2/6/11.
 *  Copyright 2011 Shamyl Zakariya. All rights reserved.
 *
 */

#include "Viewport.h"
#include <cinder/app/AppBasic.h>

#include "GameObject.h"

using namespace ci;

namespace core {

/*
		Mat4r				_modelview, _inverseModelview;
		Vec2r				_pan;
		real				_zoom, _rZoom;
		AreaID				_bounds;
*/

Viewport::Viewport():
	_pan( 0,0 ),
	_zoom(1),
	_rZoom(1),
	_bounds(0,0,0,0)
{
	setViewport( app::getWindowWidth(), app::getWindowHeight() );
	setPanAndZoom( Vec2r(0,0), 1 );
}

Viewport::~Viewport()
{}

void Viewport::set() const
{
	//
	//	set viewport, projection for a bottom-left coordinate system and modelview
	//

	gl::setViewport( _bounds );
	gl::setMatricesWindow( width(), height(), false );
	gl::multModelView( _modelview );	
}

void Viewport::setViewport( int width, int height )
{
	_bounds.set( 0,0, width, height );
	_update();

	boundsChanged( *this, _bounds );
}

void Viewport::setZoom( real z )
{
	_zoom = z;
	_update();
}

void Viewport::setZoom( real z, const Vec2r &aboutScreen )
{
	Mat4r mv, imv;
	modelviewFor( pan(), zoom(), mv );
	imv = mv.affineInverted();
	Vec2r aboutWorld = imv * aboutScreen;
	
	modelviewFor( pan(), z, mv );

	Vec2r postZoomAboutScreen = mv * aboutWorld;

	_pan += aboutScreen - postZoomAboutScreen;
	_zoom = z;
	_update();
}

void Viewport::setPan( const Vec2r &p )
{
	_pan = p;
	_update();
}

void Viewport::lookAt( const Vec2r &world, real zoom, Vec2r screen )
{
	//
	//	Update zoom and compute appropriate modelview
	//

	_zoom = zoom;

	Mat4r mv;
	Viewport::modelviewFor( _pan, _zoom, mv );

	// update pan accordingly
	_pan = _pan + (screen - mv * world );

	_update();
}

void Viewport::setPanAndZoom( const Vec2r &pan, real zoom )
{
	_pan = pan;
	_zoom = zoom;
	_update();
}

cpBB Viewport::frustum() const
{
	Vec2r lb = _inverseModelview * Vec2r( 0,0 ),
	      tr = _inverseModelview * Vec2r( _bounds.getWidth(),_bounds.getHeight() );	

	return cpBBNew( lb.x, lb.y, tr.x, tr.y );
}

void Viewport::_update()
{
	_rZoom = 1 / _zoom;
	modelviewFor( _pan, _zoom, _modelview );
	_inverseModelview = _modelview.affineInverted();
	
	motion( *this );
}

}