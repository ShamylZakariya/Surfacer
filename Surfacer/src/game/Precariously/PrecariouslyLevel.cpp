//
//  GameLevel.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/13/17.
//
//

#include "PrecariouslyLevel.hpp"

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
			RadialGravitationCalculator(origin, -abs(magnitude), falloffPower),
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
		
		class MouseBomberComponent : public core::InputComponent {
		public:
			
			MouseBomberComponent(terrain::TerrainObjectRef terrain, dvec2 centerOfMass, int numSpokes, int numRings, double radius, double thickness, double variance, int dispatchReceiptIndex = 0):
			InputComponent(dispatchReceiptIndex),
			_terrain(terrain),
			_centerOfMass(centerOfMass),
			_numSpokes(numSpokes),
			_numRings(numRings),
			_radius(radius),
			_thickness(thickness),
			_variance(variance)
			{}
			
			bool mouseDown( const ci::app::MouseEvent &event ) override {
				if (event.isMetaDown()) {
					dvec2 mouseScreen = event.getPos();
					dvec2 mouseWorld = getLevel()->getViewport()->screenToWorld(mouseScreen);
					
					// create a radial crack and cut the world with it. note, to reduce tiny fragments
					// we set a fairly high min surface area for the cut.
					
					double minSurfaceAreaThreshold = 64;
					auto crack = make_shared<RadialCrackGeometry>(mouseWorld,_numSpokes, _numRings, _radius, _thickness, _variance);
					_terrain->getWorld()->cut(crack->getPolygon(), crack->getBB(), minSurfaceAreaThreshold);
					
					// add an explosive force
					dvec2 explosiveOrigin = mouseWorld - 2*_radius * normalize(mouseWorld - _centerOfMass);
					getLevel()->addGravity(ExplosionForceCalculator::create(explosiveOrigin, 4000, 0.5, 0.5));
					
					return true;
				}
				
				return false;
			}
			
		private:
			
			int _numSpokes, _numRings;
			double _radius, _thickness, _variance;
			terrain::TerrainObjectRef _terrain;
			dvec2 _centerOfMass;
			
		};
		
		class KeyboardDelegateComponent : public core::InputComponent {
		public:
			
			typedef function<void(int keyCode)> KeyHandler;
			
		public:
			KeyboardDelegateComponent(int dispatchReceiptIndex, const initializer_list<int> keycodes, const KeyHandler &keyUpHandler, const KeyHandler &keyDownHandler = KeyHandler()):
			InputComponent(dispatchReceiptIndex),
			_upHandler(keyUpHandler),
			_downHandler(keyDownHandler)
			{
				monitorKeys(keycodes);
			}
			
			void monitoredKeyDown( int keyCode ) override {
				if (_downHandler) {
					_downHandler(keyCode);
				}
			}
			
			void monitoredKeyUp( int keyCode ) override {
				if (_upHandler) {
					_upHandler(keyCode);
				}
			}
			
		private:
			function<void(int keyCode)> _upHandler, _downHandler;
		};
		
		
	}
	
	
	/*
	 BackgroundRef _background;
	 PlanetRef _planet;
	 core::RadialGravitationCalculatorRef _gravity;
	 */
	
	PrecariouslyLevel::PrecariouslyLevel():
	Level("Unnamed") {}
	
	PrecariouslyLevel::~PrecariouslyLevel()
	{}
	
	void PrecariouslyLevel::addObject(ObjectRef obj) {
		Level::addObject(obj);
	}
	
	void PrecariouslyLevel::removeObject(ObjectRef obj) {
		Level::removeObject(obj);
	}
	
	void PrecariouslyLevel::load(ci::DataSourceRef levelXmlData) {
		
		auto root = XmlTree(levelXmlData);
		auto prefabsNode = root.getChild("prefabs");
		auto levelNode = root.getChild("level");
		
		setName(levelNode.getAttribute("name").getValue());
		
		//
		//	Load some basic level properties
		//
		
		auto spaceNode = util::xml::findElement(levelNode, "space");
		CI_ASSERT_MSG(spaceNode, "Expect a <space> node in <level> definition");
		applySpaceAttributes(*spaceNode);
		
		//
		//	Apply gravity
		//
		
		for (const auto &childNode : levelNode.getChildren()) {
			if (childNode->getTag() == "gravity") {
				buildGravity(*childNode);
			}
		}
		
		//
		//	Load background
		//
		
		auto backgroundNode = util::xml::findElement(levelNode, "background");
		CI_ASSERT_MSG(backgroundNode, "Expect <background> node in level XML");
		loadBackground(backgroundNode.value());
		
		//
		//	Load Planet
		//
		
		auto planetNode = util::xml::findElement(levelNode, "planet");
		CI_ASSERT_MSG(planetNode, "Expect <planet> node in level XML");
		loadPlanet(planetNode.value());
		
		
		if (true) {
			
			addObject(Object::with("Crack", { make_shared<MouseBomberComponent>(_planet, _planet->getOrigin(), 7, 4, 75, 2, 100) }));
			
			addObject(Object::with("Keyboard", make_shared<KeyboardDelegateComponent>(0, initializer_list<int>{ app::KeyEvent::KEY_c },
				[&](int keyCode){
					switch(keyCode) {
						case app::KeyEvent::KEY_c:
							cullRubble();
							break;
				  }
			})));
			
		}
		
	}
	
	void PrecariouslyLevel::onReady() {
		Level::onReady();
	}
	
	bool PrecariouslyLevel::onCollisionBegin(cpArbiter *arb) {
		return true;
	}
	
	bool PrecariouslyLevel::onCollisionPreSolve(cpArbiter *arb) {
		return true;
	}
	
	void PrecariouslyLevel::onCollisionPostSolve(cpArbiter *arb) {
	}
	
	void PrecariouslyLevel::onCollisionSeparate(cpArbiter *arb) {
	}
	
	void PrecariouslyLevel::applySpaceAttributes(XmlTree spaceNode) {
		if (spaceNode.hasAttribute("damping")) {
			double damping = util::xml::readNumericAttribute<double>(spaceNode, "damping", 0.95);
			damping = clamp(damping, 0.0, 1.0);
			cpSpaceSetDamping(getSpace()->getSpace(), damping);
		}
	}
	
	void PrecariouslyLevel::buildGravity(XmlTree gravityNode) {
		string type = gravityNode.getAttribute("type").getValue();
		if (type == "radial") {
			dvec2 origin = util::xml::readPointAttribute(gravityNode, "origin", dvec2(0,0));
			double magnitude = util::xml::readNumericAttribute<double>(gravityNode, "strength", 10);
			double falloffPower = util::xml::readNumericAttribute<double>(gravityNode, "falloff_power", 0);
			auto gravity = RadialGravitationCalculator::create(origin, magnitude, falloffPower);
			addGravity(gravity);
			
			if (gravityNode.getAttribute("primary") == "true") {
				_gravity = gravity;
			}
			
		} else if (type == "directional") {
			dvec2 dir = util::xml::readPointAttribute(gravityNode, "dir", dvec2(0,0));
			double magnitude = util::xml::readNumericAttribute<double>(gravityNode, "strength", 10);
			addGravity(DirectionalGravitationCalculator::create(dir, magnitude));
		}
	}
	
	void PrecariouslyLevel::loadBackground(XmlTree backgroundNode) {
		_background = Background::create(backgroundNode);
		addObject(_background);
	}
	
	void PrecariouslyLevel::loadPlanet(XmlTree planetNode) {
		double friction = util::xml::readNumericAttribute<double>(planetNode, "friction", 1);
		double density = util::xml::readNumericAttribute<double>(planetNode, "density", 1);
		double collisionShapeRadius = 0.1;
		
		const double minDensity = 1e-3;
		density = max(density, minDensity);
		
		const double minSurfaceArea = 2;
		
		const terrain::material terrainMaterial(density, friction, collisionShapeRadius, ShapeFilters::TERRAIN, CollisionType::TERRAIN, minSurfaceArea);
		const terrain::material anchorMaterial(1, friction, collisionShapeRadius, ShapeFilters::ANCHOR, CollisionType::ANCHOR, minSurfaceArea);
		auto world = make_shared<terrain::World>(getSpace() ,terrainMaterial, anchorMaterial);
		
		_planet = Planet::create("Planet", world, planetNode, DrawLayers::PLANET);
		addObject(_planet);
		
		//
		//	Look for a center of mass for gravity
		//
		
		if (_gravity) {
			_gravity->setCenterOfMass(_planet->getOrigin());
		}
	}
	
	void PrecariouslyLevel::cullRubble() {
		size_t count = _planet->getWorld()->cullDynamicGroups(128, 0.75);
		CI_LOG_D("Culled " << count << " bits of rubble");
	}
	
} // namespace surfacer
