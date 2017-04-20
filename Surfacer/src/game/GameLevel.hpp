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

SMART_PTR(GameLevel);

namespace Categories {
	enum Categories {
		TERRAIN = 1 << 30,
		CUTTER = 1 << 29,
		PICK = 1 << 28,
		ANCHOR = 1 << 27
	};
};

namespace Filters {
	extern cpShapeFilter TERRAIN;
	extern cpShapeFilter ANCHOR;
	extern cpShapeFilter CUTTER;
	extern cpShapeFilter PICK;
}

class GameLevel : public core::Level {
public:
	GameLevel();
	virtual ~GameLevel();

	void load(ci::DataSourceRef levelXmlData);
	terrain::TerrainObjectRef getTerrain() const { return _terrain; }

	//
	//	Level
	//

	void addGameObject(core::GameObjectRef obj) override;

protected:

	void applySpaceAttributes(XmlTree spaceNode);
	void applyGravityAttributes(XmlTree gravityNode);
	void loadTerrain(XmlTree terrainNode, ci::DataSourceRef svgData);

private:

	terrain::TerrainObjectRef _terrain;

};

#endif /* GameLevel_hpp */
