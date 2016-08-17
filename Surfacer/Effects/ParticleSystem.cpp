//
//  ParticleSystem.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 7/20/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "ParticleSystem.h"
#include <cinder/app/App.h>

#include "GameConstants.h"
#include "FastTrig.h"
#include "Transform.h"

using namespace ci;
using namespace core;
namespace game {

namespace {

	const static Vec2f ParticleShape[4] = 
	{ 
		Vec2f( -1, -1 ),
		Vec2f( +1, -1 ),
		Vec2f( +1, +1 ),
		Vec2f( -1, +1 )
	};

	const static Vec2f TexCoords[4] = 
	{ 
		Vec2f( 0, 0 ),
		Vec2f( 1, 0 ),
		Vec2f( 1, 1 ),
		Vec2f( 0, 1 )
	};
	
	const float 
		r2 = 1.0f / 2.0f,
		r4 = 1.0f / 4.0f;
	
	const static Vec2f AtlasOffset_None[1] = { Vec2f(0,0) };

	const static Vec2f AtlasOffset_TwoByTwo[4] = 
	{ 
		Vec2f( 0*r2, 1*r2), Vec2f( 1*r2, 1*r2), 
		Vec2f( 0*r2, 0*r2), Vec2f( 1*r2, 0*r2)
	}; 

	const static Vec2f AtlasOffset_FourByFour[16] = 
	{ 
		Vec2f( 0*r4, 3*r4), Vec2f( 1*r4, 3*r4), Vec2f( 2*r4, 3*r4), Vec2f( 3*r4, 3*r4),
		Vec2f( 0*r4, 2*r4), Vec2f( 1*r4, 2*r4), Vec2f( 2*r4, 2*r4), Vec2f( 3*r4, 2*r4),
		Vec2f( 0*r4, 1*r4), Vec2f( 1*r4, 1*r4), Vec2f( 2*r4, 1*r4), Vec2f( 3*r4, 1*r4),
		Vec2f( 0*r4, 0*r4), Vec2f( 1*r4, 0*r4), Vec2f( 2*r4, 0*r4), Vec2f( 3*r4, 0*r4)
	};


	const static Vec2f const *AtlasOffsetsByAtlasType[3] = {
		AtlasOffset_None,
		AtlasOffset_TwoByTwo,
		AtlasOffset_FourByFour
	};
	
	const static float AtlasScalingByAtlasType[3] = {
		1,r2,r4
	};


}


#pragma mark ParticleSystem

/*
		init _initializer;
		ParticleSystemController *_controller;
		std::vector< particle > _particles;
*/

ParticleSystem::ParticleSystem():
	GameObject( GameObjectType::PARTICLE_SYSTEM ),
	_controller(NULL)
{
	setName( "ParticleSystem" );
	setLayer( RenderLayer::EFFECTS );
	setVisibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING );

	addComponent( new ParticleSystemRenderer() );
}

ParticleSystem::~ParticleSystem()
{
	delete _controller;
}

void ParticleSystem::initialize( const init &initializer )
{
	GameObject::initialize(initializer);
	_initializer = initializer;
}

void ParticleSystem::setController( ParticleSystemController *controller ) 
{ 
	if ( _controller )
	{
		delete _controller;
		_controller = NULL;
	}

	_controller = controller;

	_particles.resize( _controller->count() );
	for( std::size_t i = 0, N = _particles.size(); i < N; i++ )
	{
		_particles[i].idx = i;
	}

	_controller->setParticleSystem(this);
	_controller->initialize( _particles );
}


void ParticleSystem::update( const time_state &time )
{
	_controller->update(time, _particles);
	
	//
	//	Update the AABB
	//

	cpBB bounds = cpBBInvalid;
	for ( std::vector< particle >::iterator particle( _particles.begin() ), end( _particles.begin() + _controller->activeParticleCount() ); particle != end; ++particle )
	{
		cpBBExpand( bounds, particle->position, particle->radius );
	}
	
	setAabb(bounds);
}

#pragma mark -
#pragma mark ParticleSystemRenderer

/*
		bool _initialized;
		std::vector< particle_vertex > _vertices;
		ci::gl::GlslProg _shader;
*/

