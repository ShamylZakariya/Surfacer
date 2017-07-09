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
		cpShapeFilter TERRAIN = cpShapeFilterNew(CP_NO_GROUP, _TERRAIN, _TERRAIN | _TERRAIN_PROBE | _GRABBABLE | _ANCHOR | _PLAYER | _ENEMY);
		cpShapeFilter ANCHOR = cpShapeFilterNew(CP_NO_GROUP, _ANCHOR, _TERRAIN | _PLAYER | _ENEMY);
		cpShapeFilter TERRAIN_PROBE = cpShapeFilterNew(CP_NO_GROUP, _TERRAIN_PROBE, _TERRAIN);
		cpShapeFilter GRABBABLE = cpShapeFilterNew(CP_NO_GROUP, _GRABBABLE, _TERRAIN | _ENEMY);
		cpShapeFilter PLAYER = cpShapeFilterNew(CP_NO_GROUP, _PLAYER, _TERRAIN | _ANCHOR | _ENEMY);
		cpShapeFilter ENEMY = cpShapeFilterNew(CP_NO_GROUP, _ENEMY, _TERRAIN | _ANCHOR | _PLAYER | _GRABBABLE);
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

		//
		//	Load the player
		//

		if (auto playerNode = util::xml::findElement(levelNode, "player")) {
			terrain::ElementRef playerElement = _terrain->getWorld()->getElementById("player");
			ci::DataSourceRef playerXmlData = app::loadAsset(playerNode->getAttribute("path").getValue());
			loadPlayer(*playerNode, playerXmlData, playerElement);
		}

		//
		//	Load enemies
		//

		if (auto enemiesNode = util::xml::findElement(levelNode, "enemies")) {
			loadEnemies(*enemiesNode, prefabsNode);
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
			dvec2 dir = util::xml::readPointAttribute(gravityNode, "dir", dvec2(0,0));
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
		const terrain::material anchorMaterial(1, friction, CollisionFilters::ANCHOR, CollisionType::ANCHOR);

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
		string name = playerNode.getAttributeValue<string>("name");
		_player = player::Player::create(name, playerXmlData, playerElement->getPosition());
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
	
	void GameLevel::loadEnemies(XmlTree enemiesNode, XmlTree prefabsNode) {
		for (auto enemyNode : enemiesNode) {

			// load position from our terrain's elements, or from the node itself ("x", "y" attributes)
			dvec2 position;
			if (enemyNode.hasAttribute("name") && _terrain) {
				string name = enemyNode.getAttributeValue<string>("name");
				terrain::ElementRef enemyElement = _terrain->getWorld()->getElementById(name);
				CI_ASSERT_MSG(enemyElement, ("Unable to find element with id: " + name).c_str());
				position = enemyElement->getPosition();
			} else {
				string positionValue = enemyNode.getAttributeValue<string>("position");
				position = util::xml::readPointAttribute(enemyNode, "position", dvec2(0,0));
			}

			vector<XmlTree> enemyNodes = { enemyNode };

			// find the corresponding prefab node, if any
			if (enemyNode.hasAttribute("prefab_id")) {
				string prefabId = enemyNode.getAttributeValue<string>("prefab_id");
				auto prefabNode = util::xml::findNode(prefabsNode, "prefab", "id", prefabId);
				if (prefabNode) {
					XmlTree enemyPrefabNode = prefabNode->getChild(enemyNode.getTag());
					enemyNodes.push_back(enemyPrefabNode);
				}
			}

			// build a node which can read the prefab values if available
			util::xml::XmlMultiTree prefabEnemyNode(enemyNodes);

			// now figure out which enemy this is
			string tag = enemyNode.getTag();
			string name = enemyNode.getAttributeValue<string>("name", tag);
			EntityRef enemy;

			if (tag == "eggsac") {
				enemy = enemy::Eggsac::create(name, position, prefabEnemyNode);
			}

			if (enemy) {
				_enemies.push_back(enemy);
				addGameObject(enemy);
			}
		}
	}
	
}} // namespace core::game
