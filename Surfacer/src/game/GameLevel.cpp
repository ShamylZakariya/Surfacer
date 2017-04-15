//
//  GameLevel.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/13/17.
//
//

#include "GameLevel.hpp"

using namespace core;

namespace {

	pair<XmlTree,bool> findElement(const XmlTree &node, string tag) {
		if (node.isElement() && node.getTag() == tag) {
			return make_pair(node,true);
		} else {
			for ( auto childNode : node ) {
				auto result = findElement(childNode, tag);
				if (result.second) {
					return result;
				}
			}
		}
		return make_pair(XmlTree(), false);
	}

	pair<XmlTree,bool> findNodeWithId(const XmlTree &node, string id ) {
		if (node.isElement() && node.hasAttribute("id") && node.getAttribute("id").getValue() == id) {
			return make_pair(node,true);
		} else {
			for ( auto childNode : node ) {
				auto result = findNodeWithId(childNode, id);
				if (result.second) {
					return result;
				}
			}
		}
		return make_pair(XmlTree(),false);
	}

}

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
Level("Unnamed")
{}

GameLevel::~GameLevel()
{}

void GameLevel::load(ci::DataSourceRef levelXmlData) {
	XmlTree levelNode = XmlTree(levelXmlData).getChild("level");
	setName(levelNode.getAttribute("name").getValue());

	//
	//	Load terrain
	//

	XmlTree terrainNode = findElement(levelNode, "terrain").first;
	string terrainSvgPath = terrainNode.getAttribute("path").getValue();
	loadTerrain(app::loadAsset(terrainSvgPath));
}

void GameLevel::addGameObject(core::GameObjectRef obj) {
	Level::addGameObject(obj);
}

void GameLevel::loadTerrain(ci::DataSourceRef svgData) {

	const terrain::material terrainMaterial(1, 0.5, Filters::TERRAIN);
	const terrain::material anchorMaterial(1, 1, Filters::ANCHOR);
	
	//
	//	Load terrain
	//

	// load shapes and anchors
	vector<terrain::ShapeRef> shapes;
	vector<terrain::AnchorRef> anchors;
	terrain::World::loadSvg(svgData, dmat4(), shapes, anchors, true);
	if (!shapes.empty()) {
		// partition
		auto partitionedShapes = terrain::World::partition(shapes, dvec2(0,0), 500);

		// construct
		auto world = make_shared<terrain::World>(getSpace(),terrainMaterial, anchorMaterial);
		world->build(partitionedShapes, anchors);

		_terrain = terrain::TerrainObject::create("Terrain", world);
		addGameObject(_terrain);
	}

	//
	//	Read the elements part of the svg file
	//

	XmlTree svgNode = XmlTree( svgData ).getChild( "svg" );
	auto elements = findNodeWithId(svgNode, "elements");
	if (elements.second) {
		for ( auto element : elements.first ) {
			if (element.getTag() == "circle") {
				svg_element e;
				e.position.x = strtod(element.getAttribute("cx").getValue().c_str(), nullptr);
				e.position.y = strtod(element.getAttribute("cy").getValue().c_str(), nullptr);
				e.id = element.getAttribute("id").getValue();
				_svgElements[e.id] = e;
			}
		}
	}


	//
	//	Look for a center_of_mass to build gravity
	//

	auto centerOfMassPos = _svgElements.find("center_of_mass");
	if (centerOfMassPos != _svgElements.end()) {
		svg_element centerOfMass = centerOfMassPos->second;
		CI_LOG_D("centerOfMass: " << centerOfMass.position);
		// TODO: Do something with the center of mass
	}
}
