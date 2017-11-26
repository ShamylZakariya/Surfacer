//
//  GameStage.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/13/17.
//
//

#include "PrecariouslyStage.hpp"

#include "PrecariouslyConstants.hpp"
#include "DevComponents.hpp"
#include "Xml.hpp"

using namespace core;

namespace precariously {
	
	namespace {
		
		SMART_PTR(ExplosionForceCalculator);
		class ExplosionForceCalculator : public RadialGravitationCalculator {
		public:
			static ExplosionForceCalculatorRef create(dvec2 origin, double magnitude, double falloffPower, seconds_t lifespanSeconds = 2) {
				return make_shared<ExplosionForceCalculator>(origin, magnitude, falloffPower, lifespanSeconds);
			}
			
		public:
			ExplosionForceCalculator(dvec2 origin, double magnitude, double falloffPower, seconds_t lifespanSeconds):
			RadialGravitationCalculator(GravitationLayers::EXPLOSION, origin, -abs(magnitude), falloffPower),
			_startSeconds(time_state::now()),
			_lifespanSeconds(lifespanSeconds),
			_magnitudeScale(0)
			{}
			
			void update(const time_state &time) override {
				seconds_t age = time.time - _startSeconds;
				double life = age / _lifespanSeconds;
				if (life <= 1) {
					_magnitudeScale = sin(life);
				} else {
					_magnitudeScale = 0;
					setFinished(true);
				}
			}
			
		private:
			
			seconds_t _startSeconds, _lifespanSeconds;
			double _magnitudeScale;
			
		};
	}
	
	
	/*
	 BackgroundRef _background;
	 PlanetRef _planet;
	 CloudLayerParticleSimulation _cloudLayerSimulation;
	 core::RadialGravitationCalculatorRef _gravity;
	 */
	
	PrecariouslyStage::PrecariouslyStage():
	Stage("Unnamed") {}
	
	PrecariouslyStage::~PrecariouslyStage()
	{}
	
	void PrecariouslyStage::addObject(ObjectRef obj) {
		Stage::addObject(obj);
	}
	
	void PrecariouslyStage::removeObject(ObjectRef obj) {
		Stage::removeObject(obj);
	}
	
	void PrecariouslyStage::load(ci::DataSourceRef stageXmlData) {
		
		auto root = XmlTree(stageXmlData);
		auto prefabsNode = root.getChild("prefabs");
		auto stageNode = root.getChild("stage");
		
		setName(stageNode.getAttribute("name").getValue());
		
		//
		//	Load some basic stage properties
		//
		
		auto spaceNode = util::xml::findElement(stageNode, "space");
		CI_ASSERT_MSG(spaceNode, "Expect a <space> node in <stage> definition");
		applySpaceAttributes(*spaceNode);
		
		//
		//	Apply gravity
		//
		
		for (const auto &childNode : stageNode.getChildren()) {
			if (childNode->getTag() == "gravity") {
				buildGravity(*childNode);
			}
		}
		
		//
		//	Load background
		//
		
		auto backgroundNode = util::xml::findElement(stageNode, "background");
		CI_ASSERT_MSG(backgroundNode, "Expect <background> node in stage XML");
		loadBackground(*backgroundNode);
		
		//
		//	Load Planet
		//
		
		auto planetNode = util::xml::findElement(stageNode, "planet");
		CI_ASSERT_MSG(planetNode, "Expect <planet> node in stage XML");
		loadPlanet(*planetNode);
		
		//
		//	Load cloud layers
		//

		auto cloudLayerNode = util::xml::findElement(stageNode, "cloudLayer", "id", "background");
		if (cloudLayerNode) {
			_cloudLayers.push_back(loadCloudLayer(*cloudLayerNode, DrawLayers::PLANET - 1));
		}

		cloudLayerNode = util::xml::findElement(stageNode, "cloudLayer", "id", "foreground");
		if (cloudLayerNode) {
			_cloudLayers.push_back(loadCloudLayer(*cloudLayerNode, DrawLayers::EFFECTS));
		}

		buildExplosionParticleSystem();
		
		if (true) {
			
			addObject(Object::with("Mouse", MouseDelegateComponent::create(0)->onPress([this](dvec2 screen, dvec2 world, const ci::app::MouseEvent &event){
				if (event.isMetaDown()) {
					performExplosion(world);
					return true;
				}
				return false;
			})));
			
			auto keys = initializer_list<int>{ app::KeyEvent::KEY_c, app::KeyEvent::KEY_s };
			addObject(Object::with("Keyboard", KeyboardDelegateComponent::create(0, keys)->onPress([this](int keyCode){
				switch(keyCode) {
					case app::KeyEvent::KEY_c:
						cullRubble();
						break;
					case app::KeyEvent::KEY_s:
						makeSleepersStatic();
						break;
				}
			})));
				
		}
		
	}
	
