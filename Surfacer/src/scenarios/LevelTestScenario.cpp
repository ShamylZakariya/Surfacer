//
//  LevelTestScenario.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/28/17.
//
//

#include "LevelTestScenario.hpp"

#include "GameApp.hpp"
#include "Strings.hpp"

LevelTestScenario::LevelTestScenario():
_cameraController(getCamera())
{
	_cameraController.setConstraintMask( ViewportController::ConstrainPan | ViewportController::ConstrainScale );
}

LevelTestScenario::~LevelTestScenario() {

}

void LevelTestScenario::setup() {
	LevelRef level = make_shared<Level>("Hello Levels!");

	// add some game objects, etc

	setLevel(level);
}

void LevelTestScenario::cleanup() {
	setLevel(nullptr);
}

void LevelTestScenario::resize( ivec2 size ) {

}

void LevelTestScenario::step( const time_state &time ) {

}

void LevelTestScenario::update( const time_state &time ) {

}

void LevelTestScenario::draw( const render_state &state ) {
	//const ivec2 screenSize = getWindowSize();
	gl::clear( Color( 0.2, 0.2, 0.2 ) );

	{
		// apply camera modelview
		Viewport::ScopedState cameraState(getCamera());

		drawWorldCoordinateSystem(state);
	}

	//
	// now we're in screen space - draw fpf/sps
	//

	float fps = core::GameApp::get()->getAverageFps();
	float sps = core::GameApp::get()->getAverageSps();
	string info = core::strings::format("%.1f %.1f", fps, sps);
	gl::drawString(info, vec2(10,10), Color(1,1,1));

}

bool LevelTestScenario::mouseDown( const ci::app::MouseEvent &event ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getCamera().screenToWorld(_mouseScreen);

	if ( event.isAltDown() )
	{
		_cameraController.lookAt( _mouseWorld );
		return true;
	}

	if ( isKeyDown( app::KeyEvent::KEY_SPACE )) {
		return false;
	}

	return true;
}

bool LevelTestScenario::mouseUp( const ci::app::MouseEvent &event ) {
	return false;
}

bool LevelTestScenario::mouseWheel( const ci::app::MouseEvent &event ) {
	float zoom = _cameraController.getScale(),
	wheelScale = 0.1 * zoom,
	dz = (event.getWheelIncrement() * wheelScale);

	_cameraController.setScale( std::max( zoom + dz, 0.1f ), event.getPos() );

	return true;
}

bool LevelTestScenario::mouseMove( const ci::app::MouseEvent &event, const vec2 &delta ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getCamera().screenToWorld(_mouseScreen);
	return true;
}

bool LevelTestScenario::mouseDrag( const ci::app::MouseEvent &event, const vec2 &delta ) {
	_mouseScreen = event.getPos();
	_mouseWorld = getCamera().screenToWorld(_mouseScreen);

	if ( isKeyDown( app::KeyEvent::KEY_SPACE ))
	{
		_cameraController.setPan( _cameraController.getPan() + delta );
		return true;
	}

	return true;
}

bool LevelTestScenario::keyDown( const ci::app::KeyEvent &event ) {
	if (event.getChar() == 'r') {
		reset();
		return true;
	} else if (event.getCode() == ci::app::KeyEvent::KEY_SPACE) {

		return true;
	} else if (event.getCode() == ci::app::KeyEvent::KEY_BACKQUOTE) {
		setRenderMode( RenderMode::mode( (int(getRenderMode()) + 1) % RenderMode::COUNT ));
	}
	return false;
}

bool LevelTestScenario::keyUp( const ci::app::KeyEvent &event ) {
	return false;
}

void LevelTestScenario::reset() {
	cleanup();
	setup();
}

void LevelTestScenario::drawWorldCoordinateSystem(const render_state &state) {

	const cpBB frustum = state.viewport.getFrustum();
	const int gridSize = 10;
	const int majorGridSize = 100;
	const int firstY = static_cast<int>(floor(frustum.b / gridSize) * gridSize);
	const int lastY = static_cast<int>(floor(frustum.t / gridSize) * gridSize) + gridSize;
	const int firstX = static_cast<int>(floor(frustum.l / gridSize) * gridSize);
	const int lastX = static_cast<int>(floor(frustum.r / gridSize) * gridSize) + gridSize;

	const auto MinorLineColor = ColorA(1,1,1,0.05);
	const auto MajorLineColor = ColorA(1,1,1,0.25);
	const auto AxisColor = ColorA(1,0,0,1);
	gl::lineWidth(1.0 / state.viewport.getScale());

	for (int y = firstY; y <= lastY; y+= gridSize) {
		if (y == 0) {
			gl::color(AxisColor);
		} else if ( y % majorGridSize == 0) {
			gl::color(MajorLineColor);
		} else {
			gl::color(MinorLineColor);
		}
		gl::drawLine(vec2(firstX, y), vec2(lastX, y));
	}

	for (int x = firstX; x <= lastX; x+= gridSize) {
		if (x == 0) {
			gl::color(AxisColor);
		} else if ( x % majorGridSize == 0) {
			gl::color(MajorLineColor);
		} else {
			gl::color(MinorLineColor);
		}
		gl::drawLine(vec2(x, firstY), vec2(x, lastY));
	}
	
}

