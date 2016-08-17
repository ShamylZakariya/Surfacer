#pragma once

//
//  InputDispatcher.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/14/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include <vector>

#include <cinder/app/App.h>
#include <cinder/Vector.h>
#include <cinder/app/MouseEvent.h>
#include <cinder/app/KeyEvent.h>
#include <cinder/app/FileDropEvent.h>
#include <cinder/app/ResizeEvent.h>

#include "Common.h"

namespace core {

class InputListener;

namespace ScreenCoordinateSystem {

	enum origin {
	
		TopLeft,
		BottomLeft
	
	};

}


/**
	@class InputDispatcher
*/
class InputDispatcher 
{
	public:
	
		/**
			Set the coordinate system the singleton InputDIspatcher will use.
			Defaults to ScreenCoordinateSystem::BottomLeft
		*/
		static void setScreenCoordinateSystemOrigin( ScreenCoordinateSystem::origin origin ) { _screenOrigin = origin; }
		ScreenCoordinateSystem::origin screenCoordinateSystemOrigin() { return _screenOrigin; }
	
		static InputDispatcher *get( InputDispatcher *customInputDispatcher = NULL )
		{
			if ( !_sInstance )
			{
				_sInstance = customInputDispatcher ? customInputDispatcher : new InputDispatcher();
			}
			
			return _sInstance;
		}
		

	public:
	
		virtual ~InputDispatcher();
		
		/**
			Push a listener to the top of the input receiver stack.
			This listener will be the first to receive input events.
		*/
		void pushListener( InputListener *listener );
		
		/**
			Remove a listener from the input receiver stack.
			It will no longer receive input events.
		*/
		void removeListener( InputListener *listener );
		
		/**
			Return the listener (if any) which has the topmost position in the listener stack
		*/
		InputListener *topListener() const;
		
		/**
			Pop the top-most listener from the input receiver stack.
		*/
		InputListener *popListener();
				
		/**
			Convenience function to test if a key is pressed
		*/
		inline bool isKeyDown( int keyCode ) const 
		{
			std::map< int, bool >::const_iterator pos( _keyPressState.find(keyCode));
			return pos != _keyPressState.end() ? pos->second : false;
		}

		/**
			Convenience function to get the current mouse position in screen coordinates
		*/
		inline Vec2i mousePosition() const { return _lastMouseEvent.getPos(); }
		
		void hideMouse();
		void unhideMouse();
		bool mouseHidden() const { return _mouseHidden; }

		bool isShiftDown() const { return _lastKeyEvent.isShiftDown(); }
		bool isAltDown() const { return _lastKeyEvent.isAltDown(); }
		bool isControlDown() const { return _lastKeyEvent.isControlDown(); }
		bool isMetaDown() const { return _lastKeyEvent.isMetaDown(); }
		bool isAccelDown() const { return _lastKeyEvent.isAccelDown(); }
		
	private:

		InputDispatcher();
		Vec2i _mouseDelta( const ci::app::MouseEvent &event );

	private:

		static InputDispatcher *_sInstance;
		static ScreenCoordinateSystem::origin _screenOrigin;

		std::vector< InputListener* > _listeners;
		std::map< int, bool > _keyPressState;
		ci::app::MouseEvent _lastMouseEvent;
		ci::app::KeyEvent _lastKeyEvent;
		Vec2i *_lastMousePosition;
		ci::CallbackId _mouseDownId, _mouseUpId, _mouseWheelId, _mouseMoveId, _mouseDragId, _keyDownId, _keyUpId;
		bool _mouseHidden;

		bool _mouseDown( ci::app::MouseEvent event );
		bool _mouseUp( ci::app::MouseEvent event );
		bool _mouseWheel( ci::app::MouseEvent event );
		bool _mouseMove( ci::app::MouseEvent event );
		bool _mouseDrag( ci::app::MouseEvent event );
		bool _keyDown( ci::app::KeyEvent event );
		bool _keyUp( ci::app::KeyEvent event );
};

/**
	@class InputListener
	Interface for classes which want to be notified of user input
	
	Derive your class from InputListener and then when you want
	to start listening to input, call InputListener::takeFocus() to push
	your listener to the top of the input listener stack.
	
	Your InputListener will automatically detach during destruction.
	
	Each of the input handling functions (mouseDown, mouseUp, etc ) return a boolean. Return true if your listener
	consumed the event. If so, the event will NOT be passed on to the next listener in the stack. If you ignore
	the event return true to let it pass on to the next listener.							
*/
class InputListener 
{
	public:

		friend class InputDispatcher;
	
		/**
			To use an InputListener, you mush get the singleton InputDispatcher and push your listener. The listener
			will become the first listener, and will receive input events first, before other listeners in the stack.			
		*/
		InputListener();
		virtual ~InputListener();
		
		/**
			Convenience function to make this InputListener the first listener in the dispatcher's stack
		*/
		void takeFocus();

		/**
			Convenience function to remove this listener from the dispatcher's stack
		*/
		void resignFocus();

		/**
			@return true if this InputListener has the topmost position in the input listener stack
		*/
		bool hasFocus() const;
		
		virtual bool mouseDown( const ci::app::MouseEvent &event ) { return false; }
		virtual bool mouseUp( const ci::app::MouseEvent &event ) { return false; }	
		virtual bool mouseWheel( const ci::app::MouseEvent &event ) { return false; }	
		virtual bool mouseMove( const ci::app::MouseEvent &event, const Vec2r &delta ) { return false; }	
		virtual bool mouseDrag( const ci::app::MouseEvent &event, const Vec2r &delta ) { return false; }	
		virtual bool keyDown( const ci::app::KeyEvent &event ) { return false; }	
		virtual bool keyUp( const ci::app::KeyEvent &event ) { return false; }	

		/**
			Convenience function to query if a key is currently pressed
		*/
		virtual bool isKeyDown( int keyCode ) const { return InputDispatcher::get()->isKeyDown( keyCode ); }

		Vec2i mousePosition() const { return InputDispatcher::get()->mousePosition(); }
		bool isShiftDown() const { return InputDispatcher::get()->isShiftDown(); }
		bool isAltDown() const { return InputDispatcher::get()->isAltDown(); }
		bool isControlDown() const { return InputDispatcher::get()->isControlDown(); }
		bool isMetaDown() const { return InputDispatcher::get()->isMetaDown(); }
		bool isAccelDown() const { return InputDispatcher::get()->isAccelDown(); }
				
};


}

