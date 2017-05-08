//
//  TerrainTestScenario.hpp
//  Milestone6
//
//  Created by Shamyl Zakariya on 3/5/17.
//
//

#ifndef IslandTestScenario_hpp
#define IslandTestScenario_hpp

#include "Terrain.hpp"
#include "Scenario.hpp"

using namespace ci;
using namespace core;

class TerrainTestScenario : public Scenario {
public:

	TerrainTestScenario();
	virtual ~TerrainTestScenario();

	virtual void setup() override;
	virtual void cleanup() override;
	virtual void resize( ivec2 size ) override;

	virtual void step( const time_state &time ) override;
	virtual void update( const time_state &time ) override;
	virtual void clear( const render_state &state ) override;
	virtual void drawScreen( const render_state &state ) override;

	virtual bool keyDown( const ci::app::KeyEvent &event ) override;

	void reset();

private:

	game::terrain::WorldRef testDistantTerrain();
	game::terrain::WorldRef testBasicTerrain();
	game::terrain::WorldRef testComplexTerrain();
	game::terrain::WorldRef testSimpleAnchors();
	game::terrain::WorldRef testComplexAnchors();

	game::terrain::WorldRef testSimplePartitionedTerrain();
	game::terrain::WorldRef testComplexPartitionedTerrainWithAnchors();

	game::terrain::WorldRef testSimpleSvgLoad();
	game::terrain::WorldRef testComplexSvgLoad();

	game::terrain::WorldRef testFail();

	void timeSpatialIndex();

private:

	game::terrain::TerrainObjectRef _terrain;
};

#endif /* IslandTestScenario_hpp */
