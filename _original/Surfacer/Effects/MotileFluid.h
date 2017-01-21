#pragma once

//
//  MotileFluid.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 1/24/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Fluid.h"

namespace game {

class MotileFluid : public core::GameObject, public FluidRendererModel
{
	public:
	
		enum fluid_type {
		
			// creates a gelatinous, soft-body round, bounded shoggoth
			PROTOPLASMIC_TYPE,
			
			// creates a sloppy amorphous sack of balls shoggoth
			AMORPHOUS_TYPE
		
		};
	
		struct init : public FluidRendererModel::init
		{
			fluid_type type;

			// set both introTime and extroTime > 0 to automatically manage lifecycle.
			// if both are zero, you're responsible for managing lifecycle manually via MotileFluid::setLifecycle
			seconds_t introTime, extroTime;

			Vec2 position;
			real radius; 
			real stiffness;
			real particleDensity;
			int numParticles;
			
			real pulsePeriod;
			real pulseMagnitude;
			real particleScale;			
			
			cpCollisionType collisionType;
			cpLayers collisionLayers;
			real friction;
			real elasticity;
			
			GameObject *userData;
			
			init():
				type( PROTOPLASMIC_TYPE ),
				introTime(1),
				extroTime(1),
				position(0,0),
				radius(3),
				stiffness(0.5),
				particleDensity(1),
				numParticles(24),
				pulsePeriod(4),
				pulseMagnitude(0.125),
				particleScale(2),
				collisionType(0),
				collisionLayers(CP_ALL_LAYERS),
				friction(1),
				elasticity(0.0),
				userData(NULL)
			{
				// FluidRendererModel::init
				fluidToneMapLumaPower = 0.5;
			}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				FluidRendererModel::init::initialize(v);
				
				JSON_CAST_READ(v, fluid_type, int, type );
				JSON_READ(v,introTime);
				JSON_READ(v,extroTime);
				JSON_READ(v,position);
				JSON_READ(v,radius);
				JSON_READ(v,stiffness);
				JSON_READ(v,particleDensity);
				JSON_READ(v,numParticles);
				JSON_READ(v,pulsePeriod);
				JSON_READ(v,pulseMagnitude);
				JSON_READ(v,particleScale);
				JSON_CAST_READ(v,cpCollisionType,std::size_t,collisionType);
				JSON_CAST_READ(v,cpLayers,std::size_t,collisionLayers);
				JSON_READ(v,friction);
				JSON_READ(v,elasticity);				
			}

						
		};

		struct physics_particle
		{
			real radius, slideConstraintLength, scale;
			cpBody *body;
			cpShape *shape;
			cpConstraint *slideConstraint, *springConstraint, *motorConstraint;
			Vec2 offsetPosition;
			
			physics_particle():
				radius(0),
				slideConstraintLength(0),
				scale(1),
				body(NULL),
				shape(NULL),
				slideConstraint(NULL),
				springConstraint(NULL),
				motorConstraint(NULL)
			{}		
		};
		
		typedef std::vector< physics_particle > physics_particle_vec;

	public:
	
		MotileFluid( int gameObjectType );
		virtual ~MotileFluid();

		JSON_INITIALIZABLE_INITIALIZE();
						
		// Initialization
		virtual void initialize( const init &i );
		const init &initializer() const { return _initializer; }

		// GameObject
		virtual void setFinished( bool f );
		virtual bool finished() const;
		virtual void addedToLevel( core::Level *level );
		virtual void update( const core::time_state & );

		// FluidRendererModel
		virtual const fluid_particle_vec &fluidParticles() const { return _fluidParticles; }
		
		// MotileFluid
		void setSpeed( real mps ) { _speed = mps; }
		real speed() const { return _speed; }

		void setLifecycle( real ls ) { _lifecycle = saturate(ls); }
		real lifecycle() const { return _lifecycle; }
		
		fluid_type fluidType() const { return _initializer.type; }
		physics_particle_vec &physicsParticles() { return _physicsParticles; }
		const physics_particle_vec &physicsParticles() const { return _physicsParticles; }
		
				
		cpBody *centralBody() const { return _centralBody; }		
		const cpShapeSet &fluidShapes() const { return _fluidShapes; };
		const cpBodySet &fluidBodies() const { return _fluidBodies; }
		const cpConstraintSet &fluidConstraints() const { return _fluidConstraints; }
		const cpConstraintSet &motorConstraints() const { return _motorConstraints; }
			
	private:
	
		void _buildProtoplasmic();		
		void _updateProtoplasmic( const core::time_state &time );

		void _buildAmorphous();
		void _updateAmorphous( const core::time_state &time );
						
	private:

		cpSpace *_space;
		cpBody *_centralBody;
		cpConstraint *_centralBodyConstraint;
		
		cpShapeSet _fluidShapes;
		cpBodySet _fluidBodies;		
		cpConstraintSet _fluidConstraints, _motorConstraints, _perimeterConstraints;
		
		init _initializer;
		physics_particle_vec _physicsParticles;
		fluid_particle_vec _fluidParticles;
		real _speed, _lifecycle, _springStiffness, _bodyParticleRadius;
		seconds_t _finishedTime;
		
};

class MotileFluidDebugRenderer : public core::DrawComponent
{
	public:
	
		MotileFluidDebugRenderer();
		virtual ~MotileFluidDebugRenderer();

		MotileFluid *motileFluid() const { return (MotileFluid*) owner(); }
		
	protected:

		virtual void _drawDebug( const core::render_state & );

};


} // end namespace game