ParticleSystemRenderer::ParticleSystemRenderer()
{}

ParticleSystemRenderer::~ParticleSystemRenderer()
{}

void ParticleSystemRenderer::draw( const render_state &state )
{
	_updateParticles();
	DrawComponent::draw(state);
}

ci::gl::GlslProg ParticleSystemRenderer::loadShader()
{
	ParticleSystem *system = (ParticleSystem*) owner();

	DataSourceRef vertexShader = system->initializer().vertexShader,
				  fragmentShader = system->initializer().fragmentShader;

	if ( vertexShader && fragmentShader )
	{
		return gl::GlslProg( vertexShader, fragmentShader );
	}
	
	return level()->resourceManager()->getShader( "ParticleShader" );
}


void ParticleSystemRenderer::_drawGame( const render_state &state )
{
	ParticleSystem *system = (ParticleSystem*) owner();
	const ParticleSystem::init &params = system->initializer();

	shader().bind();
	bindShaderUniforms();
		
	//
	//	Now, set up gl state
	//

	gl::disableWireframe();

	glEnable( GL_BLEND );
	glBlendEquation( GL_FUNC_ADD );
	glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );

	//
	//	Bind textures
	//

	for ( int i = 0, N = params.textures.size(); i < N; i++ )
	{
		glActiveTexture( GL_TEXTURE0 + i );
		glBindTexture( params.textures[i].getTarget(), params.textures[i].getId() );
		_shader.uniform( "Texture" + str(i), i );
	}
	
	//
	//	Set up our interleaved arrays
	//
	
	const GLsizei stride = sizeof( particle_vertex );

	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 2, GL_REAL, stride, &(_vertices[0].position) );

	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_REAL, stride, &(_vertices[0].texCoord));

	glEnableClientState( GL_COLOR_ARRAY );
	glColorPointer( 4, GL_REAL, stride, &(_vertices[0].color));
	
	//
	//	Draw!
	//
		
	glDrawArrays( GL_QUADS, 0, system->activeParticleCount() * 4 );

	//
	//	Clean up
	//

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_COLOR_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );	

	for ( int i = params.textures.size() - 1; i >= 0; i-- )
	{
		glActiveTexture( GL_TEXTURE0 + i );
		glBindTexture( params.textures[i].getTarget(), 0 );
	}

	// restore sane/default blending
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );		
	glDisable( GL_BLEND );

	ci::gl::GlslProg::unbind();
}

void ParticleSystemRenderer::_drawDevelopment( const render_state &state )
{
	_drawGame( state );

	ParticleSystem *system = (ParticleSystem*) owner();
	
	glEnable( GL_BLEND );

	real alpha = 0.75;
	ci::ColorA color( system->debugColor() ),
	           xAxis( 1,0,0,alpha ),
	           yAxis( 0,1,0,alpha );

	color.a = alpha;
	
	for ( int i = 0, N = system->activeParticleCount() * 4; i < N; i+=4 )
	{
		Vec2f a = _vertices[i+0].position,
		      b = _vertices[i+1].position,
		      c = _vertices[i+2].position,
		      d = _vertices[i+3].position;
			
		gl::color( color );
		gl::drawLine( a,b );
		gl::drawLine( b,c );
		gl::drawLine( c,d );
		gl::drawLine( d,a );
		
		gl::color( xAxis );
		gl::drawLine( mid(a,c),mid(b,c));

		gl::color( yAxis );
		gl::drawLine( mid(a,c),mid(c,d));
	}
	
	glDisable( GL_BLEND );	
}

void ParticleSystemRenderer::_drawDebug( const render_state &state )
{}

