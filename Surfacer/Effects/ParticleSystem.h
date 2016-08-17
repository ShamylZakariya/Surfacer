#pragma once

//
//  ParticleSystem.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 7/20/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "Core.h"
#include "ImageProcessing.h"
#include "Range.h"

#include <cinder/Surface.h>
#include <cinder/gl/GlslProg.h>
#include <cinder/gl/Texture.h>
#include <cinder/Rand.h>
#include <cinder/Function.h>

namespace game {

class ParticleSystem;

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
	
}



struct particle {
	ci::Vec2f	position;			// position in world coordinates
	ci::Vec4f	color;				// color of particle
	real		radius;				// particle horizontal radius
	real		xScale;
	real		yScale;
	real		angle;				// rotational angle
	real		additivity;			// from 0 to 1 where 0 is transparency blending, and 1 is additive blending
	std::size_t	idx;				// index for this particle, from 0 to N
	std::size_t	atlasIdx;			// index into the texture atlas

	particle():
		position(0,0),
		color(1,1,1,1),
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
	@class ParticleSystemController
	Base class for controllers of particle motion, lifespan, etc.
*/
class ParticleSystemController 
{
	public:
	
		ParticleSystemController(): _system(NULL) {}
		virtual ~ParticleSystemController(){}
		
		void setParticleSystem( ParticleSystem *system ) { _system = system; }
		ParticleSystem *system() { return _system; }


		/**
			Return the number of particles needed for this Updater
			This is called by the ParticleSystem before rendering or updating.
		*/
		virtual std::size_t count() const = 0;

		/**
			Set initial values for each particle in the system. 
			This is called by the ParticleSystem before rendering or updating.
		*/
		virtual void initialize( std::vector< particle > &particles ){};
				
		/**
			Implementations that update particle::angle should return true for this.
		*/
		virtual bool rotatesParticles() const = 0;
		
		/**
			Update each particle in the ParticleSystem.
			Note: The updater is responsible for updating all particles, and also for
			updating the System's AABB
		*/
		virtual void update( const core::time_state &time, std::vector< particle > &particles ) = 0;
		
		/**
			Return the number of active particles in the system
		*/
		virtual std::size_t activeParticleCount() const = 0;

	protected:
	
		ParticleSystem *_system;
};

class ParticleSystem : public core::GameObject 
{
	public:
	
		struct init : public core::GameObject::init
		{
			std::vector< ci::gl::Texture > textures;
			ci::DataSourceRef vertexShader;
			ci::DataSourceRef fragmentShader;
			ParticleAtlasType::Type atlasType;
			
			init():
				atlasType( ParticleAtlasType::None )
			{}
			
			void addTexture( ci::Surface8u surface, const ci::gl::Texture::Format format = ci::gl::Texture::Format() )
			{
				textures.push_back( ci::gl::Texture( /*util::ip::premultiply*/( surface ), format ));
			}

			void addTexture( ci::gl::Texture tex )
			{
				textures.push_back( tex );
			}
			
		};

	public:
	
		ParticleSystem();
		virtual ~ParticleSystem();
				
		virtual void initialize( const init &initializer );
		const init &initializer() const { return _initializer; }
		
		void setController( ParticleSystemController *controller );
		ParticleSystemController *controller() const { return _controller; }
		
		void update( const core::time_state &time );
		
		const std::vector< particle > &particles() const { return _particles; }
		std::vector< particle > &particles() { return _particles; }
		
		/**
			@return the number of active particles, if non-zero the system is drawing particles
		*/
		inline std::size_t activeParticleCount() const { return _controller->activeParticleCount(); }
		
		inline std::size_t particleCapacity() const { return _particles.size(); }

		/**
			@return true if the system has no particles to render
		*/
		inline bool empty() const { return _controller->activeParticleCount() == 0; }
						
	protected:

		init _initializer;
		ParticleSystemController *_controller;
		std::vector< particle > _particles;

};

class ParticleSystemRenderer : public core::DrawComponent
{
	public:
	
		ParticleSystemRenderer();
		virtual ~ParticleSystemRenderer();
		
		/**
			Called during draw to bind any custom uniforms needed by the shader
		*/
		virtual void bindShaderUniforms(){}
		
		/**
			Load the shader used by this renderer.
		*/

		virtual ci::gl::GlslProg loadShader();
		
		ci::gl::GlslProg &shader()
		{ 
			if ( !_shader ) _shader = loadShader(); 
			return _shader;
		}
		
		void draw( const core::render_state &state );		

	protected:
	
		void _drawGame( const core::render_state &state );
		void _drawDevelopment( const core::render_state &state );
		void _drawDebug( const core::render_state &state );
		void _updateParticles();
			
