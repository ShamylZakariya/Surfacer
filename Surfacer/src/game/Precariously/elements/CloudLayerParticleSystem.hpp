//
//  CloudLayerParticleSystem.hpp
//  Precariously
//
//  Created by Shamyl Zakariya on 10/18/17.
//

#ifndef CloudLayerParticleSystem_hpp
#define CloudLayerParticleSystem_hpp

#include "ParticleSystem.hpp"
#include "PerlinNoise.hpp"

namespace precariously {
	
	SMART_PTR(CloudLayerParticleSimulation);
	SMART_PTR(CloudLayerParticleSystemDrawComponent);
	SMART_PTR(CloudLayerParticleSystem);
	
	class CloudLayerParticleSimulation : public particles::ParticleSimulation {
	public:
		
		struct particle_template {
			double minRadius;
			double maxRadius;
			double minRadiusNoiseValue;
			
			particle_template():
			minRadius(0),
			maxRadius(100),
			minRadiusNoiseValue(0.5)
			{}

			static particle_template parse(const core::util::xml::XmlMultiTree &node);

		};

		
		struct config {
			core::util::PerlinNoise::config generator;
			particle_template particle;
			dvec2 origin;
			double radius;
			core::seconds_t period;
			size_t count;
			
			config():
			origin(0,0),
			radius(500),
			period(1),
			count(100)
			{}
			
			static config parse(const core::util::xml::XmlMultiTree &node);
		};
		
		
	public:
		
		CloudLayerParticleSimulation(const config &c);

		void onReady(core::ObjectRef parent, core::LevelRef level) override;
		void update(const core::time_state &timeState) override;

		bool rotatesParticles() const override { return false; }
		size_t getFirstActive() const override { return 0; };
		size_t getActiveCount() const override { return _storage.size(); }
		cpBB getBB() const override { return _bb; }


	protected:
		
		virtual void simulate();

	private:
	
		config _config;
		core::util::PerlinNoise _generator;
		core::seconds_t _time;
		cpBB _bb;
	};
	
	class CloudLayerParticleSystemDrawComponent : public particles::ParticleSystemDrawComponent {
	public:
		
		struct config : public particles::ParticleSystemDrawComponent::config {
			ci::gl::Texture2dRef textureAtlas;
			particles::ParticleAtlasType::Type atlasType;
			
			config():
			atlasType(particles::ParticleAtlasType::None)
			{}
			
			config(const particles::ParticleSystemDrawComponent::config &c):
			atlasType(particles::ParticleAtlasType::None)
			{}

			static config parse(const core::util::xml::XmlMultiTree &node);
		};
		
	public:
		
		CloudLayerParticleSystemDrawComponent(config c);
		
		// ParticleSystemDrawComponent
		void draw(const core::render_state &renderState) override;
		int getLayer() const override;

	private:
		
		config _config;
		gl::GlslProgRef _shader;
		gl::BatchRef _batch;
		
	};
	
	class CloudLayerParticleSystem : public particles::ParticleSystem {
	public:
		
		struct config {
			CloudLayerParticleSystemDrawComponent::config drawConfig;
			CloudLayerParticleSimulation::config simulationConfig;
			
			static config parse(const core::util::xml::XmlMultiTree &node);
		};
		
		static CloudLayerParticleSystemRef create(const config &c);
		
	public:
		
		CloudLayerParticleSystem(std::string name);
				
	};

	
}

#endif /* CloudLayerParticleSystem_hpp */
