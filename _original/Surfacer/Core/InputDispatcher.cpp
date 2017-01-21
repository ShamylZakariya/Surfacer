//
//  InputDispatcher.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/14/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "InputDispatcher.h"


#pragma mark -
#pragma InputListener

using namespace ci;
namespace core {

namespace {

	app::MouseEvent TranslateMouseEvent( const app::MouseEvent &e, ScreenCoordinateSystem::origin origin )
	{
		// cinder dispatches mouse events for a top-left system
		if ( origin == ScreenCoordinateSystem::TopLeft ) return e;
	
		using namespace app;

		//
		//	MouseEvent is read only, so we have to reconstruct :(
		//

		int initiator = 0;
		unsigned int modifiers = 0;
			
		if ( e.isLeft() ) initiator |= MouseEvent::LEFT_DOWN;
		if ( e.isRight() ) initiator |= MouseEvent::RIGHT_DOWN;
		if ( e.isMiddle() ) initiator |= MouseEvent::MIDDLE_DOWN;

		if ( e.isLeftDown() ) modifiers |= MouseEvent::LEFT_DOWN;
		if ( e.isRightDown() ) modifiers |= MouseEvent::RIGHT_DOWN;
		if ( e.isMiddleDown() ) modifiers |= MouseEvent::MIDDLE_DOWN;

		if ( e.isShiftDown() ) modifiers |= MouseEvent::SHIFT_DOWN;
		if ( e.isAltDown() ) modifiers |= MouseEvent::ALT_DOWN;
		if ( e.isControlDown() ) modifiers |= MouseEvent::CTRL_DOWN;
		if ( e.isMetaDown() ) modifiers |= MouseEvent::META_DOWN;
		if ( e.isAccelDown() ) modifiers |= MouseEvent::ACCEL_DOWN;
	
		return app::MouseEvent(
			initiator,
			e.getX(),
			ci::app::getWindowHeight() - e.getY(),
			modifiers,
			e.getWheelIncrement(),
			e.getNativeModifiers()
		);
	}
}

#pragma mark -
#pragma InputDispatcher

/*
		static InputDispatcher *_sInstance;
		static ScreenCoordinateSystem::origin _screenOrigin;

		std::vector< InputListener* > _listeners;
		std::map< int, bool > _keyPressState;
		ci::app::MouseEvent _lastMouseEvent;
		ci::app::KeyEvent _lastKeyEvent;
		Vec2i *_lastMousePosition;
		ci::CallbackId _mouseDownId, _mouseUpId, _mouseWheelId, _mouseMoveId, _mouseDragId, _keyDownId, _keyUpId;
		bool _mouseHidden;
*/

InputDispatcher *InputDispatcher::_sInstance = NULL;
ScreenCoordinateSystem::origin InputDispatcher::_screenOrigin = ScreenCoordinateSystem::BottomLeft;

InputDispatcher::InputDispatcher():
	_lastMousePosition(NULL),
	_lastKeyEvent(0,0,0,0),
	_mouseHidden(false)
{	
	app::App *a = app::App::get();
	_mouseDownId = a->registerMouseDown( this, &InputDispatcher::_mouseDown );
	_mouseUpId = a->registerMouseUp( this, &InputDispatcher::_mouseUp );
	_mouseWheelId = a->registerMouseWheel( this, &InputDispatcher::_mouseWheel );
	_mouseMoveId = a->registerMouseMove( this, &InputDispatcher::_mouseMove );
	_mouseDragId = a->registerMouseDrag( this, &InputDispatcher::_mouseDrag );
	_keyDownId = a->registerKeyDown( this, &InputDispatcher::_keyDown );
	_keyUpId = a->registerKeyUp( this, &InputDispatcher::_keyUp );
}

InputDispatcher::~InputDispatcher()
{
	app::App *a = app::App::get();
	a->unregisterMouseDown( _mouseDownId );	
	a->unregisterMouseUp( _mouseUpId );	
	a->unregisterMouseWheel( _mouseWheelId );	
	a->unregisterMouseMove( _mouseMoveId );	
	a->unregisterMouseDrag( _mouseDragId );	
	a->unregisterKeyDown( _keyDownId );	
	a->unregisterKeyUp( _keyUpId );	
}

void InputDispatcher::pushListener( InputListener *listener )
{
	removeListener( listener );
	_listeners.push_back( listener );
}

void InputDispatcher::removeListener( InputListener *listener )
{
	_listeners.erase( std::remove( _listeners.begin(), _listeners.end(), listener ), _listeners.end());
}

InputListener *InputDispatcher::topListener() const
{ 
	return _listeners.empty() ? NULL : _listeners.back(); 
}

InputListener *InputDispatcher::popListener()
{
	if ( !_listeners.empty() )
	{
		InputListener *back = _listeners.back();
		_listeners.pop_back();
		return back;
	}
	
	return NULL;
}


Vec2i InputDispatcher::_mouseDelta( const ci::app::MouseEvent &event )
{
	Vec2i delta(0,0);
	if ( _lastMousePosition )
	{
		delta = event.getPos() - *_lastMousePosition;
	}
	else
	{
		_lastMousePosition = new Vec2i;
	}
	
	*_lastMousePosition = event.getPos();
	return delta;
}


bool InputDispatcher::_mouseDown( ci::app::MouseEvent event )
{
	_lastMouseEvent = TranslateMouseEvent(event, _screenOrigin);
	
	for ( std::vector< InputListener* >::const_reverse_iterator it( _listeners.rbegin()), end( _listeners.rend()); it != end; ++it )
	{
		if ((*it)->mouseDown( _lastMouseEvent )) break;
	}
	
	return false;
}

bool InputDispatcher::_mouseUp( ci::app::MouseEvent event )
{
	_lastMouseEvent = TranslateMouseEvent(event, _screenOrigin);

	for ( std::vector< InputListener* >::const_reverse_iterator it( _listeners.rbegin()), end( _listeners.rend()); it != end; ++it )
	{
		if((*it)->mouseUp( _lastMouseEvent )) break;
	}
	
	return false;
}

bool InputDispatcher::_mouseWheel( ci::app::MouseEvent event )
{
	_lastMouseEvent = TranslateMouseEvent(event, _screenOrigin);

	for ( std::vector< InputListener* >::const_reverse_iterator it( _listeners.rbegin()), end( _listeners.rend()); it != end; ++it )
	{
		if((*it)->mouseWheel( _lastMouseEvent )) break;
	}
	
	return false;
}

bool InputDispatcher::_mouseMove( ci::app::MouseEvent event )
{
	_lastMouseEvent = TranslateMouseEvent(event, _screenOrigin);
	Vec2i delta = _mouseDelta( _lastMouseEvent );

	for ( std::vector< InputListener* >::const_reverse_iterator it( _listeners.rbegin()), end( _listeners.rend()); it != end; ++it )
	{
		if((*it)->mouseMove( _lastMouseEvent, delta )) break;
	}
	
	return false;
}

bool InputDispatcher::_mouseDrag( ci::app::MouseEvent event )
{
	_lastMouseEvent = TranslateMouseEvent(event, _screenOrigin);
	Vec2i delta = _mouseDelta( _lastMouseEvent );

	for ( std::vector< InputListener* >::const_reverse_iterator it( _listeners.rbegin()), end( _listeners.rend()); it != end; ++it )
	{
		if((*it)->mouseDrag( _lastMouseEvent, delta )) break;
	}
	
	return false;
}

bool InputDispatcher::_keyDown( ci::app::KeyEvent event )
{
	_keyPressState[ event.getCode() ] = true;
	_lastKeyEvent = event;

	for ( std::vector< InputListener* >::const_reverse_iterator it( _listeners.rbegin()), end( _listeners.rend()); it != end; ++it )
	{
		if((*it)->keyDown(event)) break;
	}
	
	return false;
}

bool InputDispatcher::_keyUp( ci::app::KeyEvent event )
{
	_keyPressState[ event.getCode() ] = false;
	_lastKeyEvent = event;

	for ( std::vector< InputListener* >::const_reverse_iterator it( _listeners.rbegin()), end( _listeners.rend()); it != end; ++it )
	{
		if((*it)->keyUp(event)) break;
	}
	
	return false;
}

#pragma mark -
#pragma mark InputListener

InputListener::InputListener()
{}

InputListener::~InputListener()
{
	InputDispatcher::get()->removeListener( this );
}

void InputListener::takeFocus()
{
	InputDispatcher::get()->pushListener( this );
}

void InputListener::resignFocus()
{
	InputDispatcher::get()->removeListener( this );
}

bool InputListener::hasFocus() const
{
	return InputDispatcher::get()->topListener() == this;
}

}
