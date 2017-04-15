//
//  GameLevelTestScenario.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/13/17.
//
//

#include "GameLevelTestScenario.hpp"

#include "GameApp.hpp"
#include "GameLevel.hpp"
#include "Strings.hpp"
#include "Terrain.hpp"

namespace {

	class WorldCoordinateSystemDrawComponent : public DrawComponent {
	public:

		WorldCoordinateSystemDrawComponent(int gridSize = 10):
		_gridSize(gridSize){}

		void onReady(GameObjectRef parent, LevelRef level) override{
			DrawComponent::onReady(parent, level);
		}

		cpBB getBB() const override {
			return cpBBInfinity;
		}

		void draw(const core::render_state &state) override {
			const cpBB frustum = state.viewport->getFrustum();
			const int gridSize = _gridSize;
			const int majorGridSize = _gridSize * 10;
			const int firstY = static_cast<int>(floor(frustum.b / gridSize) * gridSize);
			const int lastY = static_cast<int>(floor(frustum.t / gridSize) * gridSize) + gridSize;
			const int firstX = static_cast<int>(floor(frustum.l / gridSize) * gridSize);
			const int lastX = static_cast<int>(floor(frustum.r / gridSize) * gridSize) + gridSize;

			const auto MinorLineColor = ColorA(1,1,1,0.05);
			const auto MajorLineColor = ColorA(1,1,1,0.25);
			const auto AxisColor = ColorA(1,0,0,1);
			gl::lineWidth(1.0 / state.viewport->getScale());

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

		VisibilityDetermination::style getVisibilityDetermination() const override {
			return VisibilityDetermination::ALWAYS_DRAW;
		}

		int getLayer() const override { return -1; }

	private:

		int _gridSize;

	};

	class MousePickComponent : public core::InputComponent {
	public:

		MousePickComponent(cpShapeFilter pickFilter):
		InputComponent(0),
		_pickFilter(pickFilter),
		_mouseBody(nullptr),
		_draggingBody(nullptr),
		_mouseJoint(nullptr)
		{}

		virtual ~MousePickComponent() {
			releaseDragConstraint();
			cpBodyFree(_mouseBody);
		}

		void onReady(GameObjectRef parent, LevelRef level) override {
			InputComponent::onReady(parent, level);
			_mouseBody = cpBodyNewKinematic();
		}

		void step(const time_state &time) override {
			cpVect mouseBodyPos = cpv(_mouseWorld);
			cpVect newMouseBodyPos = cpvlerp(cpBodyGetPosition(_mouseBody), mouseBodyPos, 0.75);

			cpBodySetVelocity(_mouseBody, cpvmult(cpvsub(newMouseBodyPos, cpBodyGetPosition(_mouseBody)), time.deltaT));
			cpBodySetPosition(_mouseBody, newMouseBodyPos);
		}

		bool mouseDown( const ci::app::MouseEvent &event ) override {
			releaseDragConstraint();

			_mouseScreen = event.getPos();
			_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);

			const float distance = 1.f;
			cpPointQueryInfo info = {};
			cpShape *pick = cpSpacePointQueryNearest(getLevel()->getSpace(), cpv(_mouseWorld), distance, _pickFilter, &info);
			if (pick) {
				cpBody *pickBody = cpShapeGetBody(pick);

				if (pickBody && cpBodyGetType(pickBody) == CP_BODY_TYPE_DYNAMIC) {
					cpVect nearest = (info.distance > 0.0f ? info.point : cpv(_mouseWorld));

					_draggingBody = pickBody;
					_mouseJoint = cpPivotJointNew2(_mouseBody, _draggingBody, cpvzero, cpBodyWorldToLocal(_draggingBody,nearest));

					cpSpaceAddConstraint(getLevel()->getSpace(), _mouseJoint);
					
					return true;
				}
			}

			return false;
		}

		bool mouseUp( const ci::app::MouseEvent &event ) override {
			releaseDragConstraint();
			return false;
		}

		bool mouseMove( const ci::app::MouseEvent &event, const ivec2 &delta ) override {
			_mouseScreen = event.getPos();
			_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);
			return false;
		}

