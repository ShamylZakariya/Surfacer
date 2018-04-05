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

#pragma mark - GreeblingParticleSystemDrawComponent
    namespace {

auto vsh = R"(
#version 150
uniform mat4 ciModelViewProjection;
uniform float ciElapsedSeconds;

in vec4 ciPosition;
in vec2 ciNormal;
in vec2 ciTexCoord0;
in vec2 ciTexCoord1;
in vec2 ciTexCoord2;
in vec4 ciColor;

out vec2 Up;
out vec2 TexCoord;
out vec2 VertexPosition;
out vec2 Random;
out vec4 Color;

void main(void) {
    Up = ciNormal.xy;
    TexCoord = ciTexCoord0;
    VertexPosition = ciTexCoord1;
    Random = ciTexCoord2;
    Color = ciColor;

    vec4 right = vec4(Up.y, -Up.x, 0, 0);
    vec4 wiggle = cos((ciElapsedSeconds + (Random.x * 3.14159)) / 3.14159) * right;
    
    gl_Position = ciModelViewProjection * (ciPosition + wiggle * (1.0-VertexPosition.y));
}
)";
        
auto fsh = R"(
#version 150
uniform sampler2D uTex0;

in vec2 Up;
in vec2 TexCoord;
in vec2 VertexPosition;
in vec2 Random;
in vec4 Color;

out vec4 oColor;

void main(void) {
    float alpha = round(texture(uTex0, TexCoord).r);
    
    // NOTE: additive blending requires premultiplication
    oColor.rgb = Color.rgb * alpha;
    oColor.rg *= 0.5 + (Random * 0.5);
    oColor.a = Color.a * alpha;
}
)";
        gl::GlslProgRef greebling_ps_shader() {
            return gl::GlslProg::create(gl::GlslProg::Format().vertex(vsh).fragment(fsh));
        }
        
    }
    
    shared_ptr<GreeblingParticleSystemDrawComponent> GreeblingParticleSystemDrawComponent::create(config c) {
        if (!c.shader) {
            c.shader = greebling_ps_shader();
        }
        return make_shared<GreeblingParticleSystemDrawComponent>(c);
    }

    GreeblingParticleSystemDrawComponent::GreeblingParticleSystemDrawComponent(config c):
    ParticleSystemDrawComponent(c)
    {}
    
    void GreeblingParticleSystemDrawComponent::setShaderUniforms(const core::render_state &renderState) {
        ParticleSystemDrawComponent::setShaderUniforms(renderState);
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
