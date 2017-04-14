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

	struct gravity_info {
		dvec2 center_of_mass;
		double strength;

		gravity_info(dvec2 com, double s):
		center_of_mass(com),
		strength(s)
		{}
	};

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

	struct svg_element {
		dvec2 position;
		string id;
	};

	void loadTerrain(ci::DataSourceRef svgData);

private:


	terrain::TerrainObjectRef _terrain;
	map<string, svg_element> _svgElements;

};

#endif /* GameLevel_hpp */
