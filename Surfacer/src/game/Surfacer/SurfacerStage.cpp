//
//  GameStage.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/13/17.
//
//

#include "SurfacerStage.hpp"

#include "DevComponents.hpp"
#include "Eggsac.hpp"
#include "Xml.hpp"

using namespace core;

namespace surfacer {

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
	 RadialGravitationCalculatorRef _gravity;
	 player::PlayerRef _player;
	 set<EntityRef> _enemies;
	 */

	SurfacerStage::SurfacerStage():
	Stage("Unnamed") {}

	SurfacerStage::~SurfacerStage()
	{}

	void SurfacerStage::addObject(ObjectRef obj) {
		Stage::addObject(obj);
	}

	void SurfacerStage::removeObject(ObjectRef obj) {
		Stage::removeObject(obj);

		// if this obj was an Enemy, remove it from our store
		if (EntityRef entity = dynamic_pointer_cast<Entity>(obj)) {
			_enemies.erase(entity);
		}

		// if this obj was the player, remove it from our store
		if (_player == obj) {
			_player.reset();
		}
	}

	void SurfacerStage::load(ci::DataSourceRef stageXmlData) {

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
		
		for (const auto &childNode : stageNode.getChildren()) {
			if (childNode->getTag() == "gravity") {
				buildGravity(*childNode);
			}
		}

		//
		//	Load terrain
		//

		auto terrainNode = util::xml::findElement(stageNode, "terrain");
		CI_ASSERT_MSG(terrainNode, "Expect a <terrain> node in <stage> definition");
		string terrainSvgPath = terrainNode->getAttribute("path").getValue();
		loadTerrain(*terrainNode, app::loadAsset(terrainSvgPath));

		//
		//	Load the player
		//

		if (auto playerNode = util::xml::findElement(stageNode, "player")) {
			terrain::ElementRef playerElement = _terrain->getWorld()->getElementById("player");
			ci::DataSourceRef playerXmlData = app::loadAsset(playerNode->getAttribute("path").getValue());
			loadPlayer(*playerNode, playerXmlData, playerElement);
		}

		//
		//	Load enemies
		//

		if (auto enemiesNode = util::xml::findElement(stageNode, "enemies")) {
			loadEnemies(*enemiesNode, prefabsNode);
		}
	}

	void SurfacerStage::onReady() {
		Stage::onReady();

		addContactHandler(CollisionType::ENEMY, CollisionType::PLAYER, [this](const Stage::collision_type_pair &ctp, const ObjectRef &enemy, const ObjectRef &player){
			this->onPlayerEnemyContact(dynamic_pointer_cast<Entity>(enemy));
		});

		addContactHandler(CollisionType::WEAPON, CollisionType::ENEMY, [this](const Stage::collision_type_pair &ctp, const ObjectRef &weapon, const ObjectRef &enemy){
			this->onPlayerShotEnemy(weapon, dynamic_pointer_cast<Entity>(enemy));
		});
	}

	bool SurfacerStage::onCollisionBegin(cpArbiter *arb) {
		return true;
	}

	bool SurfacerStage::onCollisionPreSolve(cpArbiter *arb) {
		return true;
	}

	void SurfacerStage::onCollisionPostSolve(cpArbiter *arb) {
	}

	void SurfacerStage::onCollisionSeparate(cpArbiter *arb) {
	}

	void SurfacerStage::applySpaceAttributes(XmlTree spaceNode) {
		if (spaceNode.hasAttribute("damping")) {
			double damping = util::xml::readNumericAttribute<double>(spaceNode, "damping", 0.95);
			damping = clamp(damping, 0.0, 1.0);
			cpSpaceSetDamping(getSpace()->getSpace(), damping);
		}
	}

