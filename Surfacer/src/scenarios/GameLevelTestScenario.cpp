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

#include "DevComponents.hpp"

GameLevelTestScenario::GameLevelTestScenario()
{}

GameLevelTestScenario::~GameLevelTestScenario() {

}

void GameLevelTestScenario::setup() {
	GameLevelRef level = make_shared<GameLevel>();
	setLevel(level);

	level->load(app::loadAsset("levels/test0.xml"));
	auto terrain = level->getTerrain();

	auto dragger = GameObject::with("Dragger", {
		make_shared<MousePickComponent>(Filters::PICK),
		make_shared<MousePickDrawComponent>()
	});

	auto cutter = GameObject::with("Cutter", {
		make_shared<MouseCutterComponent>(terrain, Filters::CUTTER, 4),
		make_shared<MouseCutterDrawComponent>()
	});

	auto cameraController = GameObject::with("ViewportControlComponent", {
		make_shared<CameraControlComponent>(getViewportController())
	});

	auto grid = GameObject::with("Grid", { WorldCartesianGridDrawComponent::create() });

	getLevel()->addGameObject(dragger);
	getLevel()->addGameObject(cutter);
	getLevel()->addGameObject(grid);
	getLevel()->addGameObject(cameraController);

}

void GameLevelTestScenario::cleanup() {
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

void GameLevelTestScenario::drawScreen( const render_state &state ) {
	//
	// NOTE: we're in screen space, with coordinate system origin at top left
	//

	float fps = core::GameApp::get()->getAverageFps();
	float sps = core::GameApp::get()->getAverageSps();
	string info = core::strings::format("%.1f %.1f", fps, sps);
	gl::drawString(info, vec2(10,10), Color(1,1,1));

	stringstream ss;
	Viewport::look look = getViewport()->getLook();
	double scale = getViewport()->getScale();

	ss << std::setprecision(3) << "world (" << look.world.x << ", " << look.world.y  << ") scale: " << scale << " up: (" << look.up.x << ", " << look.up.y << ")";
	gl::drawString(ss.str(), vec2(10,40), Color(1,1,1));
}

bool GameLevelTestScenario::keyDown( const ci::app::KeyEvent &event ) {
	if (event.getChar() == 'r') {
		reset();
		return true;
	} else if (event.getCode() == ci::app::KeyEvent::KEY_BACKQUOTE) {
		setRenderMode( RenderMode::mode( (int(getRenderMode()) + 1) % RenderMode::COUNT ));
	}
	return false;
}


void GameLevelTestScenario::reset() {
	cleanup();
	setup();
}
