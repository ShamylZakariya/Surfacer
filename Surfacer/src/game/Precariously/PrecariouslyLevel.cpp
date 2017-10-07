//
//  GameLevel.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/13/17.
//
//

#include "PrecariouslyLevel.hpp"

#include "DevComponents.hpp"
#include "Xml.hpp"

using namespace core;

namespace precariously {

	namespace ShapeFilters {
		cpShapeFilter TERRAIN			= cpShapeFilterNew(CP_NO_GROUP, _TERRAIN,			_TERRAIN | _TERRAIN_PROBE | _GRABBABLE | _ANCHOR | _PLAYER | _ENEMY);
		cpShapeFilter TERRAIN_PROBE		= cpShapeFilterNew(CP_NO_GROUP, _TERRAIN_PROBE,		_TERRAIN);
		cpShapeFilter ANCHOR			= cpShapeFilterNew(CP_NO_GROUP, _ANCHOR,			_TERRAIN | _PLAYER | _ENEMY);
		cpShapeFilter GRABBABLE			= cpShapeFilterNew(CP_NO_GROUP, _GRABBABLE,			_TERRAIN | _ENEMY);
		cpShapeFilter PLAYER			= cpShapeFilterNew(CP_NO_GROUP, _PLAYER,			_TERRAIN | _ANCHOR | _ENEMY);
		cpShapeFilter ENEMY				= cpShapeFilterNew(CP_NO_GROUP, _ENEMY,				_TERRAIN | _ANCHOR | _PLAYER | _GRABBABLE);
	}


	/*
	 terrain::TerrainObjectRef _terrain;
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

		auto gravityNode = util::xml::findElement(levelNode, "gravity");
		CI_ASSERT_MSG(spaceNode, "Expect a <gravity> node in <level> definition");
		applyGravityAttributes(*gravityNode);

		//
		//	Load Planet
		//

		auto planetNode = util::xml::findElement(levelNode, "planet");
		CI_ASSERT_MSG(planetNode, "Expect <planet> node in level XML");
		loadPlanet(planetNode.value());
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

	void PrecariouslyLevel::applyGravityAttributes(XmlTree gravityNode) {
		string type = gravityNode.getAttribute("type").getValue();
		if (type == "radial") {
			radial_gravity_info rgi = getRadialGravity();			
			rgi.strength = util::xml::readNumericAttribute<double>(gravityNode, "strength", 10);
			rgi.falloffPower = util::xml::readNumericAttribute<double>(gravityNode, "falloff_power", 0);
			setRadialGravity(rgi);
			setGravityType(RADIAL);

			CI_LOG_D("gravity RADIAL strength: " << rgi.strength << " falloffPower: " << rgi.falloffPower);

		} else if (type == "directional") {
			dvec2 dir = util::xml::readPointAttribute(gravityNode, "dir", dvec2(0,0));
			setDirectionalGravityDirection(dir);
			setGravityType(DIRECTIONAL);

			CI_LOG_D("gravity DIRECTIONAL dir: " << dir );
		}
	}
	
	void PrecariouslyLevel::loadPlanet(XmlTree planetNode) {
		double friction = util::xml::readNumericAttribute<double>(planetNode, "friction", 1);
		double density = util::xml::readNumericAttribute<double>(planetNode, "density", 1);
		
		const double minDensity = 1e-3;
		density = max(density, minDensity);
		
		const terrain::material terrainMaterial(density, friction, ShapeFilters::TERRAIN, CollisionType::TERRAIN);
		const terrain::material anchorMaterial(1, friction, ShapeFilters::ANCHOR, CollisionType::ANCHOR);
		auto world = make_shared<terrain::World>(getSpace(),terrainMaterial, anchorMaterial);
		
		_planet = Planet::create("Planet", world, planetNode, DrawLayers::PLANET);
		addObject(_planet);
		
		// set the physics center of mass to the planet's center
		radial_gravity_info rgi = getRadialGravity();
		rgi.centerOfMass = _planet->getOrigin();
		setRadialGravity(rgi);
	}
	
} // namespace surfacer
