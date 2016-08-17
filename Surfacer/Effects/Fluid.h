#pragma once

//
//  Fluid.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/22/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "Core.h"
#include "GameConstants.h"

#include <cinder/Surface.h>
#include <cinder/gl/gl.h>
#include <cinder/gl/Texture.h>
#include <cinder/gl/GlslProg.h>
#include <cinder/gl/Fbo.h>


namespace game {

class FluidRendererModel : public core::util::JsonInitializable
{
	public:
	
		struct init : public core::GameObject::init
		{
			real fluidToneMapLumaPower;
			real fluidToneMapHue;
			real fluidOpacity;
			real fluidShininess;
			ci::ColorA fluidHighlightColor;
			real fluidHighlightLookupDistance;

			ci::fs::path 
				fluidParticleTexture,
				fluidToneMapTexture,
				fluidBackgroundTexture;
				
			init():
				fluidToneMapLumaPower(1),
				fluidToneMapHue(0),
				fluidOpacity(1),
				fluidShininess(0),
				fluidHighlightColor(0,0,0,0),
				fluidHighlightLookupDistance(1)
			{}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				core::GameObject::init::initialize(v);
				JSON_READ(v,fluidToneMapLumaPower);
				JSON_READ(v,fluidToneMapHue);
				JSON_READ(v,fluidOpacity);
				JSON_READ(v,fluidShininess);
				JSON_READ(v,fluidHighlightColor);
				JSON_READ(v,fluidHighlightLookupDistance);
				JSON_READ(v,fluidParticleTexture);
				JSON_READ(v,fluidToneMapTexture);
				JSON_READ(v,fluidBackgroundTexture);
			}
			
			
		};
	
		struct fluid_particle
		{
			ci::Vec2f position;
			float physicsRadius, drawRadius, angle, opacity;
			
			fluid_particle():
				position(0,0),
				physicsRadius(1),
				drawRadius(1),
				angle(0),
				opacity(1)
			{}
			
		};
		
		typedef std::vector< fluid_particle > fluid_particle_vec;

	public:
	
		FluidRendererModel():
			_toneMapLumaPower(1),
			_toneMapHue(0),
			_opacity(1),
			_highlightColor(0,0,0,0),
			_highlightLookupDistance(0)
		{}

		virtual ~FluidRendererModel(){}
				
		JSON_INITIALIZABLE_INITIALIZE();

		virtual void initialize( const init & );
		
		virtual const fluid_particle_vec &fluidParticles() const = 0;
		
		virtual void setFluidParticleTexture( const ci::fs::path &t ) { _particleTexture = t; }
		ci::fs::path fluidParticleTexture() const { return _particleTexture; }
		
		virtual void setFluidToneMap( const ci::fs::path &t ) { _toneMapTexture = t; }
		ci::fs::path fluidToneMap() const { return _toneMapTexture; }
		
		virtual void setFluidBackground( const ci::fs::path &t ) { _backgroundTexture = t; }
		ci::fs::path fluidBackground() const { return _backgroundTexture; }
		
		virtual void setFluidToneMapLumaPower( float p ) { _toneMapLumaPower = p; }
		float fluidToneMapLumaPower() const { return _toneMapLumaPower; }

		virtual void setFluidToneMapHue( float hue ) { _toneMapHue = hue; }
		float fluidToneMapHue() const { return _toneMapHue; }
		
		virtual void setFluidOpacity( float op ) { _opacity = saturate<float>(op); }
		float fluidOpacity() const { return _opacity; }

		virtual void setFluidHighlightColor( const Vec4f &hc ) { _highlightColor = hc; }
		Vec4f fluidHighlightColor() const { return _highlightColor; }
		
		virtual void setFluidHighlightLookupDistance( float dist ) { _highlightLookupDistance = dist; }
		float fluidHighlightLookupDistance() const { return _highlightLookupDistance; }

		virtual void setFluidShininess( float s ) { _shininess = saturate(s); }
		float fluidShininess() const { return _shininess; }

	private:
	
		ci::fs::path _particleTexture, _toneMapTexture, _backgroundTexture;
		float _toneMapLumaPower, _toneMapHue, _opacity, _shininess, _highlightLookupDistance;
		Vec4f _highlightColor;
		
};

class FluidRenderer : public core::DrawComponent 
{
	public:
	
		FluidRenderer();
		virtual ~FluidRenderer();
		
		void setModel( FluidRendererModel *m ) { _model = m; }
		FluidRendererModel *model() const { return _model; }
		
	protected:
	
		struct particle_vertex {
			ci::Vec2f position, texCoord;
			float opacity;
		};
		
		typedef std::vector< particle_vertex > particle_vertex_vec;
	
		void _drawGame( const core::render_state &state );
		void _drawDebug( const core::render_state &state );
		
		void _drawParticles( const core::render_state &state );
		
		
	private:
	
