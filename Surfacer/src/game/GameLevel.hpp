//
//  GameLevel.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/13/17.
//
//

#ifndef GameLevel_hpp
#define GameLevel_hpp

#include <cinder/Xml.h>

#include "Core.hpp"
#include "Terrain.hpp"
#include "Player.hpp"


namespace core { namespace game {

	SMART_PTR(GameLevel);

#pragma mark - Constants

	namespace CollisionType {

		/*
		 The high 16 bits are a mask, the low are a type_id, the actual type is the logical OR of the two.
		 */

		namespace is {
			enum bits {
				SHOOTABLE   = 1 << 16,
				TOWABLE		= 1 << 17
			};
		}

		enum type_id {

			TERRAIN				= 1 | is::SHOOTABLE | is::TOWABLE,
			ANCHOR				= 2 | is::SHOOTABLE,
			ENEMY				= 3 | is::SHOOTABLE,
			PLAYER				= 4 | is::SHOOTABLE,
			WEAPON				= 5,
			FLUID				= 6,
			DECORATION			= 7 | is::TOWABLE | is::SHOOTABLE,
			STATIC_DECORATION	= 8 | is::SHOOTABLE,
			SENSOR				= 9

		};
	}

	namespace ShapeFilters {

		enum Categories {
			_TERRAIN = 1 << 0,			// 1
			_TERRAIN_PROBE = 1 << 1,		// 2
			_GRABBABLE = 1 << 2,			// 4
			_ANCHOR = 1 << 3,			// 8
			_PLAYER = 1 << 4,			// 16
			_ENEMY = 1 << 5				// 32
		};

		extern cpShapeFilter TERRAIN;
		extern cpShapeFilter TERRAIN_PROBE; // special filter for making queries against terrain
		extern cpShapeFilter ANCHOR;
		extern cpShapeFilter GRABBABLE;
		extern cpShapeFilter PLAYER;
		extern cpShapeFilter ENEMY;
	}

	namespace DrawLayers {
		enum layer {
			BACKGROUND = 0,
			TERRAIN = 1,
			ENEMY = 2,
			PLAYER = 3
		};
	}


#pragma mark - GameLevel

	class GameLevel : public Level {
	public:
		GameLevel();
		virtual ~GameLevel();

		//
		//	Level
		//

		void addGameObject(GameObjectRef obj) override;
		void removeGameObject(GameObjectRef obj) override;

		//
		//	GameLevel
		//

		void load(ci::DataSourceRef levelXmlData);
		terrain::TerrainObjectRef getTerrain() const { return _terrain; }
		player::PlayerRef getPlayer() const { return _player; }
		const set<EntityRef> &getEnemies() const { return _enemies; }


	protected:

		// Level
		void onReady() override;
		bool onCollisionBegin(cpArbiter *arb) override;
		bool onCollisionPreSolve(cpArbiter *arb) override;
		void onCollisionPostSolve(cpArbiter *arb) override;
		void onCollisionSeparate(cpArbiter *arb) override;

		// GameLevel
		void applySpaceAttributes(XmlTree spaceNode);
		void applyGravityAttributes(XmlTree gravityNode);
		void loadTerrain(XmlTree terrainNode, ci::DataSourceRef svgData);
		void loadPlayer(XmlTree playerNode, ci::DataSourceRef playerXmlData, terrain::ElementRef playerElement);
		void loadEnemies(XmlTree enemiesNode, XmlTree prefabsNode);

		virtual EntityRef classload(string tag, string name, dvec2 position, util::xml::XmlMultiTree node);

		virtual void onPlayerEnemyContact(const EntityRef &enemy);
		virtual void onPlayerShotEnemy(const GameObjectRef &weapon, const EntityRef &enemy);

	private:

		terrain::TerrainObjectRef _terrain;
		player::PlayerRef _player;
		set<EntityRef> _enemies;
		
	};
	
}} // namespace core::game

#endif /* GameLevel_hpp */