	void PrecariouslyStage::onReady() {
		Stage::onReady();
	}
	
	void PrecariouslyStage::update( const core::time_state &time ) {
		Stage::update(time);
	}
	
	bool PrecariouslyStage::onCollisionBegin(cpArbiter *arb) {
		return true;
	}
	
	bool PrecariouslyStage::onCollisionPreSolve(cpArbiter *arb) {
		return true;
	}
	
	void PrecariouslyStage::onCollisionPostSolve(cpArbiter *arb) {
	}
	
	void PrecariouslyStage::onCollisionSeparate(cpArbiter *arb) {
	}
	
	void PrecariouslyStage::applySpaceAttributes(XmlTree spaceNode) {
		if (spaceNode.hasAttribute("damping")) {
			double damping = util::xml::readNumericAttribute<double>(spaceNode, "damping", 0.95);
			damping = clamp(damping, 0.0, 1.0);
			cpSpaceSetDamping(getSpace()->getSpace(), damping);
		}
	}
	
	void PrecariouslyStage::buildGravity(XmlTree gravityNode) {
		string type = gravityNode.getAttribute("type").getValue();
		if (type == "radial") {
			dvec2 origin = util::xml::readPointAttribute(gravityNode, "origin", dvec2(0,0));
			double magnitude = util::xml::readNumericAttribute<double>(gravityNode, "strength", 10);
			double falloffPower = util::xml::readNumericAttribute<double>(gravityNode, "falloff_power", 0);
			auto gravity = RadialGravitationCalculator::create(GravitationLayers::GLOBAL, origin, magnitude, falloffPower);
			addGravity(gravity);
			
			if (gravityNode.getAttribute("primary") == "true") {
				_gravity = gravity;
			}
			
		} else if (type == "directional") {
			dvec2 dir = util::xml::readPointAttribute(gravityNode, "dir", dvec2(0,0));
			double magnitude = util::xml::readNumericAttribute<double>(gravityNode, "strength", 10);
			addGravity(DirectionalGravitationCalculator::create(GravitationLayers::GLOBAL, dir, magnitude));
		}
	}
	
	void PrecariouslyStage::loadBackground(XmlTree backgroundNode) {
		_background = Background::create(backgroundNode);
		addObject(_background);
	}
	
	void PrecariouslyStage::loadPlanet(XmlTree planetNode) {
		double friction = util::xml::readNumericAttribute<double>(planetNode, "friction", 1);
		double density = util::xml::readNumericAttribute<double>(planetNode, "density", 1);
		double collisionShapeRadius = 0.1;
		
		const double minDensity = 1e-3;
		density = max(density, minDensity);
		
		const double minSurfaceArea = 2;
		const ci::Color terrainColor = util::xml::readColorAttribute(planetNode, "color", Color(1,0,1));
		const ci::Color coreColor = util::xml::readColorAttribute(planetNode, "coreColor", Color(0,1,1));
		
		
		const terrain::material terrainMaterial(density, friction, collisionShapeRadius, ShapeFilters::TERRAIN, CollisionType::TERRAIN, minSurfaceArea, terrainColor);
		const terrain::material anchorMaterial(1, friction, collisionShapeRadius, ShapeFilters::ANCHOR, CollisionType::ANCHOR, minSurfaceArea, coreColor);
		auto world = make_shared<terrain::World>(getSpace(), terrainMaterial, anchorMaterial);
		
		_planet = Planet::create("Planet", world, planetNode, DrawLayers::PLANET);
		addObject(_planet);
		
		//
		//	Look for a center of mass for gravity
		//
		
		if (_gravity) {
			_gravity->setCenterOfMass(_planet->getOrigin());
		}
	}
	
	CloudLayerParticleSystemRef PrecariouslyStage::loadCloudLayer(XmlTree cloudLayer, int drawLayer) {
		CloudLayerParticleSystem::config config = CloudLayerParticleSystem::config::parse(cloudLayer);
		config.drawConfig.drawLayer = drawLayer;
		auto cl = CloudLayerParticleSystem::create(config);
		addObject(cl);
		return cl;
	}
	
