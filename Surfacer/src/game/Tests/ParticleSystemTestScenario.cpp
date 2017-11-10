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


namespace {
	
	const double CollisionShapeRadius = 0;
	const double MinSurfaceArea = 2;
	const ci::Color TerrainColor(0.3,0.3,0.3);
	const ci::Color AnchorColor(0,0,0);
	
	namespace CollisionType {
		
		/*
		 The high 16 bits are a mask, the low are a type_id, the actual type is the logical OR of the two.
		 */
		
		namespace is {
			enum bits {
				SHOOTABLE   = 1 << 16,
				TOWABLE		= 1 << 17
			};
		}
		
		enum type_id {
			TERRAIN				= 1 | is::SHOOTABLE | is::TOWABLE,
			ANCHOR				= 2 | is::SHOOTABLE,
		};
	}
	
	namespace ShapeFilters {
		
		enum Categories {
			_TERRAIN = 1 << 0,
			_GRABBABLE = 1 << 1,
			_ANCHOR = 1 << 2,
		};
		
		cpShapeFilter TERRAIN = cpShapeFilterNew(CP_NO_GROUP, _TERRAIN,			_TERRAIN | _GRABBABLE | _ANCHOR);
		cpShapeFilter ANCHOR= cpShapeFilterNew(CP_NO_GROUP, _ANCHOR,			_TERRAIN);
		cpShapeFilter GRABBABLE= cpShapeFilterNew(CP_NO_GROUP, _GRABBABLE,		_TERRAIN);
	}

	namespace DrawLayers {
		enum layer {
			TERRAIN = 1,
		};
	}
	
	const terrain::material TerrainMaterial(1, 0.5, CollisionShapeRadius, ShapeFilters::TERRAIN, CollisionType::TERRAIN, MinSurfaceArea, TerrainColor);
	const terrain::material AnchorMaterial(1, 1, CollisionShapeRadius, ShapeFilters::ANCHOR, CollisionType::ANCHOR, MinSurfaceArea, AnchorColor);
	
	
	PolyLine2d rect(float left, float bottom, float right, float top) {
		PolyLine2d pl;
		pl.push_back(vec2(left,bottom));
		pl.push_back(vec2(right,bottom));
		pl.push_back(vec2(right,top));
		pl.push_back(vec2(left,top));
		pl.setClosed();
		return pl;
	}
}


ParticleSystemTestScenario::ParticleSystemTestScenario() {
}

ParticleSystemTestScenario::~ParticleSystemTestScenario() {
	
}

void ParticleSystemTestScenario::setup() {
	setStage(make_shared<Stage>("Particle System Tests"));
	
	getStage()->addObject(Object::with("ViewportControlComponent", {
		make_shared<ManualViewportControlComponent>(getViewportController())
	}));
	
	auto grid = WorldCartesianGridDrawComponent::create(1);
	grid->setFillColor(ColorA(209/255.0,219/255.0,169/255.0, 1.0));
	grid->setGridColor(ColorA(0.3, 0.3, 0.3, 1.0));
	getStage()->addObject(Object::with("Grid", { grid }));

	
	//
	//	Add an SVG for giggles
	//
	
	auto doc = util::svg::Group::loadSvgDocument(app::loadAsset("svg_tests/eggsac.svg"));
	doc->setPosition(dvec2(0,300));
	getStage()->addObject(Object::with("Eggsac", { make_shared<util::svg::SvgDrawComponent>(doc)}));
	
	//
	//	Build some terrain
	//
	
	vector<terrain::ShapeRef> shapes = {
		terrain::Shape::fromContour(rect(-100,  0, 100, 20)),
		terrain::Shape::fromContour(rect(-100, 20, -80, 100)),
		terrain::Shape::fromContour(rect(  80, 20, 100, 100)),
	};
	
	vector<terrain::AnchorRef> anchors = {
		terrain::Anchor::fromContour(rect(-5,5,5,15))
	};
	
	auto world = make_shared<terrain::World>(getStage()->getSpace(), TerrainMaterial, AnchorMaterial);
	world->build(shapes, anchors);

	auto terrain = terrain::TerrainObject::create("Terrain", world, DrawLayers::TERRAIN);
	getStage()->addObject(terrain);

	
	
	//
	//	Build basic particle system
	//
	
	buildExplosionPs();
	
	auto presser = MouseDelegateComponent::forPress(0, [this](dvec2 screen, dvec2 world, const ci::app::MouseEvent &event){
//		emitSmokeParticle(world, dvec2(0,0));
//		emitSparkParticle(world, dvec2(5,10));
		emitRubbleParticle(world, dvec2(25,50));
		return true;
	});

	auto dragger = MouseDelegateComponent::forDrag(0, [](dvec2 screen, dvec2 world, dvec2 deltaScreen, dvec2 deltaWorld, const ci::app::MouseEvent &event){
		// nothing yet
		return true;
	});

	
	getStage()->addObject(Object::with("Mouse Handling", { presser, dragger }));
}

