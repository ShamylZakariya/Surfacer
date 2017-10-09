//
//  GameLevel.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/13/17.
//
//

#include "SurfacerLevel.hpp"

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
	 player::PlayerRef _player;
	 set<EntityRef> _enemies;
	 */

	SurfacerLevel::SurfacerLevel():
	Level("Unnamed") {}

	SurfacerLevel::~SurfacerLevel()
	{}

	void SurfacerLevel::addObject(ObjectRef obj) {
		Level::addObject(obj);
	}

	void SurfacerLevel::removeObject(ObjectRef obj) {
		Level::removeObject(obj);

		// if this obj was an Enemy, remove it from our store
		if (EntityRef entity = dynamic_pointer_cast<Entity>(obj)) {
			_enemies.erase(entity);
		}

		// if this obj was the player, remove it from our store
		if (_player == obj) {
			_player.reset();
		}
	}

	void SurfacerLevel::load(ci::DataSourceRef levelXmlData) {

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

	void SurfacerLevel::onReady() {
		Level::onReady();

		addContactHandler(CollisionType::ENEMY, CollisionType::PLAYER, [this](const Level::collision_type_pair &ctp, const ObjectRef &enemy, const ObjectRef &player){
			this->onPlayerEnemyContact(dynamic_pointer_cast<Entity>(enemy));
		});

		addContactHandler(CollisionType::WEAPON, CollisionType::ENEMY, [this](const Level::collision_type_pair &ctp, const ObjectRef &weapon, const ObjectRef &enemy){
			this->onPlayerShotEnemy(weapon, dynamic_pointer_cast<Entity>(enemy));
		});
	}

	bool SurfacerLevel::onCollisionBegin(cpArbiter *arb) {
		return true;
	}

	bool SurfacerLevel::onCollisionPreSolve(cpArbiter *arb) {
		return true;
	}

	void SurfacerLevel::onCollisionPostSolve(cpArbiter *arb) {
	}

	void SurfacerLevel::onCollisionSeparate(cpArbiter *arb) {
	}

	void SurfacerLevel::applySpaceAttributes(XmlTree spaceNode) {
		if (spaceNode.hasAttribute("damping")) {
			double damping = util::xml::readNumericAttribute<double>(spaceNode, "damping", 0.95);
			damping = clamp(damping, 0.0, 1.0);
			cpSpaceSetDamping(getSpace()->getSpace(), damping);
		}
	}

	void SurfacerLevel::applyGravityAttributes(XmlTree gravityNode) {
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

	void SurfacerLevel::loadTerrain(XmlTree terrainNode, ci::DataSourceRef svgData) {

		double friction = util::xml::readNumericAttribute<double>(terrainNode, "friction", 1);
		double density = util::xml::readNumericAttribute<double>(terrainNode, "density", 1);
		double scale = util::xml::readNumericAttribute<double>(terrainNode, "scale", 1);
		double collisionShapeRadius = 1;

		const double minDensity = 1e-3;
		density = max(density, minDensity);

		const terrain::material terrainMaterial(density, friction, collisionShapeRadius, ShapeFilters::TERRAIN, CollisionType::TERRAIN);
		const terrain::material anchorMaterial(1, friction, collisionShapeRadius, ShapeFilters::ANCHOR, CollisionType::ANCHOR);

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

	void SurfacerLevel::loadPlayer(XmlTree playerNode, ci::DataSourceRef playerXmlData, terrain::ElementRef playerElement) {
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
	
	void SurfacerLevel::loadEnemies(XmlTree enemiesNode, XmlTree prefabsNode) {
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
			EntityRef enemy = classload(tag, name, position, prefabEnemyNode);

			if (enemy) {
				_enemies.insert(enemy);
				addObject(enemy);
			}
		}
	}

	EntityRef SurfacerLevel::classload(string tag, string name, dvec2 position, util::xml::XmlMultiTree prefabNode) {
		// I'm not going to build a classloader any more
		if (tag == "eggsac") {
			return enemy::Eggsac::create(name, position, prefabNode);
		}

		return nullptr;
	}

	void SurfacerLevel::onPlayerEnemyContact(const EntityRef &enemy) {
		HealthComponentRef health = _player->getHealthComponent();
		// TODO: Define contact damage for enemy touching player
	}

	void SurfacerLevel::onPlayerShotEnemy(const ObjectRef &weapon, const EntityRef &enemy) {
		player::ProjectileRef projectile = weapon->getComponent<player::Projectile>();
		HealthComponentRef health = enemy->getHealthComponent();

		float damageToDeal = projectile->getDamage();
		health->takeInjury(damageToDeal);
		CI_LOG_D(enemy->getName() << " shot, taking " << damageToDeal << " damage yeilding health of: " << health->getHealth());
	}

	
} // namespace surfacer
