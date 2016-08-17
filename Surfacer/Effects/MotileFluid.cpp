//
//  MotileFluid.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 1/24/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "MotileFluid.h"
#include <chipmunk/chipmunk_unsafe.h>


using namespace ci;
using namespace core;
namespace game {

/*
		cpSpace *_space;
		cpBody *_centralBody;
		cpConstraint *_centralBodyConstraint;
		
		cpShapeSet _fluidShapes;
		cpBodySet _fluidBodies;		
		cpConstraintSet _perimeterConstraints;
		
		init _initializer;
		physics_particle_vec _physicsParticles;
		fluid_particle_vec _fluidParticles;
		real _speed, _lifecycle, _springStiffness, _bodyParticleRadius;
		seconds_t _finishedTime;
*/

MotileFluid::MotileFluid( int gameObjectType ):
	GameObject( gameObjectType ),
	_space(NULL),
	_centralBody(NULL),
	_centralBodyConstraint(NULL),
	_speed(0),
	_lifecycle(0),
	_springStiffness(0),
	_bodyParticleRadius(0),
	_finishedTime(0)
{
	setName( "MotileFluid" );
	setLayer( RenderLayer::EFFECTS );
	setVisibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING );

	FluidRenderer *renderer = new FluidRenderer();
	renderer->setModel( this );
	addComponent( renderer );	
	
	addComponent( new MotileFluidDebugRenderer() );
}

MotileFluid::~MotileFluid()
{
	aboutToDestroyPhysics(this);

	foreach( cpConstraint *c, _perimeterConstraints )
	{
		cpSpaceRemoveConstraint( _space, c );
		cpConstraintFree( c );
	}

	foreach( physics_particle &particle, _physicsParticles )
	{
		if ( particle.springConstraint )
		{
			cpSpaceRemoveConstraint( _space, particle.springConstraint );
			cpConstraintFree( particle.springConstraint );
		}

		if ( particle.slideConstraint )
		{
			cpSpaceRemoveConstraint( _space, particle.slideConstraint );
			cpConstraintFree( particle.slideConstraint );
		}
		
		if ( particle.motorConstraint )
		{
			cpSpaceRemoveConstraint( _space, particle.motorConstraint );
			cpConstraintFree( particle.motorConstraint );
		}
		
		if ( particle.shape )
		{
			cpSpaceRemoveShape( _space, particle.shape );
			cpShapeFree( particle.shape );
		}
		
		if ( particle.body )
		{
			cpSpaceRemoveBody( _space, particle.body );
			cpBodyFree( particle.body );
		}		
	}
	
	if ( _centralBodyConstraint )
	{
		cpSpaceRemoveConstraint( _space, _centralBodyConstraint );
		cpConstraintFree( _centralBodyConstraint );
	}

	if ( _centralBody )
	{
		cpSpaceRemoveBody( _space, _centralBody );
		cpBodyFree( _centralBody );
	}
}

void MotileFluid::initialize( const init &i )
{
	GameObject::initialize( i );
	FluidRendererModel::initialize( i );
	_initializer = i;
}

// GameObject

void MotileFluid::setFinished( bool f )
{
	bool wasFinished = GameObject::finished();
	GameObject::setFinished(f);
	
	if ( !wasFinished && f )
	{
		_finishedTime = level()->time().time;
	}
}

bool MotileFluid::finished() const
{
	if ( GameObject::finished() )
	{
		return level()->time().time - _finishedTime > _initializer.extroTime;
	}
	
	return false;
}

void MotileFluid::addedToLevel( Level *level )
{
	GameObject::addedToLevel(level);
	_space = level->space();
	
	switch( fluidType() )
	{
		case PROTOPLASMIC_TYPE:
			_buildProtoplasmic();
			break;
			
		case AMORPHOUS_TYPE:
			_buildAmorphous();
			break;
	}
}

void MotileFluid::update( const time_state &time )
{
	if ( _initializer.introTime > 0 && _initializer.extroTime > 0 )
	{
		const seconds_t age = this->age();
		_lifecycle = 1.0;

		if ( age < _initializer.introTime )
		{
			_lifecycle = real(age / _initializer.introTime);
		}
		
		if ( GameObject::finished() )
		{
			_lifecycle = saturate<real>(1 - (( level()->time().time - _finishedTime ) / _initializer.extroTime));
		}
	}	

	switch( fluidType() )
	{
		case PROTOPLASMIC_TYPE:
			_updateProtoplasmic( time );
			break;
			
		case AMORPHOUS_TYPE:
			_updateAmorphous( time );
			break;
	}
}

