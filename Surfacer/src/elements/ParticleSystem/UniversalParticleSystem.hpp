//
//  UniversalParticleSystem.hpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 10/23/17.
//

#ifndef UniversalParticleSystem_hpp
#define UniversalParticleSystem_hpp

#include "ParticleSystem.hpp"

namespace particles {
	
	SMART_PTR(UniversalParticleSystem);
	SMART_PTR(UniversalParticleSimulation);
	SMART_PTR(UniversalParticleSystemDrawComponent);
	
	using core::seconds_t;

	/**
	 Interpolator
	 A keyframe-type interpolation but where the values are evenly spaced.
	 If you provide the values, [0,1,0], you'd get a mapping like so:
	 0.00 -> 0
	 0.25 -> 0.5
	 0.50 -> 1
	 0.75 -> 0.5
	 1.00 -> 0
	 */
	template<class T>
	struct interpolator {
	public:
		
		interpolator()
		{}
		
		interpolator(const initializer_list<T> values):
		_values(values)
		{
			CI_ASSERT_MSG(!_values.empty(), "Values array must not be empty");
		}
		
		interpolator(const interpolator<T> &copy):
		_values(copy._values)
		{
			CI_ASSERT_MSG(!_values.empty(), "Values array must not be empty");
		}
		
		interpolator &operator = (const interpolator<T> &rhs) {
			_values = rhs._values;
			return *this;
		}
		
		interpolator &operator = (const initializer_list<T> values) {
			_values = values;
			CI_ASSERT_MSG(!_values.empty(), "Values array must not be empty");
			return *this;
		}
		
		interpolator &operator = (const T &value) {
			_values.clear();
			_values.push_back(value);
			return *this;
		}
		
		// get the value equivalent to *this(0)
		T getInitialValue() const {
			return _values.front();
		}
		
		// get the value equivalent to *this(1)
		T getFinalValue() const {
			return _values.back();
		}
		
		// get the interpolated value for a given time `v` from [0 to 1]
		T operator()(double v) const {
			if (_values.size() == 1) {
				return _values.front();
			}
			
			if (v <= 0) {
				return _values.front();
			} else if (v >= 1) {
				return _values.back();
			}
			
			size_t s = _values.size() - 1;
			double dist = v * s;
			double distFloor = floor(dist);
			double distCeil = distFloor + 1;
			size_t aIdx = static_cast<size_t>(distFloor);
			size_t bIdx = aIdx + 1;
			
			T a = _values[aIdx];
			T b = _values[bIdx];
			double factor = (dist - distFloor) / (distCeil - distFloor);
			
			return a + (factor * (b - a));
		}
		
	private:
		
		vector<T> _values;
		
	};
	
	class UniversalParticleSimulation : public ParticleSimulation {
	public:
		
		struct particle_kinematics_template {
			bool isKinematic;
			double friction;
			cpShapeFilter filter;
			
			particle_kinematics_template():
			isKinematic(false),
			friction(1),
			filter(CP_SHAPE_FILTER_ALL)
			{}
			
			particle_kinematics_template(double friction, cpShapeFilter filter):
			isKinematic(true),
			friction(friction),
			filter(filter)
			{}
			
			operator bool() const {
				return isKinematic == true;
			}
		};
		
		struct particle_template {
		public:
			
			// index into texture atlas for this particle
			int atlasIdx;
			
			// lifespan in seconds of the particle
			seconds_t lifespan;
			
			// radius interpolator
			interpolator<double> radius;
			
			// damping interpolator. damping value > 0 subtracts that amount from velocity per timestamp
			interpolator<double> damping;
			
			// additivity interpolator
			interpolator<double> additivity;
			
			// mass interpolator. If non-zero particles are affected by gravity.
			// values > 0 are attracted to gravity wells and values < 0 are repelled.
			interpolator<double> mass;
			
			// color interpolator
			interpolator<ci::ColorA> color;
			
