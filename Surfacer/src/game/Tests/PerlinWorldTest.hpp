//
//  PerlinWorldTest.hpp
//  Tests
//
//  Created by Shamyl Zakariya on 11/25/17.
//

#ifndef PerlinWorldTest_hpp
#define PerlinWorldTest_hpp

#include <cinder/Rand.h>

#include "Core.hpp"
#include "Svg.hpp"

#include "Terrain.hpp"
#include "ParticleSystem.hpp"


using namespace ci;
using namespace core;
using namespace particles;

class PerlinWorldTestScenario : public Scenario {
public:
	
	PerlinWorldTestScenario();
	virtual ~PerlinWorldTestScenario();
	
	virtual void setup() override;
	virtual void cleanup() override;
	virtual void resize( ivec2 size ) override;
	
	virtual void step( const time_state &time ) override;
	virtual void update( const time_state &time ) override;
	virtual void clear( const render_state &state ) override;
	virtual void draw( const render_state &state ) override;
	virtual void drawScreen( const render_state &state ) override;
	
	virtual bool keyDown( const ci::app::KeyEvent &event ) override;
	
	void reset();
	
private:
	
	struct segment {
		dvec2 a, b;
		ci::Color color;
	};
	
	struct polyline {
		ci::PolyLine2 pl;
		ci::Color color;
	};
	
	ci::Channel8u createWorldMap() const;
	ci::Channel8u createSimpleTestMap() const;
	vector<polyline> marchToPerimeters(ci::Channel8u &iso, size_t expectedContourCount) const;
	vector<segment> testMarch(ci::Channel8u &iso) const;
	terrain::WorldRef createTerrainWorld(const ci::Channel8u &worldMap) const;
	
private:
	
	ci::Channel8u _isoSurface;
	ci::gl::Texture2dRef _isoTex;
	vector<segment> _marchSegments;
	vector<polyline> _marchedPolylines;
	int32_t _seed;
	
};

#endif /* PerlinWorldTest_hpp */
