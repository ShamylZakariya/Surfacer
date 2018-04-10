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
            struct atlas_detail {
                
                // color for greebles of this atlas idx
                ci::ColorA color;
                
                // radius for greebles of this atlas idx
                double radius;
                
                // offset in terms of radius to push greebles away from having base touching terrain surface.
                // a value of -0.5 would center greeble's origin on the surface. A value of +0.5 would push greeble up
                // by half its height above the default surface placement.
                double upOffset;
                
                atlas_detail(ci::ColorA color, double radius, double upOffset = 0):
                color(color),
                radius(radius),
                upOffset(upOffset)
                {}
            };
            
            // details to apply to greebles of texture atlas [0...N]
            vector<atlas_detail> atlasDetails;
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
    
    class GreeblingParticleSystemDrawComponent : public particles::ParticleSystemDrawComponent {
    public:
        
        struct config : public particles::ParticleSystemDrawComponent::config {

            // if non-zero, this represents the distance relative to width of the sway of each greeble particle
            // by its atlas index. The size of the vector must == the number of elements implied by the atlas type.
            vector<float> swayFactorByAtlasIdx;
            
            // period in seconds that a particle sway cycle takes
            vector<float> swayPeriodByAtlasIdx;

            config()
            {}
        };

    public:

        static shared_ptr<GreeblingParticleSystemDrawComponent> create(config c);

        GreeblingParticleSystemDrawComponent(config c);
        
    protected:

        void setShaderUniforms(const gl::GlslProgRef &program, const core::render_state &renderState) override;

    protected:
        
        config _config;
    
    };
    
    
    class GreeblingParticleSystem : public particles::BaseParticleSystem {
    public:

        struct greeble_descriptor {
            ci::ColorA color;
            double radius;
            double upOffset;
            double swayFactor;
            double swayPeriod;
            
            greeble_descriptor():
            color(1,1,1,1),
            radius(1),
            upOffset(0),
            swayFactor(0.1),
            swayPeriod(0.5)
            {}
            
            greeble_descriptor(ci::ColorA color, double radius, double upOffset, double swayFactor, double swayPeriod):
            color(color),
            radius(radius),
            upOffset(upOffset),
            swayFactor(swayFactor),
            swayPeriod(swayPeriod)
            {}
        };

        struct config {
            int attachmentBatchId;
            int drawLayer;
            ci::gl::Texture2dRef textureAtlas;
            particles::Atlas::Type atlasType;
            vector<greeble_descriptor> greebleDescriptors;
            
            config():
            attachmentBatchId(0),
            drawLayer(0),
            atlasType(particles::Atlas::None)
            {}
            
            static boost::optional<config> parse(const core::util::xml::XmlMultiTree &node);

            bool sanityCheck() const;
            GreeblingParticleSimulation::config createSimulationConfig() const;
            GreeblingParticleSystemDrawComponent::config createDrawConfig() const;
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