	void SurfacerStage::buildGravity(XmlTree gravityNode) {
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

	void SurfacerStage::loadTerrain(XmlTree terrainNode, ci::DataSourceRef svgData) {

		double friction = util::xml::readNumericAttribute<double>(terrainNode, "friction", 1);
		double density = util::xml::readNumericAttribute<double>(terrainNode, "density", 1);
		double scale = util::xml::readNumericAttribute<double>(terrainNode, "scale", 1);
		double collisionShapeRadius = 1;

		const double minDensity = 1e-3;
		density = max(density, minDensity);

		const double minSurfaceArea = 2;
		const ci::Color terrainColor = util::xml::readColorAttribute(terrainNode, "color", Color(1,0,1));
		const ci::Color anchorColor = util::xml::readColorAttribute(terrainNode, "anchorColor", Color(0,1,1));

		const terrain::material terrainMaterial(density, friction, collisionShapeRadius, ShapeFilters::TERRAIN, CollisionType::TERRAIN, minSurfaceArea, terrainColor);
		const terrain::material anchorMaterial(1, friction, collisionShapeRadius, ShapeFilters::ANCHOR, CollisionType::ANCHOR, minSurfaceArea, anchorColor);

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
		//	Look for a center of mass for gravity
		//

		if (_gravity) {
			if (terrain::ElementRef e = _terrain->getWorld()->getElementById("center_of_mass")) {
				dvec2 centerOfMass = e->getModelMatrix() * e->getModelCentroid();
				_gravity->setCenterOfMass(centerOfMass);
			}			
		}
	}

	void SurfacerStage::loadPlayer(XmlTree playerNode, ci::DataSourceRef playerXmlData, terrain::ElementRef playerElement) {
		string name = playerNode.getAttributeValue<string>("name");
		_player = player::Player::create(name, playerXmlData, playerElement->getPosition());
		addObject(_player);

		if (playerNode.hasChild("tracking")) {
			XmlTree trackingNode = playerNode.getChild("tracking");
			double areaRadius = util::xml::readNumericAttribute<double>(trackingNode, "areaRadius", 200);
			double deadZoneRadius = util::xml::readNumericAttribute<double>(trackingNode, "deadZoneRadius", 30);
			double tightness = util::xml::readNumericAttribute<double>(trackingNode, "tightness", 0.99);
			double correctiveRampPower = util::xml::readNumericAttribute<double>(trackingNode, "correctiveRampPower", 2);

			auto t = make_shared<TargetTrackingViewportControlComponent>(_player, getViewportController());
			t->setTrackingRegion(areaRadius, deadZoneRadius, correctiveRampPower, tightness);
			
			_player->addComponent(t);
		}
	}
	
	void SurfacerStage::loadEnemies(XmlTree enemiesNode, XmlTree prefabsNode) {
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
				auto prefabNode = util::xml::findElement(prefabsNode, "prefab", "id", prefabId);
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
			EntityRef enemy = classload(tag, name, position, prefabEnemyNode);

			if (enemy) {
				_enemies.insert(enemy);
				addObject(enemy);
			}
		}
	}

	EntityRef SurfacerStage::classload(string tag, string name, dvec2 position, util::xml::XmlMultiTree prefabNode) {
		// I'm not going to build a classloader any more
		if (tag == "eggsac") {
			return enemy::Eggsac::create(name, position, prefabNode);
		}

		return nullptr;
	}

	void SurfacerStage::onPlayerEnemyContact(const EntityRef &enemy) {
		HealthComponentRef health = _player->getHealthComponent();
		// TODO: Define contact damage for enemy touching player
	}

	void SurfacerStage::onPlayerShotEnemy(const ObjectRef &weapon, const EntityRef &enemy) {
		player::ProjectileRef projectile = weapon->getComponent<player::Projectile>();
		HealthComponentRef health = enemy->getHealthComponent();

		float damageToDeal = projectile->getDamage();
		health->takeInjury(damageToDeal);
		CI_LOG_D(enemy->getName() << " shot, taking " << damageToDeal << " damage yeilding health of: " << health->getHealth());
	}

	
} // namespace surfacer
