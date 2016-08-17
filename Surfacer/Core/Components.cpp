//
//  Components.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/11/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "Components.h"
#include "GameObject.h"
#include "ChipmunkDebugDraw.h"

using namespace ci;
namespace core {

#pragma mark - Component

std::string Component::description() const
{
	return "[" + name() + " owner: " + (owner() ? owner()->description() : std::string("null")) + "]";
}

Level *Component::level() const 
{ 
	return _owner->level(); 
}

#pragma mark - DrawComponent

DrawComponent::DrawComponent()
{
	setName( "DrawComponent" );
}

void DrawComponent::draw( const render_state &state )
{
	switch( state.mode )
	{
		case RenderMode::GAME:
			_drawGame( state );
			break;

		case RenderMode::DEVELOPMENT:
			_drawDevelopment( state );
			break;

		case RenderMode::DEBUG:
			_drawDebug( state );
			break;
			
		case RenderMode::COUNT: break;
	}
}

void DrawComponent::_debugDrawChipmunkBody( const render_state &state, cpBody *body )
{
	util::cpdd::DrawBody( body, state.viewport.reciprocalZoom() );
}

void DrawComponent::_debugDrawChipmunkConstraint( const render_state &state, cpConstraint *constraint, const ci::ColorA &color )
{
	util::cpdd::DrawConstraint( constraint, color, state.viewport.reciprocalZoom() );
}

void DrawComponent::_debugDrawChipmunkShape( const render_state &state, cpShape *shape, const ci::ColorA &fillColor, const ci::ColorA &strokeColor )
{
	util::cpdd::DrawShape( shape, fillColor, strokeColor );
}

void DrawComponent::_debugDrawChipmunkShapes( const render_state &state, const cpShapeSet &shapes )
{
	util::cpdd::DrawShapes( shapes, owner()->debugColor(), ci::ColorA(0,0,0,0.5) );
}

void DrawComponent::_debugDrawChipmunkShapes( const render_state &state, const cpShapeVec &shapes )
{
	foreach( cpShape *s, shapes )
	{
		util::cpdd::DrawShape( s, owner()->debugColor(), ci::ColorA(0,0,0,0.5));
	}
}

void DrawComponent::_debugDrawChipmunkConstraints( const render_state &state, const cpConstraintSet &constraints )
{
	util::cpdd::DrawConstraints( constraints, ci::ColorA( 0,0,0.5,1 ), state.viewport.reciprocalZoom() );
}

void DrawComponent::_debugDrawChipmunkConstraints( const render_state &state, const cpConstraintVec &constraints )
{
	real s = state.viewport.reciprocalZoom();
	foreach( cpConstraint *c, constraints )
	{
		util::cpdd::DrawConstraint( c, ci::ColorA(0,0,0.5,1 ), s );
	}
}

void DrawComponent::_debugDrawChipmunkBodies( const render_state &state, const cpBodySet &bodies )
{
	util::cpdd::DrawBodies( bodies, state.viewport.reciprocalZoom() );
}

void DrawComponent::_debugDrawChipmunkBodies( const render_state &state, const cpBodyVec &bodies )
{
	real s = state.viewport.reciprocalZoom();
	foreach( cpBody *body, bodies )
	{
		util::cpdd::DrawBody( body, s );
	}
}

#pragma mark - InputComponent

/*
	std::map< int, bool > _monitoredKeyStates;
*/

InputComponent::InputComponent()
{
	setName( "InputComponent" );
}

void InputComponent::monitorKey( int keyCode )
{
	_monitoredKeyStates[keyCode] = false;
}

void InputComponent::ignoreKey( int keyCode )
{
	_monitoredKeyStates.erase( keyCode );
}

bool InputComponent::isKeyDown( int keyCode ) const
{
	std::map< int, bool >::const_iterator pos = _monitoredKeyStates.find( keyCode );
	if ( pos != _monitoredKeyStates.end() )
	{
		return pos->second;
	}
	
	return false;
}

bool InputComponent::keyDown( const ci::app::KeyEvent &event )
{
	// if this is a key code we're monitoring, consume the event
	int keyCode = event.getCode();
	std::map< int, bool >::iterator pos = _monitoredKeyStates.find( keyCode );
	if ( pos != _monitoredKeyStates.end() )
	{
		pos->second = true;
		monitoredKeyDown(keyCode);
		return true;
	}
	
	return false;
}

bool InputComponent::keyUp( const ci::app::KeyEvent &event )
{
	// if this is a key code we're monitoring, consume the event
	int keyCode = event.getCode();
	std::map< int, bool >::iterator pos = _monitoredKeyStates.find( keyCode );
	if ( pos != _monitoredKeyStates.end() )
	{
		pos->second = false;
		monitoredKeyUp(keyCode);
		return true;
	}
	
	return false;
}

void InputComponent::ready()
{
	takeFocus();
}


}