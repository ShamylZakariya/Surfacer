//
//  ParticleSystem.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/18/17.
//

#ifndef ParticleSystem_hpp
#define ParticleSystem_hpp

#include "Core.hpp"
#include "Xml.hpp"

namespace particles {
	
	SMART_PTR(ParticleSimulation);
	SMART_PTR(ParticleSystem);
	SMART_PTR(ParticleSystemDrawComponent);
	
#pragma mark -
		
	const extern vec2 TexCoords[4];
		
#pragma mark -
	
	/**
	 particle Atlas Types
	 
	 Definition of different particle atlas types
	 */
	namespace ParticleAtlasType {
		
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
		
		extern const vec2* AtlasOffsets(Type atlasType);
		extern float AtlasScaling(Type atlasType);
		
	}
	
	
	
	struct particle_state {
		dvec2		home;				// initial position of particle, emission point or "default" location
		dvec2		position;			// base position in world coordinates
		dvec2		velocity;			// velocity of particle in world space
		ci::ColorA	color;				// color of particle
		double		damping;			// damping of particle's velocity per timestep. 0 is no dsamping, 1 is completely zero velocity
		double		radius;				// particle horizontal radius
		double		xScale;
		double		yScale;
		double		angle;				// rotational angle
		double		additivity;			// from 0 to 1 where 0 is transparency blending, and 1 is additive blending
		size_t		idx;				// index for this particle, from 0 to N in terms of ParticleSimulation's storage
		size_t		atlasIdx;			// index into the texture atlas
		
		particle_state():
		home(0,0),
		position(0,0),
		velocity(0,0),
		color(1,1,1,1),
		damping(0),
		radius(0),
		xScale(1),
		yScale(1),
		angle(0),
		additivity(0),
		idx(0),
		atlasIdx(0)
		{}
		
		// the default copy-ctor and operator= seem to work fine
	};
	
	/**
	 ParticleSimulation
	 A ParticleSimulation is responsible for updating a fixed-size vector of particle. It does not render.
	 Subclasses of ParticleSimulation can implement different types of behavior - e.g., one might make
	 a physics-modeled simulation using circle shapes/bodies; one might make a traditional one where
	 particles are emitted and have some kind of lifespan.
	 */
	class ParticleSimulation : public core::Component {
	public:
		
		ParticleSimulation();
		
		// Component
		
		// update particle system state
		void onReady(core::ObjectRef parent, core::LevelRef level) override {}
		void onCleanup() override {}
		void update(const core::time_state &timeState) override {}
		
		// ParticleSimulation
		
		// return true if the simulation rotates particles
		virtual bool rotatesParticles() const = 0;
		
		// initialize simulation to have count particles of template particle p
		void initialize(size_t count, particle_state p = particle_state()) {
			_storage = vector<particle_state>(count, p);
		}
		
		// initialize simulation with an example set
		void initialize(const vector<particle_state> &p) {
			_storage = p;
		}

		// get the storage vector; note,
		const vector<particle_state> &getStorage() const { return _storage; }
		
		// get the offset of the first active particle - a draw method would
		// draw particles from [getFirstActive(), (getFirstActive() + getActiveCount()) % getStorageSize())
		// this is because the storage might be used as a ring buffer
		virtual size_t getFirstActive() const = 0;
		
		// get the count of active particles, this may be less than getStorage().size()
		virtual size_t getActiveCount() const = 0;

		// get the size of the particle state storage
		size_t getStorageSize() const { return _storage.size(); }
		
		// return a bounding box containing the entirety of the particles
		virtual cpBB getBB() const = 0;
		
	protected:
		
		vector<particle_state> _storage;

	};
	
	class ParticleSystemDrawComponent : public core::DrawComponent {
	public:
		
		struct config {
			
			// PLACEHOLDER
			
			config()
			{}
			
			static config parse(const core::util::xml::XmlMultiTree &node);
		};
		
	public:
		
		ParticleSystemDrawComponent(config c);

		// DrawComponent
		cpBB getBB() const override;
		void draw(const core::render_state &renderState) override {};
		core::VisibilityDetermination::style getVisibilityDetermination() const override { return core::VisibilityDetermination::FRUSTUM_CULLING; };

		// ParticleSystemDrawComponent
		const config &getConfig() const { return _config; }
		virtual void setSimulation(const ParticleSimulationRef simulation) { _simulation = simulation; }
		ParticleSimulationRef getSimulation() const { return _simulation.lock(); }

		template<typename T>
		shared_ptr<T> getSimulation() const { return dynamic_pointer_cast<T>(_simulation.lock()); }

	private:
		
		ParticleSimulationWeakRef _simulation;
		config _config;
		
	};
	
	class ParticleSystem : public core::Object {
	public:
		
		ParticleSystem(std::string name);
		
		// Object
		void onReady(core::LevelRef level) override;
		void addComponent(core::ComponentRef component) override;
		
		// ParticleSystem
		ParticleSimulationRef getSimulation() const { return _simulation; }
		
		template<typename T>
		shared_ptr<T> getSimulation() const { return dynamic_pointer_cast<T>(_simulation); }

	private:
		
		ParticleSimulationRef _simulation;
		
	};
	
}

#endif /* ParticleSystem_hpp */
