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

namespace Filters {
	cpShapeFilter TERRAIN = cpShapeFilterNew(CP_NO_GROUP, Categories::TERRAIN, Categories::TERRAIN | Categories::CUTTER | Categories::PICK | Categories::ANCHOR);
	cpShapeFilter ANCHOR = cpShapeFilterNew(CP_NO_GROUP, Categories::ANCHOR, Categories::TERRAIN);
	cpShapeFilter CUTTER = cpShapeFilterNew(CP_NO_GROUP, Categories::CUTTER, Categories::TERRAIN);
	cpShapeFilter PICK = cpShapeFilterNew(CP_NO_GROUP, Categories::PICK, Categories::TERRAIN);
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

	XmlTree spaceNode = core::util::xml::findElement(levelNode, "space").first;
	applySpaceAttributes(spaceNode);


	XmlTree gravityNode = core::util::xml::findElement(levelNode, "gravity").first;
	applyGravityAttributes(gravityNode);

	//
	//	Load terrain
	//

	XmlTree terrainNode = core::util::xml::findElement(levelNode, "terrain").first;
	string terrainSvgPath = terrainNode.getAttribute("path").getValue();
	loadTerrain(terrainNode, app::loadAsset(terrainSvgPath));
}

void GameLevel::addGameObject(core::GameObjectRef obj) {
	Level::addGameObject(obj);
}

void GameLevel::applySpaceAttributes(XmlTree spaceNode) {
	if (spaceNode.hasAttribute("damping")) {
		double damping = strtod(spaceNode.getAttribute("damping").getValue().c_str(), nullptr);
		damping = clamp(damping, 0.0, 1.0);
		cpSpaceSetDamping(getSpace()->getSpace(), damping);
	}
}

void GameLevel::applyGravityAttributes(XmlTree gravityNode) {
	string type = gravityNode.getAttribute("type").getValue();
	if (type == "radial") {
		radial_gravity_info rgi = getRadialGravity();
		rgi.strength = strtod(gravityNode.getAttribute("strength").getValue().c_str(), nullptr);
		rgi.faloffPower = strtod(gravityNode.getAttribute("falloff_power").getValue().c_str(), nullptr);
		setRadialGravity(rgi);
		setGravityType(RADIAL);

		CI_LOG_D("gravity RADIAL strength: " << rgi.strength << " faloffPower: " << rgi.faloffPower);

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

	double friction = 1;
	double density = 1;
	if (terrainNode.hasAttribute("friction")) {
		friction = strtod(terrainNode.getAttribute("friction").getValue().c_str(), nullptr);
	}

	if (terrainNode.hasAttribute("density")) {
		density = strtod(terrainNode.getAttribute("density").getValue().c_str(), nullptr);
	}

	const double minDensity = 1e-3;
	density = max(density, minDensity);

	const terrain::material terrainMaterial(density, friction, Filters::TERRAIN);
	const terrain::material anchorMaterial(1, friction, Filters::ANCHOR);
	
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
		CI_LOG_D("gravity RADIAL strength: " << rgi.strength << " faloffPower: " << rgi.faloffPower << " centerOfMass: " << rgi.centerOfMass);
	}

}
