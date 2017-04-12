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
	virtual void draw( const render_state &state ) override;

	virtual bool mouseDown( const ci::app::MouseEvent &event ) override;
	virtual bool mouseUp( const ci::app::MouseEvent &event ) override;
	virtual bool mouseWheel( const ci::app::MouseEvent &event ) override;
	virtual bool mouseMove( const ci::app::MouseEvent &event, const ivec2 &delta ) override;
	virtual bool mouseDrag( const ci::app::MouseEvent &event, const ivec2 &delta ) override;
	virtual bool keyDown( const ci::app::KeyEvent &event ) override;
	virtual bool keyUp( const ci::app::KeyEvent &event ) override;

	void reset();

private:

	terrain::WorldRef testDistantTerrain();
	terrain::WorldRef testBasicTerrain();
	terrain::WorldRef testComplexTerrain();
	terrain::WorldRef testSimpleAnchors();
	terrain::WorldRef testComplexAnchors();

	terrain::WorldRef testSimplePartitionedTerrain();
	terrain::WorldRef testComplexPartitionedTerrainWithAnchors();

	terrain::WorldRef testSimpleSvgLoad();
	terrain::WorldRef testComplexSvgLoad();

	terrain::WorldRef testFail();

	void timeSpatialIndex();

	void releaseMouseDragConstraint();

private:

	bool _cutting;
	vec2 _cutterStart, _cutterEnd, _mouseScreen, _mouseWorld;

	terrain::TerrainObjectRef _terrain;

	cpBody *_mouseBody, *_draggingBody;
	cpConstraint *_mouseJoint;
};

#endif /* IslandTestScenario_hpp */
