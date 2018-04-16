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
    
    /*
     config _config;
     cpBB _bb;
     bool _firstSimulate;
     vector <terrain::AttachmentRef> _attachments;
     */
    
    GreeblingParticleSimulation::GreeblingParticleSimulation(const config &c):
            _config(c),
            _firstSimulate(true)
    {
    }
    
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
        setupAtlasIndices();
    }
    
    void GreeblingParticleSimulation::setupAtlasIndices() {
        
        // build index table with entry count biased by atlast probability
        vector <size_t> atlasIndices;
        for (size_t i = 0, N = _config.atlasDetails.size(); i < N; i++) {
            for (size_t j = 0, count = _config.atlasDetails[i].probability; j < count; j++) {
                atlasIndices.push_back(i);
            }
        }
        
        // assign atlas index for each greeble particle
        auto attachments = _attachments.begin();
        auto state = _state.begin();
        const auto end = _state.end();
        
        for (; state != end; ++state, ++attachments) {
            const auto &attachment = *attachments;
            
            // compute the atlas idx for this particle, and get the info on that idx for radius and offset
            state->atlasIdx = atlasIndices[attachment->getId() % atlasIndices.size()];
        }
    }
    
    void GreeblingParticleSimulation::simulate(const core::time_state &timeState) {
        cpBB bounds = cpBBInvalid;
        auto attachments = _attachments.begin();
        auto state = _state.begin();
        const auto end = _state.end();
        
        bool didUpdate = false;
        
        const config::atlas_detail *atlasDetails = _config.atlasDetails.data();
        
        for (; state != end; ++state, ++attachments) {
            const auto &attachment = *attachments;
            bool alive = !attachment->isOrphaned();
            state->active = alive;

            if (alive) {
                // only update particle state if particle moved this timestep, (or has never been moved yet)
                if (_firstSimulate || (attachment->getStepIndexForLastPositionUpdate() == timeState.step)) {

                    // get the atlas info for this particle
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
    }
    
    void GreeblingParticleSystemDrawComponent::setShaderUniforms(const gl::GlslProgRef &program, const core::render_state &renderState) {
        ParticleSystemDrawComponent::setShaderUniforms(program, renderState);
        program->uniform("swayFactor", _config.swayFactorByAtlasIdx.data(), static_cast<int>(_config.swayFactorByAtlasIdx.size()));
        program->uniform("swayPeriod", _config.swayPeriodByAtlasIdx.data(), static_cast<int>(_config.swayPeriodByAtlasIdx.size()));
    }
    
#pragma mark - GreeblingParticleSystem
    
    boost::optional<GreeblingParticleSystem::config> GreeblingParticleSystem::config::parse(const util::xml::XmlMultiTree &node) {
        config c;
        c.attachmentBatchId = util::xml::readNumericAttribute<int>(node, "attachmentBatchId", c.attachmentBatchId);
        c.drawLayer = util::xml::readNumericAttribute<int>(node, "drawLayer", c.drawLayer);

        auto atlasPath = node.getAttribute("textureAtlas");
        auto image = loadImage(app::loadAsset(*atlasPath));
        gl::Texture2d::Format fmt = gl::Texture2d::Format().mipmap(false);
        c.textureAtlas = gl::Texture2d::create(image, fmt);
        
        c.atlasType = particles::Atlas::fromString(node.getAttribute("atlasType").value_or("None"));

        // now load <greeble> children until exhausted
        for(size_t i = 0;; i++) {
            auto greeble = node.getChild("greeble", i);
            if (!greeble) {
                break;
            }
            
            greeble_descriptor gd;
            gd.color = util::xml::readColorAttribute(greeble, "color", gd.color);
            gd.radius = util::xml::readNumericAttribute<double>(greeble, "radius", gd.radius);
            gd.upOffset = util::xml::readNumericAttribute<double>(greeble, "upOffset", gd.upOffset);
            gd.swayFactor = util::xml::readNumericAttribute<double>(greeble, "swayFactor", gd.swayFactor);
            gd.swayPeriod = util::xml::readNumericAttribute<double>(greeble, "swayPeriod", gd.swayPeriod);
            gd.probability = util::xml::readNumericAttribute<size_t>(greeble, "probability", gd.probability);
            c.greebleDescriptors.push_back(gd);
        }
        
        if (c.sanityCheck()) {
            return c;
        }
        
        return boost::none;
    }
    
    bool GreeblingParticleSystem::config::sanityCheck() const {
        if (greebleDescriptors.size() != Atlas::ElementCount(atlasType)) {
            CI_ASSERT_MSG(false, "Expect greebleDescriptors.size() to match atlas count");
            return false;
        }
        return true;
    }

    GreeblingParticleSimulation::config GreeblingParticleSystem::config::createSimulationConfig() const {
        GreeblingParticleSimulation::config c;
        for (const auto &gd : greebleDescriptors) {
            c.atlasDetails.push_back(GreeblingParticleSimulation::config::atlas_detail(gd.color, gd.radius, gd.upOffset));
        }
        return c;
    }

    GreeblingParticleSystemDrawComponent::config GreeblingParticleSystem::config::createDrawConfig() const {
        GreeblingParticleSystemDrawComponent::config c;
        c.textureAtlas = textureAtlas;
        c.atlasType = atlasType;
        c.drawLayer = drawLayer;
        
        for (const auto &gd : greebleDescriptors) {
            c.swayFactorByAtlasIdx.push_back(gd.swayFactor);
            c.swayPeriodByAtlasIdx.push_back(gd.swayPeriod);
        }
        
        return c;
    }

    
    GreeblingParticleSystemRef GreeblingParticleSystem::create(string name, const config &c, const vector <terrain::AttachmentRef> &attachments) {
        if (!c.sanityCheck()) {
            CI_LOG_E("GreeblingParticleSystem::create config didn't pass sanity check.");
            return nullptr;
        }

        // create simulation
        auto simulation = make_shared<GreeblingParticleSimulation>(c.createSimulationConfig());
        simulation->setAttachments(attachments);

        // create renderer
        auto draw = GreeblingParticleSystemDrawComponent::create(c.createDrawConfig());
        
        // create system and stitch together
        auto gps = make_shared<GreeblingParticleSystem>(name, c);
        gps->addComponent(draw);
        gps->addComponent(simulation);
        
        return gps;
    }
    
    GreeblingParticleSystem::GreeblingParticleSystem(string name, const config &c) :
            BaseParticleSystem(name),
            _config(c)
    {
    }

}