#pragma mark - Protoplasmic Implementation

void MotileFluid::_buildProtoplasmic()
{
	//
	//	create the main body
	//

	const real 
		radius = std::max<real>(_initializer.radius * 0.5, 0.5),
		mass = 2 * M_PI * radius * radius,
		moment = cpMomentForCircle( mass, 0, radius, cpvzero );
		
	_centralBody = cpBodyNew( mass, moment );
	cpBodySetUserData( _centralBody, this );
	cpBodySetPos( _centralBody, cpv( _initializer.position ));
	cpSpaceAddBody( _space, _centralBody );

	_centralBodyConstraint = cpSimpleMotorNew( cpSpaceGetStaticBody( _space), _centralBody, 0 );
	cpSpaceAddConstraint( _space, _centralBodyConstraint );
	_motorConstraints.insert( _centralBodyConstraint );
	
	//
	//	Create particles for the jumbly body
	//
	
	const real 
		stiffness = saturate( _initializer.stiffness ),
		bodyParticleRadius = 1 * (2 * M_PI * radius) / _initializer.numParticles,
		bodyParticleMass = 1 * M_PI * bodyParticleRadius * bodyParticleRadius,
		bodyParticleMoment = INFINITY,
		bodyParticleSlideExtent = _initializer.radius - 0.5 * bodyParticleRadius,
		springDamping = lrp<real>(stiffness, 1, 10 );

	_springStiffness = lrp<real>(stiffness, 0.1, 0.5 ) * mass * cpvlength(cpSpaceGetGravity(_space));

	for ( int i = 0, N = _initializer.numParticles; i < N; i++ )
	{
		real 
			offsetAngle = (i * 2 * M_PI ) / N;
				
		fluid_particle fluidParticle;
		physics_particle physicsParticle;

		physicsParticle.scale = 1;
		physicsParticle.offsetPosition = radius * Vec2r( std::cos( offsetAngle ), std::sin( offsetAngle ));
		
		fluidParticle.position = _initializer.position + physicsParticle.offsetPosition;
		fluidParticle.physicsRadius = fluidParticle.drawRadius = physicsParticle.radius = bodyParticleRadius;

		physicsParticle.body = cpBodyNew( bodyParticleMass, bodyParticleMoment );
		cpBodySetPos( physicsParticle.body, cpv(fluidParticle.position) );
		cpBodySetUserData( physicsParticle.body, this );
		cpSpaceAddBody( _space, physicsParticle.body );	
		_fluidBodies.insert( physicsParticle.body );	
		
		physicsParticle.shape = cpCircleShapeNew( physicsParticle.body, bodyParticleRadius, cpvzero );
		cpShapeSetCollisionType( physicsParticle.shape, _initializer.collisionType );
		cpShapeSetLayers( physicsParticle.shape, _initializer.collisionLayers );
		cpShapeSetUserData( physicsParticle.shape, _initializer.userData ? _initializer.userData : this );
		cpShapeSetGroup( physicsParticle.shape, cpGroup(this) );
		cpShapeSetFriction( physicsParticle.shape, _initializer.friction );
		cpShapeSetElasticity( physicsParticle.shape, _initializer.elasticity );
		cpSpaceAddShape( _space, physicsParticle.shape );
		_fluidShapes.insert( physicsParticle.shape );
		
		physicsParticle.slideConstraintLength = bodyParticleSlideExtent;
		physicsParticle.slideConstraint = cpSlideJointNew( 
			_centralBody, 
			physicsParticle.body, 
			cpvzero, 
			cpvzero, 
			0, 
			bodyParticleSlideExtent);

		cpSpaceAddConstraint( _space, physicsParticle.slideConstraint );
		_fluidConstraints.insert( physicsParticle.slideConstraint );
		
		physicsParticle.springConstraint = cpDampedSpringNew( 
			_centralBody, 
			physicsParticle.body, 
			cpBodyWorld2Local( _centralBody, cpv(fluidParticle.position + physicsParticle.offsetPosition )), 
			cpvzero, 
			0, // rest length
			0, // zero stiffness at initialization time
			springDamping );

		cpSpaceAddConstraint( _space, physicsParticle.springConstraint );
		_fluidConstraints.insert( physicsParticle.springConstraint );

		_fluidParticles.push_back( fluidParticle );
		_physicsParticles.push_back( physicsParticle );
	}
	
	for ( int i = 0, N = _initializer.numParticles; i < N; i++ )
	{
		cpBody 
			*a = _physicsParticles[i].body,
			*b = _physicsParticles[(i+1)%N].body;

		cpConstraint *slide = cpSlideJointNew(a, b, cpvzero, cpvzero, 0, cpvdist( cpBodyGetPos(a), cpBodyGetPos(b)));
		cpSpaceAddConstraint( _space, slide );
		_perimeterConstraints.insert( slide );
		_fluidConstraints.insert( slide );		
	}
	
	//
	//	Add one more fluidParticle which will correspond to this->_centralBody and this->_shape in ::update()
	//
	
	_fluidParticles.push_back( fluid_particle() );	
}

