//
//  GameLevel.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/13/17.
//
//

#include "GameLevel.hpp"
#include "Xml.hpp"

using namespace core;

namespace CollisionFilters {
	cpShapeFilter TERRAIN = cpShapeFilterNew(CP_NO_GROUP, _TERRAIN, _TERRAIN | _CUTTER | _PICK | _ANCHOR | _PLAYER);
	cpShapeFilter ANCHOR = cpShapeFilterNew(CP_NO_GROUP, _ANCHOR, _TERRAIN | _PLAYER);
	cpShapeFilter CUTTER = cpShapeFilterNew(CP_NO_GROUP, _CUTTER, _TERRAIN);
	cpShapeFilter PICK = cpShapeFilterNew(CP_NO_GROUP, _PICK, _TERRAIN);
	cpShapeFilter PLAYER = cpShapeFilterNew(CP_NO_GROUP, _PLAYER, _TERRAIN | _ANCHOR);
}


/*
	terrain::TerrainObjectRef _terrain;
	map<string, svg_element> _svgElements;
*/

GameLevel::GameLevel():
Level("Unnamed") {}

GameLevel::~GameLevel()
{}

void GameLevel::load(ci::DataSourceRef levelXmlData) {
	XmlTree levelNode = XmlTree(levelXmlData).getChild("level");
	setName(levelNode.getAttribute("name").getValue());

	//
	//	Load some basic level properties
	//

	XmlTree spaceNode = core::util::xml::findElement(levelNode, "space").second;
	applySpaceAttributes(spaceNode);

	XmlTree gravityNode = core::util::xml::findElement(levelNode, "gravity").second;
	applyGravityAttributes(gravityNode);

	//
	//	Load terrain
	//

	XmlTree terrainNode = core::util::xml::findElement(levelNode, "terrain").second;
	string terrainSvgPath = terrainNode.getAttribute("path").getValue();
	loadTerrain(terrainNode, app::loadAsset(terrainSvgPath));

	//
	//	Load the player
	//

	auto maybePlayerNode = core::util::xml::findElement(levelNode, "player");
	if (maybePlayerNode.first) {
		terrain::ElementRef playerElement = _terrain->getWorld()->getElementById("player");

		ci::DataSourceRef playerXmlData = app::loadAsset(maybePlayerNode.second.getAttribute("path").getValue());
		_player = player::Player::create("Player", playerXmlData, playerElement->getPosition());
		addGameObject(_player);
	}
}

void GameLevel::addGameObject(core::GameObjectRef obj) {
	Level::addGameObject(obj);
}

void GameLevel::applySpaceAttributes(XmlTree spaceNode) {
	if (spaceNode.hasAttribute("damping")) {
		double damping = util::xml::readNumericAttribute(spaceNode, "damping", 0.95);
		damping = clamp(damping, 0.0, 1.0);
		cpSpaceSetDamping(getSpace()->getSpace(), damping);
	}
}

void GameLevel::applyGravityAttributes(XmlTree gravityNode) {
	string type = gravityNode.getAttribute("type").getValue();
	if (type == "radial") {
		radial_gravity_info rgi = getRadialGravity();
		rgi.strength = util::xml::readNumericAttribute(gravityNode, "strength", 10);
		rgi.falloffPower = util::xml::readNumericAttribute(gravityNode, "falloff_power", 0);
		setRadialGravity(rgi);
		setGravityType(RADIAL);

		CI_LOG_D("gravity RADIAL strength: " << rgi.strength << " falloffPower: " << rgi.falloffPower);

	} else if (type == "directional") {
		string dirStr = gravityNode.getAttribute("dir").getValue();
		vector<double> vals = core::util::xml::readNumericSequence(dirStr);
		dvec2 dir(vals[0], vals[1]);
		setDirectionalGravityDirection(dir);
		setGravityType(DIRECTIONAL);

		CI_LOG_D("gravity DIRECTIONAL dir: " << dir );
	}
}

void GameLevel::loadTerrain(XmlTree terrainNode, ci::DataSourceRef svgData) {

	double friction = util::xml::readNumericAttribute(terrainNode, "friction", 1);
	double density = util::xml::readNumericAttribute(terrainNode, "density", 1);

	const double minDensity = 1e-3;
	density = max(density, minDensity);

	const terrain::material terrainMaterial(density, friction, CollisionFilters::TERRAIN, CollisionType::TERRAIN);
	const terrain::material anchorMaterial(1, friction, CollisionFilters::ANCHOR, CollisionType::TERRAIN);
	
	//
	//	Load terrain
	//

	// load shapes and anchors
	vector<terrain::ShapeRef> shapes;
	vector<terrain::AnchorRef> anchors;
	vector<terrain::ElementRef> elements;
	terrain::World::loadSvg(svgData, dmat4(), shapes, anchors, elements, true);
	if (!shapes.empty()) {
		// partition
		auto partitionedShapes = terrain::World::partition(shapes, dvec2(0,0), 500);

		// construct
		auto world = make_shared<terrain::World>(getSpace(),terrainMaterial, anchorMaterial);
		world->build(partitionedShapes, anchors, elements);

		_terrain = terrain::TerrainObject::create("Terrain", world);
		addGameObject(_terrain);
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
