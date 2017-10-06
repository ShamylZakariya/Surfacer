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
		//	Load terrain
		//

		auto terrainNode = util::xml::findElement(levelNode, "terrain");
		CI_ASSERT_MSG(terrainNode, "Expect a <terrain> node in <level> definition");
		string terrainSvgPath = terrainNode->getAttribute("path").getValue();
		loadTerrain(*terrainNode, app::loadAsset(terrainSvgPath));
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
			double damping = util::xml::readNumericAttribute(spaceNode, "damping", 0.95);
			damping = clamp(damping, 0.0, 1.0);
			cpSpaceSetDamping(getSpace()->getSpace(), damping);
		}
	}

	void PrecariouslyLevel::applyGravityAttributes(XmlTree gravityNode) {
		string type = gravityNode.getAttribute("type").getValue();
		if (type == "radial") {
			radial_gravity_info rgi = getRadialGravity();
			rgi.strength = util::xml::readNumericAttribute(gravityNode, "strength", 10);
			rgi.falloffPower = util::xml::readNumericAttribute(gravityNode, "falloff_power", 0);
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

	void PrecariouslyLevel::loadTerrain(XmlTree terrainNode, ci::DataSourceRef svgData) {

		double friction = util::xml::readNumericAttribute(terrainNode, "friction", 1);
		double density = util::xml::readNumericAttribute(terrainNode, "density", 1);
		double scale = util::xml::readNumericAttribute(terrainNode, "scale", 1);

		const double minDensity = 1e-3;
		density = max(density, minDensity);

		const terrain::material terrainMaterial(density, friction, ShapeFilters::TERRAIN, CollisionType::TERRAIN);
		const terrain::material anchorMaterial(1, friction, ShapeFilters::ANCHOR, CollisionType::ANCHOR);

		//
		//	Load terrain
		//

		// load shapes and anchors
		vector<terrain::ShapeRef> shapes;
		vector<terrain::AnchorRef> anchors;
		vector<terrain::ElementRef> elements;
		terrain::World::loadSvg(svgData, dmat4(scale), shapes, anchors, elements, true);
		if (!shapes.empty()) {
			// partition
			auto partitionedShapes = terrain::World::partition(shapes, dvec2(0,0), 500);

			// construct
			auto world = make_shared<terrain::World>(getSpace(),terrainMaterial, anchorMaterial);
			world->build(partitionedShapes, anchors, elements);

			_terrain = terrain::TerrainObject::create("Terrain", world, DrawLayers::TERRAIN);
			addObject(_terrain);
		}

		//
		//	Look for a center_of_mass for gravity
		//

		if (terrain::ElementRef e = _terrain->getWorld()->getElementById("center_of_mass")) {
			radial_gravity_info rgi = getRadialGravity();
			rgi.centerOfMass = e->getModelMatrix() * e->getModelCentroid();
			setRadialGravity(rgi);
			CI_LOG_D("gravity RADIAL strength: " << rgi.strength << " falloffPower: " << rgi.falloffPower << " centerOfMass: " << rgi.centerOfMass);
		}
	}
	
} // namespace surfacer