void MotileFluid::_updateProtoplasmic( const time_state &time )
{
	real 
		lifecycle = this->lifecycle(),
		phaseOffset = M_PI * (time.time / _initializer.pulsePeriod),
		phaseIncrement = M_PI / _physicsParticles.size(),
		pulseMagnitude = _initializer.pulseMagnitude,
		particleScale = _initializer.particleScale;
				
	cpBB bounds = cpBBInvalid;	
	const Vec2r 
		mainBodyPosition = v2r( cpBodyGetPos( _centralBody )),
		up( 0,1 );
	
	//
	//	Update fluid particles. Note, fluidParticles is one larger than physicsParticles,
	//	corresponding to the spawnpoint central body -- so we'll manually update the last one outside the loop
	//
	
	fluid_particle_vec::iterator fluidParticle = _fluidParticles.begin();
	physics_particle_vec::iterator physicsParticle = _physicsParticles.begin();
	
	for( physics_particle_vec::iterator end(_physicsParticles.end()); 
		physicsParticle != end;
		++physicsParticle, ++fluidParticle )		
	{
		//
		//	spring want to hold particle at physics_particle::offsetPosition relative to _centralBody - 
		//	we need to ramp up/down spring stiffness to allow this to work
		//

		cpSlideJointSetMax( physicsParticle->slideConstraint, lifecycle * physicsParticle->slideConstraintLength );
		cpDampedSpringSetStiffness(physicsParticle->springConstraint, lifecycle * _springStiffness);

		//
		//	Scale the particle radius by lifecycle
		//

		physicsParticle->scale = lifecycle;
		real currentRadius = physicsParticle->radius * physicsParticle->scale;
	
		fluidParticle->physicsRadius = currentRadius;
		fluidParticle->drawRadius = particleScale * currentRadius;
		cpCircleShapeSetRadius( physicsParticle->shape, currentRadius );
		
		//
		//	Update position and angle
		//

		fluidParticle->position = v2r(cpBodyGetPos( physicsParticle->body ));
		fluidParticle->angle = cpBodyGetAngle( physicsParticle->body );
		cpBBExpand( bounds, fluidParticle->position, fluidParticle->drawRadius );

		//
		//	Now, make the particles on the bottom fully opaque, and the ones across the top transparent,
		//	with a periodic wiggle to make things more interesting
		//

		real wiggle = 1;
		if ( pulseMagnitude > 0 )
		{
			wiggle = ((1-pulseMagnitude) + (pulseMagnitude * std::cos( phaseOffset )));
		}

		Vec2r dir = normalize( fluidParticle->position - mainBodyPosition );
		fluidParticle->opacity = lifecycle * wiggle * lrp<real>( (( dot( dir, up ) + 1 ) * 0.5), 1, 0.125 );

		phaseOffset += phaseIncrement;
	}
	
	//
	//	Update the last fluid particle to match _body
	//
	
	fluidParticle->position = mainBodyPosition;
	fluidParticle->angle = cpBodyGetAngle( _centralBody );
	fluidParticle->drawRadius = fluidParticle->physicsRadius = std::pow( lifecycle, real(0.25) ) * _initializer.radius * 0.5;

	if ( pulseMagnitude > 0 )
	{
		fluidParticle->opacity = lifecycle * (1-pulseMagnitude) + ( pulseMagnitude * std::sin( phaseOffset ) * std::cos( phaseOffset ));
	}
	else
	{
		fluidParticle->opacity = lifecycle;
	}

	cpBBExpand( bounds, fluidParticle->position, fluidParticle->drawRadius );	
	
	//
	//	Update speed
	//

	if ( _centralBodyConstraint )
	{
		const real 
			circumference = 2 * _initializer.radius * M_PI,
			requiredTurns = _speed / circumference,
			motorRate = 2 * M_PI * requiredTurns;
	
		cpSimpleMotorSetRate( _centralBodyConstraint, motorRate );
	}

	//
	//	update Aabb
	//

	setAabb( bounds );	
}

