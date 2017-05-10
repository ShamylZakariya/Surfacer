//
//  GameLevel.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/13/17.
//
//

#include "GameLevel.hpp"

#include "DevComponents.hpp"
#include "Eggsac.hpp"
#include "Xml.hpp"


namespace core { namespace game {

	namespace CollisionFilters {
		cpShapeFilter TERRAIN = cpShapeFilterNew(CP_NO_GROUP, _TERRAIN, _TERRAIN | _TERRAIN_PROBE | _PICK | _ANCHOR | _PLAYER | _ENEMY);
		cpShapeFilter ANCHOR = cpShapeFilterNew(CP_NO_GROUP, _ANCHOR, _TERRAIN | _PLAYER | _ENEMY);
		cpShapeFilter TERRAIN_PROBE = cpShapeFilterNew(CP_NO_GROUP, _TERRAIN_PROBE, _TERRAIN);
		cpShapeFilter PICK = cpShapeFilterNew(CP_NO_GROUP, _PICK, _TERRAIN);
		cpShapeFilter PLAYER = cpShapeFilterNew(CP_NO_GROUP, _PLAYER, _TERRAIN | _ANCHOR | _ENEMY);
		cpShapeFilter ENEMY = cpShapeFilterNew(CP_NO_GROUP, _ENEMY, _TERRAIN | _ANCHOR | _PLAYER);
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

		XmlTree spaceNode = util::xml::findElement(levelNode, "space").second;
		applySpaceAttributes(spaceNode);

		XmlTree gravityNode = util::xml::findElement(levelNode, "gravity").second;
		applyGravityAttributes(gravityNode);

		//
		//	Load terrain
		//

		XmlTree terrainNode = util::xml::findElement(levelNode, "terrain").second;
		string terrainSvgPath = terrainNode.getAttribute("path").getValue();
		loadTerrain(terrainNode, app::loadAsset(terrainSvgPath));

		//
		//	Load the player
		//

		auto maybePlayerNode = util::xml::findElement(levelNode, "player");
		if (maybePlayerNode.first) {
			terrain::ElementRef playerElement = _terrain->getWorld()->getElementById("player");
			ci::DataSourceRef playerXmlData = app::loadAsset(maybePlayerNode.second.getAttribute("path").getValue());
			loadPlayer(maybePlayerNode.second, playerXmlData, playerElement);
		}

		//
		//	Load enemies
		//

		auto maybeEnemiesNode = util::xml::findElement(levelNode, "enemies");
		if (maybeEnemiesNode.first) {
			loadEnemies(maybeEnemiesNode.second);
		}
	}

	void GameLevel::addGameObject(GameObjectRef obj) {
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
			vector<double> vals = util::xml::readNumericSequence(dirStr);
			dvec2 dir(vals[0], vals[1]);
			setDirectionalGravityDirection(dir);
			setGravityType(DIRECTIONAL);

			CI_LOG_D("gravity DIRECTIONAL dir: " << dir );
		}
	}

	void GameLevel::loadTerrain(XmlTree terrainNode, ci::DataSourceRef svgData) {

		double friction = util::xml::readNumericAttribute(terrainNode, "friction", 1);
		double density = util::xml::readNumericAttribute(terrainNode, "density", 1);
		double scale = util::xml::readNumericAttribute(terrainNode, "scale", 1);

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
		terrain::World::loadSvg(svgData, dmat4(scale), shapes, anchors, elements, true);
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

	void GameLevel::loadPlayer(XmlTree playerNode, ci::DataSourceRef playerXmlData, terrain::ElementRef playerElement) {
		_player = player::Player::create("Player", playerXmlData, playerElement->getPosition());
		addGameObject(_player);

		if (playerNode.hasChild("tracking")) {
			XmlTree trackingNode = playerNode.getChild("tracking");
			double areaRadius = util::xml::readNumericAttribute(trackingNode, "areaRadius", 200);
			double deadZoneRadius = util::xml::readNumericAttribute(trackingNode, "deadZoneRadius", 30);
			double tightness = util::xml::readNumericAttribute(trackingNode, "tightness", 0.99);
			double correctiveRampPower = util::xml::readNumericAttribute(trackingNode, "correctiveRampPower", 2);

			auto t = make_shared<TargetTrackingViewportControlComponent>(_player, getViewportController());
			t->setTrackingRegion(areaRadius, deadZoneRadius, correctiveRampPower, tightness);
			
			_player->addComponent(t);
		}
	}
	
	void GameLevel::loadEnemies(XmlTree enemiesNode) {
		for (auto enemyNode : enemiesNode) {

			// load position from our terrain's elements, or from the node itself ("x", "y" attributes)
			dvec2 position;
			if (enemyNode.hasAttribute("id") && _terrain) {
				string id = enemyNode.getAttributeValue<string>("id");
				terrain::ElementRef enemyElement = _terrain->getWorld()->getElementById(id);
				CI_ASSERT_MSG(enemyElement, ("Unable to find element with id: " + id).c_str());
				position = enemyElement->getPosition();
			} else {
				string positionValue = enemyNode.getAttributeValue<string>("position");
				position = util::xml::readPointAttribute(enemyNode, "position", dvec2(0,0));
			}

			// now figure out which enemy this is
			string tag = enemyNode.getTag();
			string name = enemyNode.getAttributeValue<string>("id", tag);
			GameObjectRef enemy;

			if (tag == "eggsac") {
				enemy = enemy::Eggsac::create(name, position, enemyNode);
			}

			if (enemy) {
				addGameObject(enemy);
			}
		}
	}
	
}} // namespace core::game