void ParticleSystemRenderer::_updateParticles()
{
	ParticleSystem *system = (ParticleSystem*) owner();

	//
	//	We're using a faster but less accurate trig approximation
	//

	using util::fasttrig::sin;
	using util::fasttrig::cos;
	
	//
	//	Now assign offsets and scaling to match this system's atlas type; the per-perticle
	//	lookup into these arrays will happen in the loop
	//

	const ParticleAtlasType::Type atlasType = system->initializer().atlasType;
	const Vec2f const* AtlasOffsets = AtlasOffsetsByAtlasType[atlasType];
	const float AtlasScaling = AtlasScalingByAtlasType[atlasType];	
	
	//
	//	Lazily re/allocate vertex storage
	//

	if ( _vertices.size() != system->particles().size() * 4 )
	{
		_vertices.resize( system->particles().size() * 4 );
	}

	//
	//	Update the four vertices that correspond to each particle.
	//	note: we use a different loop for controllers that rotate particles
	//

	if ( system->controller()->rotatesParticles() )
	{
		Vec2f rotatedShape[4];
		util::Transform rotator;
		

		std::vector< particle_vertex >::iterator vertex = _vertices.begin();
		for ( std::vector< particle >::const_iterator particle( system->particles().begin()), pEnd( system->particles().begin() + system->activeParticleCount()); particle != pEnd; ++particle )
		{
		
			const real 
				radius = particle->radius,
				xScale = radius * particle->xScale,
				yScale = radius * particle->yScale;

			rotator.setAngle( particle->angle );
			rotatedShape[0].x = -xScale;
			rotatedShape[0].y = -yScale;

			rotatedShape[1].x = +xScale;
			rotatedShape[1].y = -yScale;

			rotatedShape[2].x = +xScale;
			rotatedShape[2].y = +yScale;

			rotatedShape[3].x = -xScale;
			rotatedShape[3].y = +yScale;
				
			rotatedShape[0] = rotator * rotatedShape[0];
			rotatedShape[1] = rotator * rotatedShape[1];
			rotatedShape[2] = rotator * rotatedShape[2];
			rotatedShape[3] = rotator * rotatedShape[3];
		
			
			//
			//	For each vertex, assign position, color and texture coordinate
			//
			
			for ( int i = 0; i < 4; i++ )
			{
				vertex->position = particle->position + rotatedShape[i];
				
				ci::Vec4f pc = particle->color;
				vertex->color = ci::Vec4f( pc.x * pc.w, pc.y * pc.w, pc.z * pc.w, pc.w * ( real(1) - particle->additivity ));				
								
				vertex->texCoord = (TexCoords[i] * AtlasScaling) + AtlasOffsets[particle->atlasIdx];

				++vertex;
			}
		}		
	}
	else
	{
		//
		//	For each vertex, assign position, color and texture coordinate
		//

		std::vector< particle_vertex >::iterator vertex = _vertices.begin();
		for ( std::vector< particle >::const_iterator particle( system->particles().begin()), pEnd( system->particles().begin() + system->activeParticleCount()); particle != pEnd; ++particle )
		{
			for ( int i = 0; i < 4; i++ )
			{
				vertex->position = particle->position + (particle->radius * ParticleShape[i]);

				ci::Vec4f pc = particle->color;
				vertex->color = ci::Vec4f( pc.x * pc.w, pc.y * pc.w, pc.z * pc.w, pc.w * ( real(1) - particle->additivity ));				

				vertex->texCoord = (TexCoords[i] * AtlasScaling) + AtlasOffsets[particle->atlasIdx];				

				++vertex;
			}
		}		
	}
}

#pragma mark -
#pragma mark UniversalParticleSystemController

/*
		std::size_t _activeParticleCount;
		std::vector< particle_state > _templates, _states;
		signals::signal< void( const time_state & ) > _updateSignal;
		std::vector< Emitter * > _emitters;
*/

UniversalParticleSystemController::UniversalParticleSystemController():
	_activeParticleCount(0)
{}

UniversalParticleSystemController::~UniversalParticleSystemController()
{
	foreach( Emitter *e, _emitters ) delete e;
}

void UniversalParticleSystemController::initialize( const init &initializer )
{
	//
	//	populate _templates with particle_template::count mutations of the initializer template. 
	//	Each time we emit a particle, we'll randomly grab from _templates.
	//

	foreach( const particle_template &pt, initializer.templates )
	{
		for ( int i = 0; i < pt.count; i++ )
		{
			_templates.push_back( pt.vend() );
		}
	}
	
	std::random_shuffle( _templates.begin(), _templates.end() );
	initialize( _templates.size() );
	
	//
	//	create the default emitter
	//
	
	emitter(initializer.defaultParticlesPerSecond);
}

