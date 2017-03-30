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

namespace {

	SMART_PTR(BoxDrawComponent);
	SMART_PTR(OrbitMovement);
	SMART_PTR(ShapeGameObject);


	class OrbitMovement : public core::Component {
	public:
		OrbitMovement(vec2 orbitCenter, float orbitRadius, float radsPerSecond):
		_orbitCenter(orbitCenter),
		_orbitRadius(orbitRadius),
		_radsPerSecond(radsPerSecond),
		_rads(0) {}

		virtual ~OrbitMovement() {
			CI_LOG_D("dtor");
		}

		void onReady(GameObjectRef parent, LevelRef level) override{
			Component::onReady(parent, level);
			CI_LOG_D("onReady");
			_drawComponent = getSibling<DrawComponent>();
		}

		void step(const time_state &timeState) override{
		}

		void update(const time_state &timeState) override{
			_rads += _radsPerSecond * timeState.deltaT;
			if (DrawComponentRef dc = _drawComponent.lock()) {
				dc->notifyMoved();
			}
		}

		vec2 getCurrentPosition() const {
			return vec2(_orbitCenter.x + cos(_rads) * _orbitRadius, _orbitCenter.y + sin(_rads) * _orbitRadius);
		}

		float getRadiansPerSecond() const { return _radsPerSecond; }
		void setRadiansPerSecond(float rps) { _radsPerSecond = rps; }

	private:
		vec2 _orbitCenter;
		float _orbitRadius;
		float _radsPerSecond, _rads;
		DrawComponentWeakRef _drawComponent;
	};

	class BoxDrawComponent : public DrawComponent {
	public:
		BoxDrawComponent(vec2 size, Color color):
		_size(size),
		_color(color)
		{}

		virtual ~BoxDrawComponent() {
			CI_LOG_D("dtor");
		}

		void onReady(GameObjectRef parent, LevelRef level) override{
			DrawComponent::onReady(parent, level);
			CI_LOG_D("onReady");
			_orbitMovement = getSibling<OrbitMovement>();
		}

		cpBB getBB() const override {
			if (OrbitMovementRef movement = _orbitMovement.lock()) {
				return cpBBNewForExtents(cpv(movement->getCurrentPosition()), _size.x/2, _size.y/2);
			} else {
				return cpBBNew(0, 0, 0, 0);
			}
		}

		void draw(const core::render_state &renderState) override {
			gl::color(Color(1,0,1));
			cpBB bb = getBB();
			Rectf aabbRect(bb.l, bb.b, bb.r, bb.t);
			gl::drawSolidRect(aabbRect);
		}

		VisibilityDetermination::style getVisibilityDetermination() const override {
			return VisibilityDetermination::FRUSTUM_CULLING;
		}

		int getLayer() const override { return 0; }

	private:
		vec2 _size;
		Color _color;
		OrbitMovementWeakRef _orbitMovement;
	};

	class OrbitSpeedControlComponent : public InputComponent {
	public:
		OrbitSpeedControlComponent(){
			monitorKey(app::KeyEvent::KEY_UP);
			monitorKey(app::KeyEvent::KEY_DOWN);
			monitorKey(app::KeyEvent::KEY_ESCAPE);
		}

		void monitoredKeyDown( int keyCode ) override {

			const float deltaRadiansPerSecond = 10 * M_PI / 180;

			switch(keyCode){
				case app::KeyEvent::KEY_ESCAPE:
					CI_LOG_D("KEY_ESCAPE - Committing suicide :" << getGameObject()->getId());
					getGameObject()->setFinished();
					break;
				case app::KeyEvent::KEY_UP: {
					OrbitMovementRef mover = getSibling<OrbitMovement>();
					mover->setRadiansPerSecond(mover->getRadiansPerSecond()+deltaRadiansPerSecond);
					break;
				}
				case app::KeyEvent::KEY_DOWN: {
					OrbitMovementRef mover = getSibling<OrbitMovement>();
					mover->setRadiansPerSecond(mover->getRadiansPerSecond()-deltaRadiansPerSecond);
					break;
				}
			}
		}

	};

	GameObjectRef make_shape_object(string name, vec2 size, vec2 orbitCenter, float orbitRadius, float radsPerSecond) {
		GameObjectRef obj = make_shared<GameObject>(name);
		obj->addComponent(make_shared<BoxDrawComponent>(size, Color(1,0,0)));
		obj->addComponent(make_shared<OrbitMovement>(orbitCenter, orbitRadius, radsPerSecond));
		obj->addComponent(make_shared<OrbitSpeedControlComponent>());
		return obj;
	}

}

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
	level->addGameObject(make_shape_object("Shape0", vec2(50,50), vec2(0,0), 100, 0.05));

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
	_cameraController.step(time);
}

void LevelTestScenario::clear( const render_state &state ) {
	gl::clear( Color( 0.2, 0.2, 0.2 ) );
}

void LevelTestScenario::draw( const render_state &state ) {
	//const ivec2 screenSize = getWindowSize();

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

