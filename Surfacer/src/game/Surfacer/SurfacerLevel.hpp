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


namespace surfacer {

	SMART_PTR(SurfacerLevel);

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

	class SurfacerLevel : public core::Level {
	public:
		SurfacerLevel();
		virtual ~SurfacerLevel();

		//
		//	Level
		//

		void addObject(core::ObjectRef obj) override;
		void removeObject(core::ObjectRef obj) override;

		//
		//	GameLevel
		//

		void load(ci::DataSourceRef levelXmlData);
		terrain::TerrainObjectRef getTerrain() const { return _terrain; }
		player::PlayerRef getPlayer() const { return _player; }
		const set<core::EntityRef> &getEnemies() const { return _enemies; }


	protected:

		// Level
		void onReady() override;
		bool onCollisionBegin(cpArbiter *arb) override;
		bool onCollisionPreSolve(cpArbiter *arb) override;
		void onCollisionPostSolve(cpArbiter *arb) override;
		void onCollisionSeparate(cpArbiter *arb) override;

		// GameLevel
		void applySpaceAttributes(XmlTree spaceNode);
		void buildGravity(XmlTree gravityNode);
		void loadTerrain(XmlTree terrainNode, ci::DataSourceRef svgData);
		void loadPlayer(XmlTree playerNode, ci::DataSourceRef playerXmlData, terrain::ElementRef playerElement);
		void loadEnemies(XmlTree enemiesNode, XmlTree prefabsNode);

		virtual core::EntityRef classload(string tag, string name, dvec2 position, core::util::xml::XmlMultiTree node);

		virtual void onPlayerEnemyContact(const core::EntityRef &enemy);
		virtual void onPlayerShotEnemy(const core::ObjectRef &weapon, const core::EntityRef &enemy);

	private:

		terrain::TerrainObjectRef _terrain;
		core::RadialGravitationCalculatorRef _gravity;
		player::PlayerRef _player;
		set<core::EntityRef> _enemies;
		
	};
	
} // namespace surfacer

#endif /* GameLevel_hpp */
