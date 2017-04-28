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

SMART_PTR(GameLevel);


namespace CollisionType {

	/*
		The high 16 bits are a mask, the low are a type_id, the actual type is the | of the two.
	 */

	namespace is {
		enum bits {
			SHOOTABLE   = 1 << 16,
			TOWABLE		= 1 << 17
		};
	}

	enum type_id {

		TERRAIN				= 1 | is::SHOOTABLE | is::TOWABLE,	// 196609
		MONSTER				= 2 | is::SHOOTABLE,				// 65538
		PLAYER				= 3 | is::SHOOTABLE,				// 65539
		WEAPON				= 4,								// 4
		FLUID				= 5,								// 5
		DECORATION			= 6 | is::TOWABLE | is::SHOOTABLE,	// 196614
		STATIC_DECORATION	= 7 | is::SHOOTABLE,				// 65543
		SENSOR				= 10								// 10

	};	
}

namespace CollisionFilters {

	enum Categories {
		_TERRAIN = 1 << 30,
		_CUTTER = 1 << 29,
		_PICK = 1 << 28,
		_ANCHOR = 1 << 27,
		_PLAYER = 1 << 26
	};

	extern cpShapeFilter TERRAIN;
	extern cpShapeFilter ANCHOR;
	extern cpShapeFilter CUTTER;
	extern cpShapeFilter PICK;
	extern cpShapeFilter PLAYER;
}

namespace DrawLevels {
	enum Levels {
		BACKGROUND = 0,
		TERRAIN = 1,
		ENEMY = 2,
		PLAYER = 3
	};
}

class GameLevel : public core::Level {
public:
	GameLevel();
	virtual ~GameLevel();

	void load(ci::DataSourceRef levelXmlData);
	terrain::TerrainObjectRef getTerrain() const { return _terrain; }
	player::PlayerRef getPlayer() const { return _player; }

	//
	//	Level
	//

	void addGameObject(core::GameObjectRef obj) override;

protected:

	void applySpaceAttributes(XmlTree spaceNode);
	void applyGravityAttributes(XmlTree gravityNode);
	void loadTerrain(XmlTree terrainNode, ci::DataSourceRef svgData);
	void loadPlayer(XmlTree playerNode, ci::DataSourceRef playerXmlData, terrain::ElementRef playerElement);

private:

	terrain::TerrainObjectRef _terrain;
	player::PlayerRef _player;

};

#endif /* GameLevel_hpp */
