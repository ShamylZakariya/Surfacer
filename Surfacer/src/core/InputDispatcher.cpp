//
//  InputDispatcher.cpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 2/14/17.
//
//

#include "InputDispatcher.hpp"

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

			return app::MouseEvent(e.getWindow(),
								   initiator,
								   e.getX(),
								   app::getWindowHeight() - e.getY(),
								   modifiers,
								   e.getWheelIncrement(),
								   e.getNativeModifiers()
								   );
		}

		bool inputListenerDispatchIndexSorter(InputListener *a, InputListener *b) {
			return a->getDispatchReceiptIndex() < b->getDispatchReceiptIndex();
		}
	}

#pragma mark -
#pragma InputDispatcher

	/*
	 static InputDispatcher *_sInstance;
	 static ScreenCoordinateSystem::origin _screenOrigin;

	 std::vector< InputListener* > _listeners;
	 std::map< int, bool > _keyPressState;
	 app::MouseEvent _lastMouseEvent;
	 app::KeyEvent _lastKeyEvent;
	 ivec2 *_lastMousePosition;
	 Connection _mouseDownId, _mouseUpId, _mouseWheelId, _mouseModeId, _mouseDragId, _keyDownId, _keyUpId;
	 bool _mouseHidden;
	 */

	InputDispatcherRef InputDispatcher::_sInstance;
	ScreenCoordinateSystem::origin InputDispatcher::_screenOrigin = ScreenCoordinateSystem::BottomLeft;

	InputDispatcher::InputDispatcher(app::WindowRef window):
	_lastMousePosition(NULL),
	_lastKeyEvent(),
	_mouseHidden(false)
	{
		_mouseDownId = window->getSignalMouseDown().connect([this](app::MouseEvent &e){
			_mouseDown(e);
		});

		_mouseUpId = window->getSignalMouseUp().connect([this](app::MouseEvent &e){
			_mouseUp(e);
		});

		_mouseWheelId = window->getSignalMouseWheel().connect([this](app::MouseEvent &e){
			_mouseWheel(e);
		});

		_mouseMoveId = window->getSignalMouseMove().connect([this](app::MouseEvent &e){
			_mouseMove(e);
		});

		_mouseDragId = window->getSignalMouseDrag().connect([this](app::MouseEvent &e){
			_mouseDrag(e);
		});

		_keyDownId = window->getSignalKeyDown().connect([this](app::KeyEvent &e){
			_keyDown(e);
		});

		_keyUpId = window->getSignalKeyUp().connect([this](app::KeyEvent &e){
			_keyUp(e);
		});
	}

	InputDispatcher::~InputDispatcher()
	{
		_mouseDownId.disconnect();
		_mouseUpId.disconnect();
		_mouseWheelId.disconnect();
		_mouseMoveId.disconnect();
		_mouseDragId.disconnect();
		_keyDownId.disconnect();
		_keyUpId.disconnect();
	}

	void InputDispatcher::hideMouse() {
		app::AppBase::get()->hideCursor();
		_mouseHidden = true;
	}
	void InputDispatcher::unhideMouse() {
		app::AppBase::get()->showCursor();
		_mouseHidden = false;
	}

	void InputDispatcher::_addListener( InputListener* listener )
	{
		_removeListener( listener );
		_listeners.push_back( listener );
		_sortListeners();
	}

	void InputDispatcher::_removeListener( InputListener* listener )
	{
		_listeners.erase( remove( begin(_listeners), end(_listeners), listener ), end(_listeners));
	}

	InputListener* InputDispatcher::_topListener() const
	{
		return _listeners.empty() ? NULL : _listeners.back();
	}

	void InputDispatcher::_sortListeners() {
		sort(_listeners.begin(), _listeners.end(), inputListenerDispatchIndexSorter);
	}

	ivec2 InputDispatcher::_mouseDelta( const app::MouseEvent &event )
	{
		ivec2 delta(0,0);
		if ( _lastMousePosition )
		{
			delta = event.getPos() - *_lastMousePosition;
		}
		else
		{
			_lastMousePosition = new ivec2;
		}

		*_lastMousePosition = event.getPos();
		return delta;
	}

	bool InputDispatcher::_mouseDown( app::MouseEvent event )
	{
		_lastMouseEvent = TranslateMouseEvent(event, _screenOrigin);
		for ( auto listener : _listeners )
		{
			if (listener->isListening() && listener->mouseDown( _lastMouseEvent )) break;
		}

		return false;
	}

	bool InputDispatcher::_mouseUp( app::MouseEvent event )
	{
		_lastMouseEvent = TranslateMouseEvent(event, _screenOrigin);

		for ( auto listener : _listeners )
		{
			if(listener->isListening() && listener->mouseUp( _lastMouseEvent )) break;
		}

		return false;
	}

	bool InputDispatcher::_mouseWheel( app::MouseEvent event )
	{
		_lastMouseEvent = TranslateMouseEvent(event, _screenOrigin);

		for ( auto listener : _listeners )
		{
			if(listener->isListening() && listener->mouseWheel( _lastMouseEvent )) break;
		}

		return false;
	}

	bool InputDispatcher::_mouseMove( app::MouseEvent event )
	{
		_lastMouseEvent = TranslateMouseEvent(event, _screenOrigin);
		ivec2 delta = _mouseDelta( _lastMouseEvent );

		for ( auto listener : _listeners )
		{
			if(listener->isListening() && listener->mouseMove( _lastMouseEvent, delta )) break;
		}

		return false;
	}

	bool InputDispatcher::_mouseDrag( app::MouseEvent event )
	{
		_lastMouseEvent = TranslateMouseEvent(event, _screenOrigin);
		ivec2 delta = _mouseDelta( _lastMouseEvent );

		for ( auto listener : _listeners )
		{
			if(listener->isListening() && listener->mouseDrag( _lastMouseEvent, delta )) break;
		}

		return false;
	}

	bool InputDispatcher::_keyDown( app::KeyEvent event )
	{
		_keyPressState[ event.getCode() ] = true;
		_lastKeyEvent = event;

		for ( auto listener : _listeners )
		{
			if(listener->isListening() && listener->keyDown(event)) break;
		}

		return false;
	}

	bool InputDispatcher::_keyUp( app::KeyEvent event )
	{
		_keyPressState[ event.getCode() ] = false;
		_lastKeyEvent = event;

		for ( auto listener : _listeners )
		{
			if(listener->isListening() && listener->keyUp(event)) break;
		}

		return false;
	}

#pragma mark -
#pragma mark InputListener

	/*
		bool _listening;
		int _dispatchReceiptIndex;
	 */
	InputListener::InputListener():
	_listening(false),
	_dispatchReceiptIndex(0) {
		InputDispatcher::get()->_addListener(this);
	}

	InputListener::InputListener(int dispatchReceiptIndex):
	_listening(false),
	_dispatchReceiptIndex(dispatchReceiptIndex) {
		InputDispatcher::get()->_addListener(this);
	}

	InputListener::~InputListener() {
		InputDispatcher::get()->_removeListener( this );
	}

	void InputListener::setDispatchReceiptIndex(int newIndex) {
		_dispatchReceiptIndex = newIndex;
		InputDispatcher::get()->_sortListeners();
	}

}