void ParticleSystemTestScenario::cleanup() {
	setStage(nullptr);
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

void ParticleSystemTestScenario::buildExplosionPs() {
	auto image = loadImage(app::loadAsset("precariously/textures/Explosion.png"));
	gl::Texture2d::Format fmt = gl::Texture2d::Format().mipmap(false);
	
	UniversalParticleSystem::config config;
	config.drawConfig.drawLayer = 1000;
	config.drawConfig.textureAtlas = gl::Texture2d::create(image, fmt);
	config.drawConfig.atlasType = ParticleAtlasType::TwoByTwo;
	
	_explosionPs = UniversalParticleSystem::create("_explosionPs", config);
	
	auto stage = getStage();
	stage->addObject(_explosionPs);
	stage->addGravity(DirectionalGravitationCalculator::create(dvec2(0,-1), 9.8 * 10));

	// build a "smoke" particle template
	_smoke.atlasIdx = 0;
	_smoke.lifespan = 3;
	_smoke.radius = { 0.0, 10.0, 0.0 };
	_smoke.damping = { 0.0, 0.02 };
	_smoke.additivity = { 0.0 };
	_smoke.mass = { -1.0, 0.0 };
	_smoke.color = { ci::ColorA(1,1,1,0), ci::ColorA(1,1,1,1) };
	
	// build a "spark" particle template
	_spark.atlasIdx = 1;
	_spark.lifespan = 2;
	_spark.radius = { 0.0, 2.0, 0.0 };
	_spark.damping = { 0.0, 0.02 };
	_spark.additivity = { 1.0 };
	_spark.mass = { -1.0, +10.0 };
	_spark.orientToVelocity = true;
	_spark.color = { ci::ColorA(1,0.5,0.5,1), ci::ColorA(1,0.5,0.5,0) };
	
	// build a "rubble" particle template
	_rubble.atlasIdx = 2;
	_rubble.lifespan = 10;
	_rubble.radius = { 0.1, 2.0, 2.0, 2.0, 0.1 };
	_rubble.damping = 0.02;
	_rubble.additivity = 0.0;
	_rubble.mass = 10.0;
	_rubble.color = TerrainColor;
	_rubble.kinematics = particles::UniversalParticleSimulation::particle_kinematics_template(1, ShapeFilters::TERRAIN);
}

void ParticleSystemTestScenario::emitSmokeParticle(dvec2 world, dvec2 vel) {
	UniversalParticleSimulationRef sim = _explosionPs->getComponent<UniversalParticleSimulation>();
	_smoke.position = world;
	_smoke.velocity = vel;
	sim->emit(_smoke);
}

void ParticleSystemTestScenario::emitSparkParticle(dvec2 world, dvec2 vel) {
	UniversalParticleSimulationRef sim = _explosionPs->getComponent<UniversalParticleSimulation>();
	_spark.position = world;
	_spark.velocity = vel;
	sim->emit(_spark);
}

void ParticleSystemTestScenario::emitRubbleParticle(dvec2 world, dvec2 vel) {
	UniversalParticleSimulationRef sim = _explosionPs->getComponent<UniversalParticleSimulation>();
	_rubble.position = world;
	_rubble.velocity = vel;
	sim->emit(_rubble);
}