#pragma mark - Amorphous MotileFluid Implementation

void MotileFluid::_buildAmorphous()
{
	//
	//	Create particles for the main body which the jumbly particles are attached to
	//	the main body has no collider, and a gear joint against the world to prevent rotation
	//

	const real 
		centralBodyRadius = 1,
		centralBodyMass = 1 * M_PI * centralBodyRadius * centralBodyRadius,
		centralBodyMoment = cpMomentForCircle( centralBodyMass, 0, centralBodyRadius, cpvzero );
		
	_centralBody = cpBodyNew( centralBodyMass, centralBodyMoment );
	cpBodySetUserData( _centralBody, this );
	cpBodySetPos( _centralBody, cpv( _initializer.position ));
	cpSpaceAddBody( _space, _centralBody );

	_centralBodyConstraint = cpGearJointNew( cpSpaceGetStaticBody( _space ), _centralBody, 0, 1 );
	cpSpaceAddConstraint( _space, _centralBodyConstraint );

	const real 
		overallRadius = std::max<real>(_initializer.radius, 0.5),
		bodyParticleRadius = 0.5 * (2 * M_PI * overallRadius) / _initializer.numParticles,
		bodyParticleMass = 1 * M_PI * bodyParticleRadius * bodyParticleRadius,
		bodyParticleMoment = cpMomentForCircle( bodyParticleMass, 0, bodyParticleRadius, cpvzero ),
		bodyParticleSlideExtent = _initializer.radius + _initializer.pulseMagnitude * bodyParticleRadius;

	_bodyParticleRadius = bodyParticleRadius;
		
	for ( int i = 0, N = _initializer.numParticles; i < N; i++ )
	{
		real 
			offsetAngle = (i * 2 * M_PI ) / N;
				
		fluid_particle fluidParticle;
		physics_particle physicsParticle;

		physicsParticle.scale = 1;
		physicsParticle.offsetPosition = overallRadius * Vec2r( std::cos( offsetAngle ), std::sin( offsetAngle ));
		
		fluidParticle.position = _initializer.position + physicsParticle.offsetPosition;
		fluidParticle.physicsRadius = fluidParticle.drawRadius = physicsParticle.radius = bodyParticleRadius;

		physicsParticle.body = cpBodyNew( bodyParticleMass, bodyParticleMoment );
		cpBodySetPos( physicsParticle.body, cpv(fluidParticle.position) );
		cpBodySetUserData( physicsParticle.body, this );
		cpSpaceAddBody( _space, physicsParticle.body );		
		_fluidBodies.insert( physicsParticle.body );
		
		physicsParticle.shape = cpCircleShapeNew( physicsParticle.body, bodyParticleRadius, cpvzero );
		cpShapeSetCollisionType( physicsParticle.shape, _initializer.collisionType );
		cpShapeSetLayers( physicsParticle.shape, _initializer.collisionLayers );
		cpShapeSetUserData( physicsParticle.shape, _initializer.userData ? _initializer.userData : this );
		cpShapeSetFriction( physicsParticle.shape, _initializer.friction );
		cpShapeSetElasticity( physicsParticle.shape, _initializer.elasticity );
		cpSpaceAddShape( _space, physicsParticle.shape );
		_fluidShapes.insert( physicsParticle.shape );
		
		physicsParticle.slideConstraintLength = bodyParticleSlideExtent;
		physicsParticle.slideConstraint = cpSlideJointNew( 
			_centralBody, 
			physicsParticle.body, 
			cpvzero, 
			cpvzero, 
			0, 
			bodyParticleSlideExtent);

		cpSpaceAddConstraint( _space, physicsParticle.slideConstraint );
		_fluidConstraints.insert( physicsParticle.slideConstraint );
		
		physicsParticle.motorConstraint = cpSimpleMotorNew( cpSpaceGetStaticBody(_space), physicsParticle.body, 0 );
		cpSpaceAddConstraint( _space, physicsParticle.motorConstraint );
		_motorConstraints.insert( physicsParticle.motorConstraint );

		_fluidParticles.push_back( fluidParticle );
		_physicsParticles.push_back( physicsParticle );
	}	
	
	if ( true )
	{
		fluid_particle centralParticle;
		centralParticle.physicsRadius = centralParticle.drawRadius = 0;
		centralParticle.position = v2r( cpBodyGetPos( _centralBody ));

		_fluidParticles.push_back( centralParticle );
	}
	
}

