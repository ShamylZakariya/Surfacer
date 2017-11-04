//
//  ParticleSystemTestScenario.cpp
//  Tests
//
//  Created by Shamyl Zakariya on 11/2/17.
//

#include "ParticleSystemTestScenario.hpp"

#include "App.hpp"
#include "Strings.hpp"

#include "DevComponents.hpp"



ParticleSystemTestScenario::ParticleSystemTestScenario() {
}

ParticleSystemTestScenario::~ParticleSystemTestScenario() {
	
}

void ParticleSystemTestScenario::setup() {
	setLevel(make_shared<Level>("Hello Svg"));
	
	auto cameraController = Object::with("ViewportControlComponent", {
		make_shared<ManualViewportControlComponent>(getViewportController())
	});
	
	auto grid = Object::with("Grid", { WorldCartesianGridDrawComponent::create(1) });
	
	getLevel()->addObject(grid);
	getLevel()->addObject(cameraController);
	
	//
	//	Build a thing
	//
	
	auto doc = util::svg::Group::loadSvgDocument(app::loadAsset("svg_tests/eggsac.svg"));
	getLevel()->addObject(Object::with("Eggsac", { make_shared<util::svg::SvgDrawComponent>(doc)}));

	//
	//	Build basic particle system
	//
	
	buildPs0();
	
	auto presser = MouseDelegateComponent::forPress(0, [this](dvec2 screen, dvec2 world, const ci::app::MouseEvent &event){
		testPs0SingleEmission(world, dvec2(0,0));
		return true;
	});

	auto dragger = MouseDelegateComponent::forDrag(0, [](dvec2 screen, dvec2 world, dvec2 deltaScreen, dvec2 deltaWorld, const ci::app::MouseEvent &event){
		// nothing yet
		return true;
	});

	
	getLevel()->addObject(Object::with("Mouse Handling", { presser, dragger }));
}

void ParticleSystemTestScenario::cleanup() {
	setLevel(nullptr);
}

void ParticleSystemTestScenario::resize( ivec2 size ) {
	
}

void ParticleSystemTestScenario::step( const time_state &time ) {
	
}

void ParticleSystemTestScenario::update( const time_state &time ) {
}

void ParticleSystemTestScenario::clear( const render_state &state ) {
	gl::clear( Color( 0.2, 0.2, 0.2 ) );
}

void ParticleSystemTestScenario::drawScreen( const render_state &state ) {
	
	//
	// NOTE: we're in screen space, with coordinate system origin at top left
	//
	
	float fps = App::get()->getAverageFps();
	float sps = App::get()->getAverageSps();
	string info = core::strings::format("%.1f %.1f", fps, sps);
	gl::drawString(info, vec2(10,10), Color(1,1,1));
	
	stringstream ss;
	Viewport::look look = getViewport()->getLook();
	double scale = getViewport()->getScale();
	
	ss << std::setprecision(3) << "world (" << look.world.x << ", " << look.world.y  << ") scale: " << scale << " up: (" << look.up.x << ", " << look.up.y << ")";
	gl::drawString(ss.str(), vec2(10,40), Color(1,1,1));
	
}

bool ParticleSystemTestScenario::keyDown( const ci::app::KeyEvent &event ) {
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

void ParticleSystemTestScenario::reset() {
	cleanup();
	setup();
}

#pragma mark - Tests

void ParticleSystemTestScenario::buildPs0() {
	auto image = loadImage(app::loadAsset("precariously/textures/Explosion.png"));
	gl::Texture2d::Format fmt = gl::Texture2d::Format().mipmap(false);
	
	UniversalParticleSystem::config config;
	config.drawConfig.drawLayer = 1000;
	config.drawConfig.textureAtlas = gl::Texture2d::create(image, fmt);
	config.drawConfig.atlasType = ParticleAtlasType::TwoByTwo;
	
	_ps0 = UniversalParticleSystem::create("Ps0", config);
	
	auto level = getLevel();
	level->addObject(_ps0);
	level->addGravity(DirectionalGravitationCalculator::create(dvec2(0,-1), 9.8));

	// build a "smoke" particle template
	_smoke.atlasIdx = 0;
	_smoke.lifespan = 3;
	_smoke.radius = { 0.0, 10.0, 0.0 };
	_smoke.damping = { 0.0, 0.1 };
	_smoke.additivity = { 0.0 };
	_smoke.mass = { -1.0, 0.0 };
	_smoke.color = { ci::ColorA(1,1,0,1), ci::ColorA(0,1,1,1) };
	_smoke.position = dvec2(0,0);
	_smoke.velocity = dvec2(0,10);
}

void ParticleSystemTestScenario::testPs0SingleEmission(dvec2 world, dvec2 vel) {
	UniversalParticleSimulationRef sim = _ps0->getComponent<UniversalParticleSimulation>();
	_smoke.position = world;
	_smoke.velocity = vel;
	sim->emit(_smoke);
}
