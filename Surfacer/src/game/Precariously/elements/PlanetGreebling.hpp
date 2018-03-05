//
//  PlanetGreebling.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 3/2/18.
//

#ifndef PlanetGreebling_hpp
#define PlanetGreebling_hpp

#include "Planet.hpp"
#include "ParticleSystem.hpp"
#include "PrecariouslyConstants.hpp"

namespace precariously {
    
    SMART_PTR(GreeblingParticleSimulation);
    SMART_PTR(GreeblingParticleSystem);

    class GreeblingParticleSimulation : public particles::BaseParticleSimulation {
    public:
        
        struct config {
            // nothing, yet!
            
            static config parse(const core::util::xml::XmlMultiTree &node);
        };
        
    public:

        GreeblingParticleSimulation(const config &c);
        
        void onReady(core::ObjectRef parent, core::StageRef stage) override;
        
        void update(const core::time_state &timeState) override;
        
        void setParticleCount(size_t count) override;
        
        size_t getFirstActive() const override {
            return 0;
        };
        
        size_t getActiveCount() const override {
            return _state.size();
        }
        
        cpBB getBB() const override {
            return _bb;
        }
        
        // GreeblingParticleSimulation
        void setAttachments(const vector <terrain::AttachmentRef> &attachments);
        const vector <terrain::AttachmentRef> &getAttachments() const { return _attachments; }

    protected:
        
        void simulate(const core::time_state &time);

    protected:

        config _config;
        cpBB _bb;
        bool _firstSimulate;
        vector <terrain::AttachmentRef> _attachments;
        
    };
    
    
    class GreeblingParticleSystem : public particles::BaseParticleSystem {
    public:
        
        struct config {
            GreeblingParticleSimulation::config simulationConfig;
            particles::ParticleSystemDrawComponent::config drawConfig;
            
            config()
            {
            }
            
            static config parse(const core::util::xml::XmlMultiTree &node);
        };
        
        static GreeblingParticleSystemRef create(std::string name, const config &c, const vector <terrain::AttachmentRef> &attachments);
        
    public:
        
        // do not call this, call ::create
        GreeblingParticleSystem(std::string name, const config &c);
        
    private:
        
        config _config;
        
    };
    
}

#endif /* PlanetGreebling_hpp */
