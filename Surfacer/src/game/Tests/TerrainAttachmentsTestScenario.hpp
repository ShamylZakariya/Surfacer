//
//  TerrainAttachmentsTestScenario.hpp
//  Tests
//
//  Created by Shamyl Zakariya on 2/8/18.
//

#ifndef TerrainAttachmentsTestScenario_hpp
#define TerrainAttachmentsTestScenario_hpp

#include "Terrain.hpp"
#include "Scenario.hpp"

using namespace ci;
using namespace core;

class TerrainAttachmentsTestScenario : public Scenario {
public:
    
    TerrainAttachmentsTestScenario();
    
    virtual ~TerrainAttachmentsTestScenario();
    
    virtual void setup() override;
    
    virtual void cleanup() override;
    
    virtual void resize(ivec2 size) override;
    
    virtual void step(const time_state &time) override;
    
    virtual void update(const time_state &time) override;
    
    virtual void clear(const render_state &state) override;
    
    virtual void drawScreen(const render_state &state) override;
    
    virtual bool keyDown(const ci::app::KeyEvent &event) override;
    
    void reset();
    
private:
    
    terrain::WorldRef testBasicTerrain();
    terrain::WorldRef testBasicAttachmentAdapter();
    terrain::WorldRef testComplexSvgLoad();
    
private:
    
    terrain::TerrainObjectRef _terrain;
};

#endif /* TerrainAttachmentsTestScenario_hpp */