void UniversalParticleSystemController::initialize( std::size_t particleCount )
{
	_states.resize( particleCount, particle_state() );
	
	//
	//	create the default emitter
	//
	
	{
		init initializer;
		emitter(initializer.defaultParticlesPerSecond);
	}
}

UniversalParticleSystemController::Emitter*
UniversalParticleSystemController::emitter( std::size_t particlesPerSecond ) 
{ 
	Emitter *e = new Emitter(this, particlesPerSecond);
	_emitters.push_back(e);
	return e;
}

void UniversalParticleSystemController::update( const time_state &time, std::vector< particle > &particles )
{
	Vec2r gravity = v2r( cpSpaceGetGravity( system()->level()->space() ));

	//
	//	Update emitters
	//

	foreach( Emitter *e, _emitters ) e->_update( time );

	//
	//	update all particles
	//
	
	for( std::vector< particle >::iterator particle( particles.begin()), end(particles.end()); 
		 particle != end; ++particle )
	{
		particle_state &state = _states[ particle->idx ];

		if ( state._alive )
		{
			//
			//	We want to force the particle's alpha to fade in when the particle is first vended,
			//	and to fade out as particles get older. This mitigates popping.
			//

			const real age = state._age / state.lifespan;
			real ageAlpha = 1;

			if ( age < state.fade.min ) 
			{
				ageAlpha = age / state.fade.min; 
			}
			else if ( age > state.fade.max ) 
			{
				ageAlpha = 1 - (( age - state.fade.max ) / ( 1 - state.fade.max ));
			}
			
			const real damping = lrp( age, state.damping );			
			const Vec2r changeInPosition = state._linearVelocity * time.deltaT;
						
			particle->position = particle->position + changeInPosition;
			state._linearVelocity = state._linearVelocity * damping + ( gravity * lrp( age, state.gravity ) * time.deltaT );
				
			particle->color = lrp( age, state.color );
			particle->color.w *= ageAlpha;
			particle->additivity = lrp( age, state.additivity );
			
			const real vel = state._linearVelocity.length();
			if ( state.orientsToVelocity )
			{
				if ( vel > real(0) )
				{
					Vec2r dir = state._linearVelocity / vel;
					particle->angle = std::atan2( dir.y, dir.x );
				}
			}
			else
			{
				particle->angle = particle->angle + ( lrp( age, state.angularVelocity ) * time.deltaT );
				state.angularVelocity.min *= damping;
				state.angularVelocity.max *= damping;
			}

			//
			//	compute particle radius and scaling
			//

			real radius = lrp( age, state.radius );
			particle->radius = radius;
			particle->xScale = particle->yScale = 1;
			
			if ( state.velocityScaling > 1 )
			{
				real 
					distanceTraveled = changeInPosition.length(),
					velocityScaling = (state.velocityScaling * distanceTraveled * real(0.5)) / radius;
				
				if ( state.orientsToVelocity )
				{
					particle->xScale = std::max<real>(velocityScaling,1);
					particle->yScale = 1;
				}
				else
				{
					particle->xScale = particle->yScale = velocityScaling;
				}
			}

			//
			//	increment age, and mark if this particle has died
			//

			state._age += time.deltaT;
			state._alive = state._age < state.lifespan;
		}
	}
	
	//
	//	move dead particles to end of storage
	//

	_pack( particles );
}

bool UniversalParticleSystemController::_emit( const particle_state &emitState, const Vec2r &position, const Vec2r &velocity )
{
	std::vector< particle > &particles = system()->particles();
	if ( _activeParticleCount < particles.size() )
	{
		// get the particle we'll be acting on
		particle &particle = particles[_activeParticleCount];

		// initialize the state for this particle
		_states[particle.idx] = emitState;
		particle_state &state = _states[particle.idx];
		state._linearVelocity = state.initialVelocity() + velocity;
		state._age = 0;
		state._alive = true;

		// initialize the corresponding particle
		particle.position = position;
		particle.color = state.color.min;
		particle.radius = state.radius.min;
		particle.additivity = state.additivity.min;
		particle.atlasIdx = state.atlasIdx;

		if ( !state.orientsToVelocity )
		{
			particle.angle = Rand::randFloat(M_PI*2);
		}
		else
		{
			particle.angle = 0;
			state.angularVelocity = 0;
		}
		
		_activeParticleCount++;
		return true;
	}
		
	return false;
}



