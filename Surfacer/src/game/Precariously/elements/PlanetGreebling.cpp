//
//  PlanetGreebling.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/2/18.
//

#include "PlanetGreebling.hpp"

#include "GlslProgLoader.hpp"

using namespace core;
using namespace particles;
namespace precariously {
    
    GreeblingParticleSimulation::config GreeblingParticleSimulation::config::parse(const util::xml::XmlMultiTree &node) {
        config c;
        // TODO: Implement parser
        return c;
    }

    
    /*
     config _config;
     cpBB _bb;
     bool _firstSimulate;
     vector <terrain::AttachmentRef> _attachments;
     */
    
    GreeblingParticleSimulation::GreeblingParticleSimulation(const config &c):
            _config(c),
            _firstSimulate(true)
    {}
    
    void GreeblingParticleSimulation::onReady(core::ObjectRef parent, core::StageRef stage) {
        BaseParticleSimulation::onReady(parent, stage);
        if (!_attachments.empty()) {
            simulate(stage->getTimeState());
        }
    }
    
    void GreeblingParticleSimulation::update(const core::time_state &timeState) {
        simulate(timeState);
    }
    
    void GreeblingParticleSimulation::setParticleCount(size_t count) {
        BaseParticleSimulation::setParticleCount(count);
    }
    
    void GreeblingParticleSimulation::setAttachments(const vector <terrain::AttachmentRef> &attachments) {
        _attachments = attachments;
        setParticleCount(_attachments.size());
    }
    
    void GreeblingParticleSimulation::simulate(const core::time_state &timeState) {
        cpBB bounds = cpBBInvalid;
        auto attachments = _attachments.begin();
        auto state = _state.begin();
        const auto end = _state.end();
        
        bool didUpdate = false;
        
        const size_t numAtlasDetails = _config.atlas_details.size();
        const config::atlas_detail *atlasDetails = _config.atlas_details.data();
        
        for (; state != end; ++state, ++attachments) {
            const auto &attachment = *attachments;
            bool alive = !attachment->isOrphaned();
            state->active = alive;

            if (alive) {
                // only update particle state if particle moved this timestep, (or has never been moved yet)
                if (_firstSimulate || (attachment->getStepIndexForLastPositionUpdate() == timeState.step)) {
                    
                    // compute the atlas idx for this particle, and get the info on that idx for radius and offset
                    state->atlasIdx = attachment->getId() % numAtlasDetails;
                    config::atlas_detail atlasDetail = atlasDetails[state->atlasIdx];

                    // set right and up vectors for this particle
                    state->right = attachment->getWorldRotation() * atlasDetail.radius;
                    state->up = rotateCCW(state->right);

                    // move particle to position + offset that moves it such that base touches terrain surface, + per-atlas idx offset
                    state->position = attachment->getWorldPosition() + (state->up * (0.5 + atlasDetail.upOffset));

                    state->color = atlasDetail.color;
                    state->additivity = 0;
                    didUpdate = true;

                    bounds = cpBBExpand(bounds, state->position, atlasDetail.radius);
                } else {
                    // particle didn't move, but we need to include it in the bounds. radius of 2 is a "guess", we could
                    // also use a zero-radius or a huge radius. I suspect 2 is safe for greebling.
                    bounds = cpBBExpand(bounds, state->position, 2);
                }
            }
        }

        if (_firstSimulate || didUpdate) {
            _bb = bounds;
            notifyMoved();
        }
        
        _firstSimulate = false;
    }

#pragma mark - GreeblingParticleSystemDrawComponent
    
    shared_ptr<GreeblingParticleSystemDrawComponent> GreeblingParticleSystemDrawComponent::create(config c) {
        if (!c.shader) {
            c.shader = core::util::loadGlslAsset("precariously/shaders/greebling.glsl");
        }
        return make_shared<GreeblingParticleSystemDrawComponent>(c);
    }

    GreeblingParticleSystemDrawComponent::GreeblingParticleSystemDrawComponent(config c):
    ParticleSystemDrawComponent(c),
    _config(c)
    {
        CI_ASSERT(c.swayFactorByAtlasIdx.size() == Atlas::ElementCount(c.atlasType));
    }
    
    void GreeblingParticleSystemDrawComponent::setShaderUniforms(const gl::GlslProgRef &program, const core::render_state &renderState) {
        ParticleSystemDrawComponent::setShaderUniforms(program, renderState);
        program->uniform("swayPeriod", static_cast<float>(_config.swayPeriod));
        program->uniform("swayFactors", _config.swayFactorByAtlasIdx.data(), static_cast<int>(_config.swayFactorByAtlasIdx.size()));
    }
    
#pragma mark - GreeblingParticleSystem
    
    GreeblingParticleSystem::config GreeblingParticleSystem::config::parse(const util::xml::XmlMultiTree &node) {
        config c;
        // TODO: Implement a good XML layout for this
//        c.simulationConfig = GreeblingParticleSimulation::config::parse(node.getChild("simulation"));
//        c.drawConfig = ParticleSystemDrawComponent::config::parse(node.getChild("draw"));
        return c;
    }
    
    GreeblingParticleSystemRef GreeblingParticleSystem::create(string name, const config &c, const vector <terrain::AttachmentRef> &attachments) {
        auto simulation = make_shared<GreeblingParticleSimulation>(c.simulationConfig);
        auto draw = GreeblingParticleSystemDrawComponent::create(c.drawConfig);
        
        auto gps = make_shared<GreeblingParticleSystem>(name, c);
        gps->addComponent(draw);
        gps->addComponent(simulation);
        
        simulation->setAttachments(attachments);
        
        return gps;
    }
    
    GreeblingParticleSystem::GreeblingParticleSystem(string name, const config &c) :
            BaseParticleSystem(name),
            _config(c)
    {
    }

}