	void PrecariouslyStage::buildExplosionParticleSystem() {
		using namespace particles;

		auto image = loadImage(app::loadAsset("precariously/textures/Explosion.png"));
		gl::Texture2d::Format fmt = gl::Texture2d::Format().mipmap(false);
		
		ParticleSystem::config config;
		config.maxParticleCount = 500;
		config.keepSorted = true;
		config.drawConfig.drawLayer = DrawLayers::EFFECTS;
		config.drawConfig.textureAtlas = gl::Texture2d::create(image, fmt);
		config.drawConfig.atlasType = Atlas::TwoByTwo;
		config.kinematicParticleGravitationLayerMask = GravitationLayers::GLOBAL;
		
		auto ps = ParticleSystem::create("Explosion ParticleSystem", config);
		addObject(ps);
		
		// build a "smoke" particle template
		particle_prototype smoke;
		smoke.atlasIdx = 0;
		smoke.lifespan = 3;
		smoke.radius = { 0, 32, 16, 8, 0 };
		smoke.damping = { 0, 0, 0.1 };
		smoke.additivity = { 0.9, 0, 0 };
		smoke.mass = { 0 };
		smoke.initialVelocity = 60;
		smoke.gravitationLayerMask = GravitationLayers::GLOBAL;
		smoke.color = { ci::ColorA(1,0.3,0.1,1), ci::ColorA(1,0.7,0.2,1), ci::ColorA(1,1,1,1), ci::ColorA(0.3,0.3,0.3,1), ci::ColorA(0.3,0.3,0.3,1) };
		
		// build a "spark" particle template
		particle_prototype spark;
		spark.atlasIdx = 1;
		spark.lifespan = 6;
		spark.radius = { 0, 8, 8, 8, 8, 0 };
		spark.damping = { 0.0, 0.02 };
		spark.additivity = { 1.0 };
		spark.mass = 10;
		spark.orientToVelocity = true;
		spark.initialVelocity = 100;
		spark.minVelocity = 60;
		spark.color = { ci::ColorA(1,0.5,0.5,1), ci::ColorA(1,0.5,0.5,0) };
		smoke.gravitationLayerMask = GravitationLayers::GLOBAL;
		spark.kinematics = particle_prototype::kinematics_prototype(1, 1, 0.2, ShapeFilters::TERRAIN);
		
		_explosionEmitter = ps->createEmitter();
		_explosionEmitter->add(smoke, ParticleEmitter::Source(2, 1, 0.2), 10);
		_explosionEmitter->add(spark, ParticleEmitter::Source(1, 1, 0.15), 10);
	}
	
	void PrecariouslyStage::cullRubble() {
		size_t count = _planet->getWorld()->cullDynamicGroups(128, 0.75);
		CI_LOG_D("Culled " << count << " bits of rubble");
	}
	
	void PrecariouslyStage::makeSleepersStatic() {
		size_t count = _planet->getWorld()->makeSleepingDynamicGroupsStatic(5, 0.75);
		CI_LOG_D("Static'd " << count << " bits of sleeping rubble");
	}
	
	void PrecariouslyStage::performExplosion(dvec2 world) {
		// create a radial crack and cut the world with it. note, to reduce tiny fragments
		// we set a fairly high min surface area for the cut.
		
		double minSurfaceAreaThreshold = 64;
		int numSpokes = 7;
		int numRings = 3;
		double radius = 75;
		double thickness = 2;
		double variance = 100;
		
		auto crack = make_shared<RadialCrackGeometry>(world, numSpokes, numRings, radius, thickness, variance);
		_planet->getWorld()->cut(crack->getPolygon(), crack->getBB(), minSurfaceAreaThreshold);
		
		// get the closest point on terrain surface and use that to place explosive charge
		if (auto r = queryNearest(world, ShapeFilters::TERRAIN_PROBE)) {

			dvec2 origin = world + radius * normalize(_planet->getOrigin() - r.point);
			auto gravity = ExplosionForceCalculator::create(origin, 4000, 0.5, 0.5);
			addGravity(gravity);

			for (auto &cls : _cloudLayers) {
				cls->getSimulation<CloudLayerParticleSimulation>()->addGravityDisplacement(gravity);
			}
		}
		
		dvec2 emissionDir = normalize(world - _planet->getOrigin());
		_explosionEmitter->emit(world, emissionDir, 0.5, 140, particles::ParticleEmitter::Sawtooth);
	}
	
} // namespace surfacer
