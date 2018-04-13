//
//  GameStage.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 4/13/17.
//
//

#ifndef PrecariouslyStage_hpp
#define PrecariouslyStage_hpp

#include <cinder/Xml.h>

#include "Core.hpp"

#include "Background.hpp"
#include "Planet.hpp"
#include "CloudLayerParticleSystem.hpp"
#include "ParticleSystem.hpp"

namespace precariously {

    SMART_PTR(PrecariouslyStage);

#pragma mark - PrecariouslyStage

    class PrecariouslyStage : public core::Stage {
    public:
        PrecariouslyStage();

        virtual ~PrecariouslyStage();

        //
        //	Stage
        //

        void addObject(core::ObjectRef obj) override;

        void removeObject(core::ObjectRef obj) override;

        //
        //	PrecariouslyStage
        //

        void load(ci::DataSourceRef stageXmlData);

        BackgroundRef getBackground() const {
            return _background;
        }

        PlanetRef getPlanet() const {
            return _planet;
        }


    protected:

        // Stage
        void onReady() override;

        void update(const core::time_state &time) override;

        bool onCollisionBegin(cpArbiter *arb) override;

        bool onCollisionPreSolve(cpArbiter *arb) override;

        void onCollisionPostSolve(cpArbiter *arb) override;

        void onCollisionSeparate(cpArbiter *arb) override;

        // PrecariouslyStage
        void applySpaceAttributes(XmlTree spaceNode);

        void buildGravity(XmlTree gravityNode);

        void loadBackground(XmlTree planetNode);

        void loadPlanet(XmlTree planetNode);

        CloudLayerParticleSystemRef loadCloudLayer(XmlTree cloudLayer, int drawLayer);
        
        void loadGreebleSystem(const core::util::xml::XmlMultiTree &greebleNode);

        void buildExplosionParticleSystem();

        void buildDustParticleSystem();

        void cullRubble();

        void makeSleepersStatic();

        void performExplosion(dvec2 world);
        
        void handleTerrainTerrainContact(cpArbiter *arbiter);

    private:

        BackgroundRef _background;
        PlanetRef _planet;
        vector <CloudLayerParticleSystemRef> _cloudLayers;
        core::RadialGravitationCalculatorRef _gravity;
        particles::ParticleEmitterRef _explosionEmitter;
        particles::ParticleEmitterRef _dustEmitter;

    };

} // namespace surfacer

#endif /* PrecariouslyStage_hpp */
