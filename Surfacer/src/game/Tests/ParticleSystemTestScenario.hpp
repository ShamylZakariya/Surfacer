//
//  ParticleSystemTestScenario.hpp
//  Tests
//
//  Created by Shamyl Zakariya on 11/2/17.
//

#ifndef ParticleSystemTestScenario_hpp
#define ParticleSystemTestScenario_hpp

#include "Core.hpp"
#include "Svg.hpp"

#include "Terrain.hpp"
#include "UniversalParticleSystem.hpp"


using namespace ci;
using namespace core;
using namespace particles;

class ParticleSystemTestScenario : public Scenario {
public:
	
	ParticleSystemTestScenario();
	virtual ~ParticleSystemTestScenario();
	
	virtual void setup() override;
	virtual void cleanup() override;
	virtual void resize( ivec2 size ) override;
	
	virtual void step( const time_state &time ) override;
	virtual void update( const time_state &time ) override;
	virtual void clear( const render_state &state ) override;
	virtual void drawScreen( const render_state &state ) override;
	
	virtual bool keyDown( const ci::app::KeyEvent &event ) override;
	
	void reset();
	
protected:

	void buildExplosionPs();
	void emitSmokeParticle(dvec2 world, dvec2 vel);
	void emitSparkParticle(dvec2 world, dvec2 vel);
	void emitRubbleParticle(dvec2 world, dvec2 vel);
	
private:

	UniversalParticleSystemRef _explosionPs;
	UniversalParticleSimulation::particle_template _smoke, _spark, _rubble;
	
};

#endif /* ParticleSystemTestScenario_hpp */