		bool mouseDrag( const ci::app::MouseEvent &event, const ivec2 &delta ) override {
			_mouseScreen = event.getPos();
			_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);
			return false;
		}

		void releaseDragConstraint() {
			if (_mouseJoint) {
				cpSpaceRemoveConstraint(getLevel()->getSpace(), _mouseJoint);
				cpConstraintFree(_mouseJoint);
				_mouseJoint = nullptr;
				_draggingBody = nullptr;
			}
		}


	private:

		cpShapeFilter _pickFilter;
		cpBody *_mouseBody, *_draggingBody;
		cpConstraint *_mouseJoint;
		vec2 _mouseScreen, _mouseWorld;

	};

	class MouseCutterComponent : public core::InputComponent {
	public:

		MouseCutterComponent(terrain::TerrainObjectRef terrain, cpShapeFilter cutFilter, float radius):
		InputComponent(1),
		_cutting(false),
		_radius(radius),
		_cutFilter(cutFilter),
		_terrain(terrain)
		{}

		virtual ~MouseCutterComponent() {
		}

		void onReady(GameObjectRef parent, LevelRef level) override {
			InputComponent::onReady(parent, level);
		}

		void step(const time_state &time) override {
		}

		bool mouseDown( const ci::app::MouseEvent &event ) override {
			_mouseScreen = event.getPos();
			_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);

			_cutting = true;
			_cutStart = _cutEnd = _mouseWorld;

			return false;
		}

		bool mouseUp( const ci::app::MouseEvent &event ) override {
			if (_cutting) {
				if (_terrain) {
					const float radius = _radius / getLevel()->getViewport()->getScale();
					_terrain->getWorld()->cut(_cutStart, _cutEnd, radius, _cutFilter);
				}

				_cutting = false;
			}
			return false;
		}

		bool mouseMove( const ci::app::MouseEvent &event, const ivec2 &delta ) override {
			_mouseScreen = event.getPos();
			_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);
			return false;
		}

		bool mouseDrag( const ci::app::MouseEvent &event, const ivec2 &delta ) override {
			_mouseScreen = event.getPos();
			_mouseWorld = getLevel()->getViewport()->screenToWorld(_mouseScreen);
			if (_cutting) {
				_cutEnd = _mouseWorld;
			}
			return false;
		}

	private:

		bool _cutting;
		float _radius;
		cpShapeFilter _cutFilter;
		vec2 _mouseScreen, _mouseWorld, _cutStart, _cutEnd;
		terrain::TerrainObjectRef _terrain;

	};



}

/*
	terrain::TerrainObjectRef _terrain;
*/

GameLevelTestScenario::GameLevelTestScenario()
{}

GameLevelTestScenario::~GameLevelTestScenario() {

}

void GameLevelTestScenario::setup() {
	GameLevelRef level = make_shared<GameLevel>();
	setLevel(level);

	level->load(app::loadAsset("levels/test0.xml"));
	_terrain = level->getTerrain();


	auto dragger = GameObject::with("Dragger", { make_shared<MousePickComponent>(Filters::PICK) });
	auto cutter = GameObject::with("Cutter", { make_shared<MouseCutterComponent>(_terrain, Filters::CUTTER, 4)});
	auto grid = GameObject::with("Grid", { make_shared<WorldCoordinateSystemDrawComponent>() });

	getLevel()->addGameObject(dragger);
	getLevel()->addGameObject(cutter);
	getLevel()->addGameObject(grid);

}

void GameLevelTestScenario::cleanup() {
	_terrain = nullptr;
	setLevel(nullptr);
}

void GameLevelTestScenario::resize( ivec2 size ) {
}

void GameLevelTestScenario::step( const time_state &time ) {
}

void GameLevelTestScenario::update( const time_state &time ) {
}

void GameLevelTestScenario::clear( const render_state &state ) {
	gl::clear( Color( 0.2, 0.2, 0.2 ) );
}

void GameLevelTestScenario::draw( const render_state &state ) {
	//
	// NOTE: we're in screen space, with coordinate system origin at top left
	//

	float fps = core::GameApp::get()->getAverageFps();
	float sps = core::GameApp::get()->getAverageSps();
	string info = core::strings::format("%.1f %.1f", fps, sps);
	gl::drawString(info, vec2(10,10), Color(1,1,1));

}

bool GameLevelTestScenario::mouseDown( const ci::app::MouseEvent &event ) {

	if ( event.isAltDown() ){
		dvec2 mouseWorld = getLevel()->getViewport()->screenToWorld(event.getPos());
		getViewportController()->lookAt( mouseWorld );
		return true;
	}

	if ( isKeyDown( app::KeyEvent::KEY_SPACE )) {
		return false;
	}

	return false;
}

bool GameLevelTestScenario::mouseUp( const ci::app::MouseEvent &event ) {
	return false;
}

bool GameLevelTestScenario::mouseWheel( const ci::app::MouseEvent &event ) {
	float zoom = getViewportController()->getScale(),
	wheelScale = 0.1 * zoom,
	dz = (event.getWheelIncrement() * wheelScale);

	getViewportController()->setScale( std::max( zoom + dz, 0.1f ), event.getPos() );

	return true;
}

bool GameLevelTestScenario::mouseMove( const ci::app::MouseEvent &event, const ivec2 &delta ) {
	return false;
}

bool GameLevelTestScenario::mouseDrag( const ci::app::MouseEvent &event, const ivec2 &delta ) {
	if ( isKeyDown( app::KeyEvent::KEY_SPACE )){
		getViewportController()->setPan( getViewportController()->getPan() + dvec2(delta) );
		return true;
	}

	return false;
}

bool GameLevelTestScenario::keyDown( const ci::app::KeyEvent &event ) {
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

bool GameLevelTestScenario::keyUp( const ci::app::KeyEvent &event ) {
	return false;
}

void GameLevelTestScenario::reset() {
	cleanup();
	setup();
}