			// initial position of particle. as particle is simulated position will change
			dvec2 position;
			
			// initial velocity to apply to particle. particle motion will be ballistic after this
			dvec2 velocity;
			
			// if true, particle is rotated such that the X axis aligns with the direction of velocity
			bool orientToVelocity;
			
			particle_kinematics_template kinematics;
			
		private:
			
			friend class UniversalParticleSimulation;

			cpShape* _shape;
			cpBody* _body;
			double _completion;
			seconds_t _age;

			void destroy() {
				if (_shape) {
					cpCleanupAndFree(_shape);
					_shape = nullptr;
				}
				if (_body) {
					cpCleanupAndFree(_body);
					_body = nullptr;
				}
			}
			
			void prepare() {
				_age = 0;
				_completion = 0;
			}

		public:
			
			particle_template():
			atlasIdx(0),
			lifespan(0),
			position(0,0),
			velocity(0,0),
			orientToVelocity(false),
			_shape(nullptr),
			_body(nullptr),
			_completion(0),
			_age(0)
			{}			
		};
		
	public:
		
		UniversalParticleSimulation();
		
		// Component
		void onReady(core::ObjectRef parent, core::StageRef stage) override;
		void onCleanup() override;
		void update(const core::time_state &time) override;

		// ParticleSimulation
		void setParticleCount(size_t count) override;
		size_t getFirstActive() const override;
		size_t getActiveCount() const override;
		cpBB getBB() const override;

		// UniversalParticleSimulation
		
		// emit a single particle
		void emit(const particle_template &particle);
		
	protected:
		
		virtual void _prepareForSimulation(const core::time_state &time);
		virtual void _simulate(const core::time_state &time);
		
	protected:
		
		size_t _count;
		cpBB _bb;
		vector<particle_template> _templates, _pending;
		core::SpaceAccessRef _spaceAccess;
		
	};
	
	class UniversalParticleSystemDrawComponent : public particles::ParticleSystemDrawComponent {
	public:
		
		struct config : public particles::ParticleSystemDrawComponent::config {
			ci::gl::Texture2dRef textureAtlas;
			particles::ParticleAtlasType::Type atlasType;

			config():
			atlasType(particles::ParticleAtlasType::None)
			{}
			
			config(const particles::ParticleSystemDrawComponent::config &c):
			particles::ParticleSystemDrawComponent::config(c),
			atlasType(particles::ParticleAtlasType::None)
			{}
			
			static config parse(const core::util::xml::XmlMultiTree &node);
		};
		
	public:
		
		UniversalParticleSystemDrawComponent(config c);
		
		// ParticleSystemDrawComponent
		void setSimulation(const particles::ParticleSimulationRef simulation) override;
		void draw(const core::render_state &renderState) override;
		
	protected:
		
		// update _particles store for submitting to GPU - return true iff there are particles to draw
		virtual bool updateParticles(const ParticleSimulationRef &sim);
		
		struct particle_vertex {
			vec2 position;
			vec2 texCoord;
			ci::ColorA color;
		};
		
		config _config;
		gl::GlslProgRef _shader;
		vector<particle_vertex> _particles;
		gl::VboRef _particlesVbo;
		gl::BatchRef _particlesBatch;
		GLsizei _batchDrawStart, _batchDrawCount;
		
	};
	
	class UniversalParticleSystem : public particles::ParticleSystem {
	public:
		
		struct config {
			size_t maxParticleCount;
			particles::UniversalParticleSystemDrawComponent::config drawConfig;
			
			config():
			maxParticleCount(500)
			{}
			
			static config parse(const core::util::xml::XmlMultiTree &node);
		};
		
		static UniversalParticleSystemRef create(std::string name, const config &c);
		
	public:
		
		// do not call this, call ::create
		UniversalParticleSystem(std::string name);
		
	};
	
	
}

#endif /* UniversalParticleSystem_hpp */
