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
	virtual void draw( const render_state &state ) override;

	virtual bool mouseDown( const ci::app::MouseEvent &event ) override;
	virtual bool mouseUp( const ci::app::MouseEvent &event ) override;
	virtual bool mouseWheel( const ci::app::MouseEvent &event ) override;
	virtual bool mouseMove( const ci::app::MouseEvent &event, const vec2 &delta ) override;
	virtual bool mouseDrag( const ci::app::MouseEvent &event, const vec2 &delta ) override;
	virtual bool keyDown( const ci::app::KeyEvent &event ) override;
	virtual bool keyUp( const ci::app::KeyEvent &event ) override;

	void reset();

private:

	void testBasicTerrainSetup();
	void testComplexTerrainSetup();
	void testSimpleAnchors();
	void testComplexAnchors();

	void testSimplePartitionedTerrain();


	void timeSpatialIndex();

	void drawWorldCoordinateSystem(const render_state &state);
	void releaseMouseDragConstraint();

private:

	bool _cutting;
	vec2 _cutterStart, _cutterEnd, _mouseScreen, _mouseWorld;

	ViewportController _cameraController;

	terrain::WorldRef _terrainWorld;

	cpSpace *_space;
	cpBody *_mouseBody, *_draggingBody;
	cpConstraint *_mouseJoint;
};

#endif /* IslandTestScenario_hpp */