void MotileFluid::_updateAmorphous( const time_state &time )
{
	real 
		lifecycle = this->lifecycle(),
		phaseOffset = M_PI * time.time / _initializer.pulsePeriod,
		phaseIncrement = 2 * M_PI / _physicsParticles.size(),
		pulseMagnitude = _initializer.pulseMagnitude,
		particleScale = _initializer.particleScale,
		motorRate = 0;
				
	cpBB bounds = cpBBInvalid;	

	const Vec2r 
		mainBodyPosition = v2r( cpBodyGetPos( _centralBody ));
		
	{
		// compute required rotation rate - note, we ignore lifecycle to avoid div by zero.
		const real 
			circumference = 2 * _bodyParticleRadius * M_PI,
			requiredTurns = _speed / circumference;

		motorRate = 2 * M_PI * requiredTurns / std::sqrt(_fluidParticles.size());
	}
	
	//
	//	Update fluid particles. Note, fluidParticles is one larger than physicsParticles,
	//	corresponding to the spawnpoint central body -- so we'll manually update the last one outside the loop
	//
	
	fluid_particle_vec::iterator fluidParticle = _fluidParticles.begin();
	physics_particle_vec::iterator physicsParticle = _physicsParticles.begin();
	
	real 
		averageDrawRadius = 0,
		averageDrawAngle = 0;
	
	
	for( physics_particle_vec::iterator end(_physicsParticles.end()); 
		physicsParticle != end;
		++physicsParticle, ++fluidParticle )		
	{
		//
		//	spring want to hold particle at physics_particle::offsetPosition relative to _centralBody - 
		//	we need to ramp up/down spring stiffness to allow this to work
		//

		cpSlideJointSetMax( physicsParticle->slideConstraint, lifecycle * physicsParticle->slideConstraintLength );

		//
		//	Scale the particle radius by lifecycle
		//

		real pulse = 1 + pulseMagnitude * std::cos( phaseOffset );
		physicsParticle->scale = lifecycle * pulse;
		real currentRadius = physicsParticle->radius * physicsParticle->scale;
		cpCircleShapeSetRadius( physicsParticle->shape, std::max<real>( currentRadius, 0.01 ) );

		
		//
		//	Update fluid rendering properties
		//

		fluidParticle->position = v2r(cpBodyGetPos( physicsParticle->body ));
		fluidParticle->angle = cpBodyGetAngle( physicsParticle->body );
		fluidParticle->physicsRadius = currentRadius;
		fluidParticle->drawRadius = particleScale * currentRadius;
		fluidParticle->opacity = lifecycle;
		
		averageDrawRadius += fluidParticle->drawRadius;
		averageDrawAngle += fluidParticle->angle;
		
		//
		//	Update motor rate
		//
		
		if ( physicsParticle->motorConstraint )
		{
			cpSimpleMotorSetRate( physicsParticle->motorConstraint, motorRate );
		}

		cpBBExpand( bounds, fluidParticle->position, fluidParticle->drawRadius );
		
		phaseOffset += phaseIncrement;
	}

	//
	//	Update the 'cap' particle atop the centralBody
	//

	if ( fluidParticle != _fluidParticles.end() )
	{
		real rs = real(1) / _physicsParticles.size();
		averageDrawRadius *= rs;
		averageDrawAngle *= rs;
		
		fluidParticle->position = mainBodyPosition;
		fluidParticle->angle = averageDrawAngle;
		fluidParticle->physicsRadius = 0;
		fluidParticle->drawRadius = lrp<real>( time.deltaT, fluidParticle->drawRadius, averageDrawRadius * 1 );
		fluidParticle->opacity = lifecycle;
		
		cpBBExpand( bounds, fluidParticle->position, fluidParticle->drawRadius );
	}
	
		
	//
	//	update Aabb
	//

	setAabb( bounds );	
}

#pragma mark - MotileFluidDebugRenderer

MotileFluidDebugRenderer::MotileFluidDebugRenderer()
{
	setName( "MotileFluidDebugRenderer" );
}

MotileFluidDebugRenderer::~MotileFluidDebugRenderer()
{}

void MotileFluidDebugRenderer::_drawDebug( const render_state &state )
{
	_debugDrawChipmunkConstraints( state, motileFluid()->fluidConstraints() );
	_debugDrawChipmunkBody( state, motileFluid()->centralBody() );
}



} // end namespace game