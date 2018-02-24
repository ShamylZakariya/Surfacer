//
//  IPTestsScenario.hpp
//  Tests
//
//  Created by Shamyl Zakariya on 2/22/18.
//

#ifndef IPTestsScenario_hpp
#define IPTestsScenario_hpp

#include <cinder/Rand.h>

#include "Core.hpp"
#include "Svg.hpp"

#include "Terrain.hpp"
#include "ParticleSystem.hpp"


using namespace ci;
using namespace core;
using namespace particles;

class IPTestsScenario : public Scenario {
public:
    
    IPTestsScenario();
    
    virtual ~IPTestsScenario();
    
    virtual void setup() override;
    
    virtual void cleanup() override;
    
    virtual void resize(ivec2 size) override;
    
    virtual void step(const time_state &time) override;
    
    virtual void update(const time_state &time) override;
    
    virtual void clear(const render_state &state) override;
    
    virtual void draw(const render_state &state) override;
    
    virtual void drawScreen(const render_state &state) override;
    
    virtual bool keyDown(const ci::app::KeyEvent &event) override;
    
    void reset();

private:

    ci::Channel8u testRemap(int width, int height);
    ci::Channel8u testDilate(int width, int height);
    ci::Channel8u testErode(int width, int height);

private:
    
    vector<ci::Channel8u> _channels;
    vector<ci::gl::Texture2dRef> _channelTextures;
    int32_t _seed;
        
};
#endif /* IPTestsScenario_hpp */