#pragma mark -
#pragma mark UniversalParticleSystemController::Emitter

/*
	UniversalParticleSystemController *_controller;
	bool _throttled;
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
*/

UniversalParticleSystemController::Emitter::Emitter( UniversalParticleSystemController *controller, std::size_t pps ):
	_controller( controller ),
	_throttled(false),
	_noisy(false),
	_targetParticlesPerSecond(0),
	_particlesEmittedSinceLastTally(0),
	_particlesPerStep(0),
	_particlesToEmitThisStep(0),
	_currentParticlesPerSecond(0),
	_lastTallyTime(0)
{
	setParticlesPerSecond( pps );
}

void UniversalParticleSystemController::Emitter::emit( std::size_t count, const Vec2r &position, const Vec2r &velocity )
{
	if ( _controller->_templates.empty() )
	{
		throw DrawException( "UniversalParticleSystemController::Emitter::emit -- Cannot emit random particles from template when template is empty" );
	}

	for ( std::size_t i = 0; i < count; i++ )
	{
		UniversalParticleSystemController::particle_state state = _controller->_templates[ Rand::randInt( _controller->_templates.size())];
		emit( state, position, velocity );
	}
}

void UniversalParticleSystemController::Emitter::emit( const particle_state &particle, const Vec2r &position, const Vec2r &velocity )
{
	if ( _throttled )
	{
		_emission.push_back( emission_state( particle, position, velocity ));

		if ( _noisy )
		{
			app::console() << _controller->system()->description() << "::Emitter::emit - there are " << _emission.size() << " particles in emission stack" << std::endl;
		}
	}
	else
	{
		_controller->_emit( particle, position, velocity );
		_particlesEmittedSinceLastTally++;
	}
}

void UniversalParticleSystemController::Emitter::_update( const time_state &time )
{
	if ( _throttled )
	{
		//
		//	Lazily compute the number of particles that can be emitted per step
		//
		
		if ( _particlesPerStep < Epsilon )
		{
			_particlesPerStep = _targetParticlesPerSecond * time.deltaT;
		}

		if ( !_emission.empty() )
		{
			// increment allotment
			_particlesToEmitThisStep += _particlesPerStep;

			//
			//	Randomise our _emission buffer (to prevent fifo bias), and emit each that can be emitted.
			//	We clear at the end, because all that are left in the buffer are beyond our emission allotment for this step
			//
		
			if ( _noisy )
			{
				app::console() << _controller->system()->description() << "::Emitter::_update emitting: " << _particlesToEmitThisStep << " particles" << std::endl;
			}
		
			std::random_shuffle( _emission.begin(), _emission.end() );
			while( _particlesToEmitThisStep >= 1 - Epsilon && !_emission.empty() )
			{
				const emission_state &es( _emission.back() );
				_controller->_emit( es.state, es.position, es.velocity );
				_emission.pop_back();
				_particlesToEmitThisStep--;
				_particlesEmittedSinceLastTally++;
			}			
			
			_emission.clear();
		}

	}

	//
	//	Compute our current emission rate for debugging
	//

	if ( time.time > _lastTallyTime + 1 )
	{
		seconds_t elapsed = time.time - _lastTallyTime;
		_lastTallyTime = time.time;
		
		_currentParticlesPerSecond = real( _particlesEmittedSinceLastTally / elapsed );
		_particlesEmittedSinceLastTally = 0;
	}		
}

void UniversalParticleSystemController::Emitter::setParticlesPerSecond( std::size_t pps )
{
	_targetParticlesPerSecond = pps;
	_throttled = pps < INT_MAX;
	_particlesPerStep = 0; // mark that we need to compute our emission limit in _update
}


} // end namespace game