	protected:
	
		struct particle_vertex {
			ci::Vec2f position;
			ci::Vec2f texCoord;
			ci::Vec4f color;
			
			particle_vertex():
				position(0,0),
				texCoord(0,0),
				color(1,1,1,1)
			{}
			
		};

		std::vector< particle_vertex > _vertices;
		ci::gl::GlslProg _shader;
		
};

#pragma mark -
#pragma mark UniversalParticleSystemController

/**
	@class UniversalParticleSystemController
	
	This is a general-purpose controller, usable for creating common effects like
	dust clouds, fire, sparks, explosions. It's pretty powerful, and if you need
	a really simple effect, it's possibly overkill. See ShapeClusterController for an example of
	a simpler controller tailored for orbiting particles.
	
*/
class UniversalParticleSystemController : public ParticleSystemController
{
	public:

		struct particle_state 
		{
			bool orientsToVelocity;			//	if true, particle is rotated to match its velocity
			
			real velocityScaling;			//	if > 1 the particle diameter is scaled to span the distance
											//	the particle traveled between the previous and current positions.
											//	this looks wonky unless orientsToVelocity is true, in which case
											//	the particle is only scaled along the X axis - making neat looking streaks!
											//	note: this overrides the radius parameter below.

			real lifespan;					//	lifespan in seconds of the particle

			range<real> fade;				//	age to fade in and fade out particles in range 0,1 of age/lifespan
			range<real> damping;			//	damping factor to apply to linear & angular velocity
			range<real> gravity;			//	gravity scalar, negative values cause boyancy
			range<real> radius;				//	radius of particle
			range<real> additivity;			//	additivity blending of particle
			range<real> angularVelocity;	//	angular velocity of particle, if orientsToVelocity == false
			range<Vec4f> color;				//	color of particle
			
			/**
				Functor returning an initial velocity for a particle.
				The idea is, if you have some particles in a system which need to be launched in a particular direction, 
				and others in another direction, you can assign custom initialVelocity functions for each type.
				
				This will be called on a particle only during emit() - and the velocity passed to emi will be added
				to the value returned here.
				
				The default simply returns Vec2r(0,0)
			*/
			std::function< Vec2r() > initialVelocity;
			
			int atlasIdx;					//	the texture atlas index
			


			// these are used by Controller and should not be manually set

			bool _alive;					//	whether the particle is alive. Used by Controller
			real _age;						//	age of the particle. Used by Controller
			Vec2r _linearVelocity;			//	linearVelocity of the particle. Used by Controller
			
			particle_state():
			
				orientsToVelocity(false),
				velocityScaling(0),
				lifespan(1),
				fade(0.1,0.75),
				damping(0.999,0.99),
				gravity(1,1),
				radius(0,1),
				additivity(0,0),
				angularVelocity(0,0),
				color(Vec4f(1,1,1,1),Vec4f(1,1,1,1)),
				initialVelocity( particle_state::defaultInitialVelocity ),
				atlasIdx(0),
				
				_alive(false),
				_age(0),
				_linearVelocity(0,0)
			{}
						
			void clamp()
			{
				fade = ::saturate( fade );
				damping = ::saturate( damping );
				radius = ::max( radius, real(0));
				additivity = ::saturate( additivity );
				color = ::saturate( color );
				lifespan = std::max( lifespan, real(0));
			}
			
			// default initial velocity function for new particles
			static Vec2r defaultInitialVelocity() { return Vec2r(0,0); }
		};
		
		
		struct particle_template
		{
			particle_state particle;
			std::size_t count;
			real mutability;
			
			particle_template( const particle_state &ps, std::size_t c, real m ):
				particle(ps),
				count(c),
				mutability(m)
			{}
			
			particle_state vend() const 
			{
				using namespace ci;

				particle_state particle = this->particle;
				particle._age = 0;
								
				particle.gravity.min *= 1 + Rand::randFloat( -mutability, mutability );
				particle.gravity.max *= 1 + Rand::randFloat( -mutability, mutability );

				particle.fade.min *= 1 + Rand::randFloat( -mutability, mutability );
				particle.fade.max *= 1 + Rand::randFloat( -mutability, mutability );

				particle.radius.min *= 1 + Rand::randFloat( -mutability, mutability );
				particle.radius.max *= 1 + Rand::randFloat( -mutability, mutability );

				particle.additivity.min *= 1 + Rand::randFloat( -mutability, mutability );
				particle.additivity.max *= 1 + Rand::randFloat( -mutability, mutability );

				particle.color.min *= 1 + Rand::randFloat( -mutability, mutability );
				particle.color.max *= 1 + Rand::randFloat( -mutability, mutability );

				particle.angularVelocity.min *= 1 + Rand::randFloat( -mutability, mutability );
				particle.angularVelocity.max *= 1 + Rand::randFloat( -mutability, mutability );

				particle.lifespan *= 1 + Rand::randFloat( -mutability, mutability );

				particle.clamp();
				return particle;
			}
		};
	
