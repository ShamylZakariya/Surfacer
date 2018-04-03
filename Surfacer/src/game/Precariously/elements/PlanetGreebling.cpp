//
//  PlanetGreebling.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/2/18.
//

#include "PlanetGreebling.hpp"

using namespace core;
using namespace particles;
namespace precariously {
    
    GreeblingParticleSimulation::config GreeblingParticleSimulation::config::parse(const util::xml::XmlMultiTree &node) {
        config c;
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
        
        // TODO: Make radius part of the config, per-texel or whatever
        const double radius = 1;
        bool didUpdate = false;
        
        for (; state != end; ++state, ++attachments) {
            const auto &attachment = *attachments;
            bool alive = !attachment->isOrphaned();
            state->active = alive;

            if (alive) {
                if (_firstSimulate || (attachment->getStepIndexForLastPositionUpdate() == timeState.step)) {
                    state->position = attachment->getWorldPosition();
                    state->right = attachment->getWorldRotation() * radius;
                    state->up = rotateCCW(state->right);
                    state->color = ColorA(1,1,1,1);
                    state->additivity = 0;
                    state->atlasIdx = attachment->getId() % 4;
                    didUpdate = true;
                }
                bounds = cpBBExpand(bounds, state->position, radius);
            }
        }

        if (_firstSimulate || didUpdate) {
            _bb = bounds;
            notifyMoved();
        }
        
        _firstSimulate = false;
    }
    
#pragma mark - GreeblingParticleSystem
    
    GreeblingParticleSystem::config GreeblingParticleSystem::config::parse(const util::xml::XmlMultiTree &node) {
        config c;
        c.simulationConfig = GreeblingParticleSimulation::config::parse(node.getChild("simulation"));
        c.drawConfig = ParticleSystemDrawComponent::config::parse(node.getChild("draw"));
        return c;
    }
    
    GreeblingParticleSystemRef GreeblingParticleSystem::create(string name, const config &c, const vector <terrain::AttachmentRef> &attachments) {
        auto simulation = make_shared<GreeblingParticleSimulation>(c.simulationConfig);
        auto draw = make_shared<ParticleSystemDrawComponent>(c.drawConfig);
        
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
