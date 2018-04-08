//
//  BaseParticleSystem.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/18/17.
//

#ifndef BaseParticleSystem_hpp
#define BaseParticleSystem_hpp

#include "Core.hpp"
#include "Xml.hpp"

namespace particles {

    using core::seconds_t;

    SMART_PTR(BaseParticleSimulation);

    SMART_PTR(BaseParticleSystem);

    SMART_PTR(BaseParticleSystemDrawComponent);


    const extern vec2 TexCoords[4];

#pragma mark -

    /**
     particle Atlas Types

     Definition of different particle atlas types
     */
    namespace Atlas {

        enum Type {

            //	No particle atlas, the texture is one big image
                    None = 0,

            /*
             +---+---+
             | 2 | 3 |
             +---+---+
             | 0 | 1 |
             +---+---+
             */
                    TwoByTwo,

            /*
             +---+---+---+---+
             | 12| 13| 14| 15|
             +---+---+---+---+
             | 8 | 9 | 10| 11|
             +---+---+---+---+
             | 4 | 5 | 6 | 7 |
             +---+---+---+---+
             | 0 | 1 | 2 | 3 |
             +---+---+---+---+
             */

                    FourByFour
        };

        extern Type fromString(std::string typeStr);

        extern std::string toString(Type t);

        extern const vec2 *AtlasOffsets(Type atlasType);

        extern float AtlasScaling(Type atlasType);
        
        extern size_t ElementCount(Type atlasType);

    }

#pragma mark - BaseParticleSimulation

    struct particle_state {
        bool active;                // if true particle is active and should be rendered
        dvec2 position;            // base position in world coordinates
        dvec2 right;                // x-axis scaled to half horizontal particle size
        dvec2 up;                    // y-axis scaled to half vertical particle size
        ci::ColorA color;                // color of particle
        double additivity;            // from 0 to 1 where 0 is transparency blending, and 1 is additive blending
        size_t atlasIdx;            // index into the texture atlas

        particle_state() :
                active(false),
                position(0, 0),
                right(0.5, 0),
                up(0, 0.5),
                color(1, 1, 1, 1),
                additivity(0),
                atlasIdx(0) {
        }

        // the default copy-ctor and operator= seem to work fine
    };

    /**
     BaseParticleSimulation
     A BaseParticleSimulation is responsible for updating a fixed-size vector of particle. It does not render.
     Subclasses of BaseParticleSimulation can implement different types of behavior - e.g., one might make
     a physics-modeled simulation using circle shapes/bodies; one might make a traditional one where
     particles are emitted and have some kind of lifespan.
     */
    class BaseParticleSimulation : public core::Component {
    public:

        BaseParticleSimulation();

        // Component
        void onReady(core::ObjectRef parent, core::StageRef stage) override {
        }

        void onCleanup() override {
        }

        void update(const core::time_state &timeState) override {
        }

        // BaseParticleSimulation

        // set the number of particles this simulation will represent
        virtual void setParticleCount(size_t particleCount) {
            _state.resize(particleCount);
        }

        // get the maximum number of particles that might be drawn or simulated.
        // note, to get the number of *active* particles, see getActiveCount()
        size_t getParticleCount() const {
            return _state.size();
        }

        // get the storage vector; note,
        const vector<particle_state> &getParticleState() const {
            return _state;
        }

        // get the offset of the first active particle - a draw method would
        // draw particles from [getFirstActive(), (getFirstActive() + getActiveCount()) % getParticleCount())
        // this is because the storage might be used as a ring buffer
        virtual size_t getFirstActive() const = 0;

        // get the count of active particles, this may be less than getParticleState().size()
        virtual size_t getActiveCount() const = 0;

        // return true iff there are active particles being simulated
        virtual bool isActive() const {
            return getActiveCount() > 0;
        }

        // return true iff there are no active particles being simulated
        bool isEmpty() const {
            return !isActive();
        }

        // return a bounding box containing the entirety of the particles
        virtual cpBB getBB() const = 0;

    protected:

        vector<particle_state> _state;

    };

#pragma mark - BaseParticleSystemDrawComponent

    class BaseParticleSystemDrawComponent : public core::DrawComponent {
    public:

        struct config {

            int drawLayer;

            config() :
                    drawLayer(0) {
            }

            config(const config &other) :
                    drawLayer(other.drawLayer) {
            }

            static config parse(const core::util::xml::XmlMultiTree &node);
        };

    public:

        BaseParticleSystemDrawComponent(config c);

        // DrawComponent
        cpBB getBB() const override;

        void draw(const core::render_state &renderState) override {
        };

        int getLayer() const override;

        core::VisibilityDetermination::style getVisibilityDetermination() const override {
            return core::VisibilityDetermination::FRUSTUM_CULLING;
        };

        // BaseParticleSystemDrawComponent
        const config &getConfig() const {
            return _config;
        }

        config &getConfig() {
            return _config;
        }

        virtual void setSimulation(const BaseParticleSimulationRef simulation) {
            _simulation = simulation;
        }

        BaseParticleSimulationRef getSimulation() const {
            return _simulation.lock();
        }

        template<typename T>
        shared_ptr<T> getSimulation() const {
            return dynamic_pointer_cast<T>(_simulation.lock());
        }

    private:

        BaseParticleSimulationWeakRef _simulation;
        config _config;

    };

#pragma mark - BaseParticleSystem

    class BaseParticleSystem : public core::Object {
    public:

        BaseParticleSystem(std::string name);

        // Object
        void onReady(core::StageRef stage) override;

        void addComponent(core::ComponentRef component) override;

        // BaseParticleSystem
        BaseParticleSimulationRef getSimulation() const {
            return _simulation;
        }

        template<typename T>
        shared_ptr<T> getSimulation() const {
            return dynamic_pointer_cast<T>(_simulation);
        }

    private:

        BaseParticleSimulationRef _simulation;

    };

}

#endif /* BaseParticleSystem_hpp */