		struct init 
		{
			std::vector< particle_template > templates;

			// default particles per second for the default emitter
			std::size_t defaultParticlesPerSecond;
			
			init():
				defaultParticlesPerSecond(24)
			{}
		};
		
		class Emitter {
		
			public:

				/**
					Emit @a count particles at @a position in space, with an added initial @a velocity. 			
				*/
				void emit( std::size_t count, const Vec2r &position, const Vec2r &velocity = Vec2r(0,0) );
				
				/**
					Emit one particle described by @a particle at a given position with a given initialVelocity
				*/
				void emit( const particle_state &particle, const Vec2r &position, const Vec2r &velocity = Vec2r(0,0) );
				
				void setParticlesPerSecond( std::size_t pps );
				void disableThrottling() { setParticlesPerSecond( INT_MAX ); }
				std::size_t particlesPerSecond() const { return _targetParticlesPerSecond; }
				real currentParticlesPerSecond() const { return _currentParticlesPerSecond; }
				UniversalParticleSystemController *controller() const { return _controller; }
				ParticleSystem *system() const { return _controller->system(); }
				
				// return the number of particles which are pending to be emitted
				std::size_t pendingParticleCount() const { return _emission.size(); }
				
				void setNoisy( bool noisy ) { _noisy = noisy; }
				bool noisy() const { return _noisy; }
				
			
			private:
			
				friend class UniversalParticleSystemController;
			
				Emitter( UniversalParticleSystemController *controller, std::size_t pps );
				void _update( const core::time_state &time );

			private:

				UniversalParticleSystemController *_controller;
				bool _throttled, _noisy;
				std::size_t _targetParticlesPerSecond, _particlesEmittedSinceLastTally;
				real _particlesPerStep, _particlesToEmitThisStep;
				real _currentParticlesPerSecond;
				seconds_t _lastTallyTime;
				
				struct emission_state {
					particle_state state;
					Vec2r position, velocity;
					
					emission_state( const particle_state &s, const Vec2r &p, const Vec2r &v ):
						state( s ),
						position( p ),
						velocity( v ) 
					{}
					
				};
				
				std::vector< emission_state > _emission;		
		};

	public:
	
		UniversalParticleSystemController();		
		virtual ~UniversalParticleSystemController();

		/**
			Initialize this controller with your parameter struct. 
			This must be called before attaching to a particle system.
			This initialize() method corresponds to use of:
				void emit( std::size_t count, const Vec2r &position, const Vec2r &velocity, real mutation );
		*/
		void initialize( const init &init );
		
		/**
			Alternate initialization. 
			Tells the controller we'll be using a max of @a particleCount particles
		*/
		void initialize( std::size_t particleCount );
		
		/**
			Get the default emitter for this UniversalParticleSystemController
		*/
		Emitter *defaultEmitter() const { return _emitters.front(); }
		
		/**
			create a new emitter.
			The Emitter is owned by this controller, and will be deleted with this controller.
		*/
		Emitter *emitter( std::size_t particlesPerSecond = INT_MAX );
		
		std::size_t count() const { return _states.size(); }
		std::size_t activeParticleCount() const { return _activeParticleCount; }
		bool rotatesParticles() const { return true; }
		void update( const core::time_state &time, std::vector< particle > &particles );
						
	private:
	
		friend class Emitter;
	
		// returns true for alive particles
		struct particle_alive_test
		{
			const std::vector< particle_state > &_states;

			particle_alive_test( const std::vector< particle_state > &states ):
				_states(states)
			{}
		
			inline bool operator()( const particle &p ) const
			{
				return _states[p.idx]._alive;
			}
		};

		/**
			Emit one particle described by @a particle at a given position with a given initialVelocity
			@return true if the particle could be emitted, false if the system has no space for more particles
		*/
		bool _emit( const particle_state &particle, const Vec2r &position, const Vec2r &velocity );

		/**
			Move dead particles to end of storage, and return how many active particles there are
		*/
		std::size_t _pack( std::vector< particle > &particles )
		{	
			_activeParticleCount = std::partition( particles.begin(), particles.end(), particle_alive_test( _states ) ) - particles.begin();
			return _activeParticleCount;
		}
	
	private:
	
		std::size_t _activeParticleCount;
		std::vector< particle_state > _templates, _states;
		std::vector< Emitter * > _emitters;
};


} // end namespace game