		FluidRendererModel *_model;
		particle_vertex_vec _vertices;
		ci::gl::GlslProg _shader, _compositingShader;
		ci::gl::Fbo _compositingFbo;
		ci::gl::Texture _particleTexture, _toneMapTexture, _backgroundTexture;
		GLint _opacityAttrLoc;

};


class Fluid : public core::GameObject, public FluidRendererModel
{
	public:
	
		struct physics_particle {
			cpShape *shape;
			cpBody *body;
			real radius, currentRadius, visualRadiusScale, linearDamping, angularDamping;
			seconds_t lifespan, age, entranceDuration, exitDuration;
			
			real radiusForCurrentAge() const 
			{
				if ( age < entranceDuration )
				{
					const real minRadius = radius * 0.5;
					if ( entranceDuration > 0 ) return std::max<real>( radius * ( age / entranceDuration ), minRadius );
					return radius;
				}
				else if ( age > lifespan - exitDuration )
				{
					const real minRadius = radius * 0.01;
					real scale = 1 - saturate((age - ( lifespan - exitDuration )) / exitDuration);
					return std::max( radius * scale, minRadius );
				}
				
				return radius;
			}
		};
		
		struct particle_init : public core::util::JsonInitializable {
			Vec2r position, initialVelocity;
			real radius, visualRadiusScale, density, linearDamping, angularDamping, friction, elasticity;
			seconds_t lifespan, entranceDuration, exitDuration;
			
			particle_init():
				position(0,0),
				initialVelocity(0,0),
				radius(0.5),
				visualRadiusScale(2),
				density(1),
				linearDamping(1),
				angularDamping(1),
				friction(0.01),
				elasticity(0),
				lifespan( INFINITY ),
				entranceDuration(1),
				exitDuration(2)
			{}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				using core::util::read;
				JSON_READ(v,position);
				JSON_READ(v,initialVelocity);
				JSON_READ(v,radius);
				JSON_READ(v,visualRadiusScale);
				JSON_READ(v,density);
				JSON_READ(v,linearDamping);
				JSON_READ(v,angularDamping);
				JSON_READ(v,friction);
				JSON_READ(v,elasticity);
				JSON_READ(v,lifespan);
				JSON_READ(v,entranceDuration);
				JSON_READ(v,exitDuration);
			}

							
		};
		
		struct init : public FluidRendererModel::init
		{
			real attackStrength;				// injury of health units per second
			InjuryType::type injuryType;		// type of injury this fluid causes

			real clumpingForce;

			particle_init particle;				// particle parameters to use if boundingVolumeRadius > 0
			Vec2r boundingVolumePosition;		// position of bounding circle to fill with particles, if boundingVolumeRadius > 0
			real boundingVolumeRadius;			// radius of bounding volume to fill. if > 0, the Fluid will automatically fill it
			
			init():
				attackStrength(0),
				injuryType( InjuryType::NONE ),
				clumpingForce(0),
				boundingVolumePosition(0,0),
				boundingVolumeRadius(0)
			{}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				FluidRendererModel::init::initialize(v);
				
				JSON_READ(v, attackStrength);
				JSON_ENUM_READ(v,InjuryType,injuryType);
				JSON_READ(v, clumpingForce);
				JSON_READ(v, boundingVolumePosition);
				JSON_READ(v, boundingVolumeRadius);
				JSON_READ(v, particle);
			}
		};
		
		typedef std::vector< physics_particle > physics_particle_vec;

	public:
	
		Fluid();
		virtual ~Fluid();

		JSON_INITIALIZABLE_INITIALIZE();

		// FLuid
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
		
		virtual void addedToLevel( core::Level *level );
		virtual void ready();
		virtual void update( const core::time_state &time );
				
		real attackStrength() const { return _initializer.attackStrength; }
		InjuryType::type injuryType() const { return _initializer.injuryType; }
		
		bool sleeping() const { return _sleeping; }

		std::size_t numParticles() const { return _physicsParticles.size(); }
		const fluid_particle_vec &fluidParticles() const { return _fluidParticles; }
		const physics_particle_vec &physicsParticles() const { return _physicsParticles; }
		

		/**
			emit one fluid particle
		*/
		void emit( const particle_init &particleInitializer );
		
		/**
			emit fluid particles filling a circular volume
		*/
		void emitCircularVolume( const particle_init &particleInitializer, const Vec2r &volumePosition, real volumeRadius );

		/**
			destroy all particles
		*/
		void clear();

	protected:
			
		void _updateLifecycle( const core::time_state &time, bool &wakeupNeeded );
		void _clump( const core::time_state &time );

	private:
	
		bool _sleeping;
		cpSpace *_space;
		init _initializer;
		physics_particle_vec _physicsParticles;
		fluid_particle_vec _fluidParticles;
};



} // end namespace game
