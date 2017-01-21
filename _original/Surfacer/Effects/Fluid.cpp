//
//  Fluid.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/22/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "Fluid.h"

#include "ChipmunkDebugDraw.h"
#include "FilterStack.h"
#include "GameConstants.h"
#include "Transform.h"

#include <cinder/Rand.h>
#include <chipmunk/chipmunk_unsafe.h>

using namespace ci;
using namespace  core;
namespace game {

namespace {

	struct clump_callback_info {

		cpShape *testShape;
		cpBody *testBody;
		Fluid *fluid;

		real 
			testShapeRadius,
			clumpImpulse;

	};

	void clumpCallback(cpShape *shape, void *data)
	{
		clump_callback_info *info = (clump_callback_info*) data;
		
		if ( shape == info->testShape || cpShapeGetUserData( shape ) != info->fluid ) 
		{
			return;
		}

		cpBody *otherBody = cpShapeGetBody( shape );
		cpVect dir = cpvsub(cpBodyGetPos(otherBody), cpBodyGetPos(info->testBody)); 
		real 
			otherShapeRadius = cpCircleShapeGetRadius( shape ),
			minDist = info->testShapeRadius + otherShapeRadius,
			maxDist = minDist * real(1.5),
			middleDist = (minDist + maxDist) * real(0.5),
			dist = cpvlength( dir ),
			impulse = info->clumpImpulse * real(0.5),
			scale = 0;
		
		//
		//	maximum clumping force applied at midpoint between min and max,
		//	scaling to zero when shapes are touching, and when they're at maxDist.
		//

		if ( dist <= minDist )
		{
			return;
		}
		else if ( dist < middleDist )
		{
			scale = (dist - minDist) / (middleDist - minDist);
		}
		else if ( dist > middleDist )
		{
			scale = 1 - (( dist - middleDist ) / ( maxDist - middleDist ));
		}
		else if ( dist > maxDist ) 
		{
			return;
		}
						
		// normalize force dir and apply impulse
		dir = cpvmult( dir, 1 / dist );
		cpBodyApplyImpulse( info->testBody, cpvmult(dir, scale * impulse * cpBodyGetMass( info->testBody )), cpvzero );
		cpBodyApplyImpulse( otherBody, cpvmult(dir, -scale * impulse * cpBodyGetMass( otherBody )), cpvzero );
	}
}

#pragma mark - FluidRendererModel

void FluidRendererModel::initialize( const init &i )
{
	setFluidParticleTexture( i.fluidParticleTexture );
	setFluidToneMap( i.fluidToneMapTexture );
	setFluidBackground( i.fluidBackgroundTexture );
	setFluidToneMapLumaPower( i.fluidToneMapLumaPower );
	setFluidToneMapHue( i.fluidToneMapHue );
	setFluidOpacity( i.fluidOpacity );
	setFluidHighlightColor( Vec4( i.fluidHighlightColor.r, i.fluidHighlightColor.g, i.fluidHighlightColor.b, i.fluidHighlightColor.a ));
	setFluidHighlightLookupDistance( i.fluidHighlightLookupDistance );
	setFluidShininess(i.fluidShininess);
}

#pragma mark - FluidRenderer

namespace {

	const std::size_t TONE_MAP_BIT = 1 << 0;
	const std::size_t BACKGROUND_BIT = 1 << 1;

}

/*
		ci::fs::path _particleTexture, _toneMapTexture;
		float _toneMapLumaPower, _toneMapHue, _opacity, _shininess, _highlightLookupDistance;
		Vec4 _highlightColor;
*/

FluidRenderer::FluidRenderer():
	_model(NULL),
	_opacityAttrLoc(0)
{}

FluidRenderer::~FluidRenderer()
{}
		
void FluidRenderer::_drawGame( const render_state &state )
{
	FluidRendererModel *model = this->model();
	Area vpBounds = state.viewport.bounds();		

	if ( !_compositingFbo || !(vpBounds==_compositingFbo.getBounds()))
	{
		gl::Fbo::Format format;
		format.setTarget(GL_TEXTURE_RECTANGLE_ARB);
		format.enableColorBuffer( true );
		format.enableDepthBuffer( false );
		format.enableMipmapping( false );

		_compositingFbo = gl::Fbo(	
			state.viewport.bounds().getWidth(),
			state.viewport.bounds().getHeight(),
			format
		);
	}

	{
		gl::SaveFramebufferBinding bindingSaver;
		_compositingFbo.bindFramebuffer();

			gl::pushMatrices();
			state.viewport.set();
			gl::clear( Color::black() );						
			
				_drawParticles( state );
			
			gl::popMatrices();

		_compositingFbo.unbindFramebuffer();
	}

	//
	//	Lazily load textures, and load appropriate shader depending on which textures are being used
	//
	if ( !model->fluidToneMap().empty() )
	{
		if ( !_toneMapTexture )
		{
			_toneMapTexture = level()->resourceManager()->getTexture( model->fluidToneMap() );
		}
	}
	
	if ( !model->fluidBackground().empty() )
	{
		if ( !_backgroundTexture )
		{
			_backgroundTexture = level()->resourceManager()->getTexture( model->fluidBackground() );
		}
	}
	
	if ( !_compositingShader )
	{
		std::string frag = "FluidCompositor_ToneMapAndBackground.frag";
		if ( _toneMapTexture && !_backgroundTexture ) frag = "FluidCompositor_ToneMap.frag";
		else if ( !_toneMapTexture && _backgroundTexture ) frag = "FluidCompositor_Background.frag";

		_compositingShader = level()->resourceManager()->getShader( "FluidCompositor.vert", frag );
	}

	_compositingShader.bind();
	_compositingShader.uniform( "InputImage", 0 );
	
	{
		int texUnit = 1;
			
		if ( _toneMapTexture )
		{
			_toneMapTexture.bind(texUnit);
			glActiveTexture( GL_TEXTURE0 + texUnit );
			_toneMapTexture.setWrap( GL_CLAMP_TO_EDGE, GL_REPEAT );
			_toneMapTexture.setMinFilter( GL_NEAREST );
			_toneMapTexture.setMagFilter( GL_NEAREST );

			_compositingShader.uniform( "ToneMap", texUnit );
			_compositingShader.uniform( "ToneMapWidth", float(_toneMapTexture.getSize().x) );

			texUnit++;
		}
		
		if ( _backgroundTexture )
		{
			_backgroundTexture.bind(texUnit);
			glActiveTexture( GL_TEXTURE0 + texUnit );
			_backgroundTexture.setWrap( GL_CLAMP_TO_EDGE, GL_REPEAT );
			_backgroundTexture.setMinFilter( GL_NEAREST );
			_backgroundTexture.setMagFilter( GL_NEAREST );


			_compositingShader.uniform( "BackgroundImage", texUnit );
			_compositingShader.uniform( "BackgroundImageSize", Vec2(_backgroundTexture.getWidth(),_backgroundTexture.getHeight()) );
		}		
	}

	
	_compositingShader.uniform( "LumaPower", std::max<float>(model->fluidToneMapLumaPower(),0.01) );
	_compositingShader.uniform( "Hue", model->fluidToneMapHue() );
	_compositingShader.uniform( "Opacity", model->fluidOpacity() );
	_compositingShader.uniform( "HighlightColor", model->fluidHighlightColor() );
	_compositingShader.uniform( "HighlightLookupDistPx", float(model->fluidHighlightLookupDistance() * state.viewport.zoom()));
	_compositingShader.uniform( "Shininess", float(model->fluidShininess()));
		
		//
		// piggyback the FilterStack's draw API - this works because the filter stack is sized to our screen
		//

		owner()->level()->scenario()->filters()->draw( _compositingFbo );	
	
	_compositingShader.unbind();

	if ( _toneMapTexture )
	{
		_toneMapTexture.unbind(1);
	}
	else if ( _backgroundTexture )
	{
		_backgroundTexture.unbind(1);
	}

	glActiveTexture( GL_TEXTURE0 );
}

void FluidRenderer::_drawDebug( const render_state &state )
{
	FluidRendererModel *model = this->model();
	const FluidRendererModel::fluid_particle_vec &particles = model->fluidParticles();

	if ( particles.empty() ) return;

	gl::enableAlphaBlending(true);
	
	ci::ColorA color = owner()->debugColor();
	color.a = 0.5;
	gl::color( color );
	
	ci::ColorA 
		fillColor = owner()->debugColor(),
		strokeColor = ci::ColorA(1,1,1,0.5);	 

	for ( FluidRendererModel::fluid_particle_vec::const_iterator 
		particle(particles.begin()), 
		end( particles.end()); 
		particle != end; 
		++particle )
	{
		util::cpdd::DrawCircle( cpv( particle->position ), particle->angle, particle->physicsRadius, fillColor, strokeColor );
	}
}

void FluidRenderer::_drawParticles( const render_state &state )
{
	FluidRendererModel *model = this->model();	
	const FluidRendererModel::fluid_particle_vec &particles = model->fluidParticles();
	
	if ( particles.empty() ) return;
	
	//
	//	Lazily load particle texture
	//

	if ( !_particleTexture )
	{
		_particleTexture = level()->resourceManager()->getTexture( model->fluidParticleTexture() );
	}
	
	//
	//	Lazily create our particle vertex store
	//
	
	if ( particles.size() * 4 > _vertices.size() )
	{
		const ci::Vec2 texCoords[4] = {
			ci::Vec2(0,0),
			ci::Vec2(1,0),
			ci::Vec2(1,1),
			ci::Vec2(0,1)
		};
	
		_vertices.resize( particles.size() * 4 );
		for( particle_vertex_vec::iterator v( _vertices.begin()), end( _vertices.end()); v != end; ++v )
		{
			v->texCoord = texCoords[( v - _vertices.begin() ) % 4];
		}
	}
		
	//
	//	lazily create shader
	//
	
	if ( !_shader )
	{
		_shader = level()->resourceManager()->getShader( "FluidParticleShader" );
		_opacityAttrLoc = _shader.getAttribLocation( "ParticleOpacity" );
	}
	
	
	//
	//	Update our particle positions
	//
	
	real particleCorner = 1.414213562;
	const ci::Vec2 vertexTemplate[4] = {
		ci::Vec2( -particleCorner, -particleCorner ),
		ci::Vec2( +particleCorner, -particleCorner ),
		ci::Vec2( +particleCorner, +particleCorner ),
		ci::Vec2( -particleCorner, +particleCorner )
	};

	Vec2 rotatedShape[4];
	util::Transform rotator;
	particle_vertex_vec::iterator vertex( _vertices.begin());
	for( FluidRendererModel::fluid_particle_vec::const_iterator 
		particle( particles.begin()), 
		end( particles.end()); 
		particle != end; 
		++particle )
	{
		rotator.setAngle( particle->angle );
		for ( int i = 0; i < 4; ++i, ++vertex )
		{
			vertex->position = particle->position + (rotator * vertexTemplate[i] * particle->drawRadius);
			vertex->opacity = particle->opacity;
		}		
	}
	
	
	_particleTexture.bind(0);
	_particleTexture.setWrap( GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE );
	_particleTexture.setMagFilter( GL_LINEAR );
	_particleTexture.setMinFilter( GL_LINEAR_MIPMAP_LINEAR );

	_shader.bind();
	_shader.uniform( "ParticleTex", 0 );
	
	glEnable( GL_BLEND );
	glBlendFunc( GL_ONE, GL_ONE );
	
	//
	//	Now, submit vertices
	//
	
	const GLsizei stride = sizeof( particle_vertex );

	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 2, GL_REAL, stride, &(_vertices[0].position) );

	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer( 2, GL_REAL, stride, &(_vertices[0].texCoord));

	glEnableVertexAttribArray( _opacityAttrLoc );
	glVertexAttribPointer( _opacityAttrLoc, 1, GL_REAL, GL_FALSE, stride, &(_vertices[0].opacity) );
	
	//
	//	Draw!
	//
		
	glDrawArrays( GL_QUADS, 0, particles.size() * 4 );

	//
	//	Clean up
	//

	glDisableClientState( GL_TEXTURE_COORD_ARRAY );	
	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableVertexAttribArray( _opacityAttrLoc );

	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	_particleTexture.unbind(0);
	_shader.unbind();
}

#pragma mark - Fluid

CLASSLOAD(Fluid);

/*
		bool _sleeping;
		cpSpace *_space;
		init _initializer;
		physics_particle_vec _physicsParticles;
		fluid_particle_vec _fluidParticles;
*/

Fluid::Fluid():
	GameObject( GameObjectType::FLUID ),
	_sleeping(false),
	_space(NULL)
{
	setName( "Fluid" );
	setLayer( RenderLayer::TERRAIN - 1 );
	setVisibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING );

	FluidRenderer *renderer = new FluidRenderer();
	renderer->setModel( this );
	addComponent( renderer );
}

Fluid::~Fluid()
{
	clear();
}

void Fluid::initialize( const init &initializer )
{
	clear();

	GameObject::initialize( initializer );
	FluidRendererModel::initialize( initializer );
	_initializer = initializer;
}

void Fluid::addedToLevel( Level *level )
{
	GameObject::addedToLevel(level);
	_space = level->space();
}

void Fluid::ready()
{
	if ( _initializer.boundingVolumeRadius > 0 )
	{
		emitCircularVolume( _initializer.particle, _initializer.boundingVolumePosition, _initializer.boundingVolumeRadius );
	}
}


void Fluid::update( const time_state &time )
{
	//
	//	Update particle lifecycles - this includes scaling radius
	//

	bool wakeupNeeded = false;
	_updateLifecycle( time, wakeupNeeded );
	
	//
	//	Make certain storage matches up
	//

	if ( _fluidParticles.size() != _physicsParticles.size() )
	{
		_fluidParticles.resize( _physicsParticles.size() );
	}

	//
	//	Early exit if we're empty
	//

	if ( _physicsParticles.empty() )
	{
		setAabb( cpBBInvalid );
		return;
	}

	//
	//	Apply clumping forces to particles
	//

	if ( _initializer.clumpingForce > 0 )
	{
		_clump( time );
	}
	

	//
	//	Update particles and AABB.
	//	If a particle is not sleeping, and its velocity is above the threshold, dampen its motion.
	//	Leave alone anything sleeping or going real slow, because we don't want to waken still particles by
	//	damping them when it's not necessary.
	//
				
	const real 
		sleepLVThreshold = 0.001,
		sleepLVThreshold2 = sleepLVThreshold * sleepLVThreshold;
	
	cpBB bounds = cpBBInvalid;
	bool sleeping = true;
	
	fluid_particle_vec::iterator fluidParticle = _fluidParticles.begin();
	physics_particle_vec::iterator physicsParticle = _physicsParticles.begin();
	
	for( physics_particle_vec::iterator end( _physicsParticles.end()); physicsParticle != end; ++fluidParticle, ++physicsParticle )
	{
		cpBody *body = physicsParticle->body;
		cpVect vel = cpBodyGetVel( body );
		real lVel2 = cpvlengthsq( vel );
		
		if ( wakeupNeeded || (!cpBodyIsSleeping( body ) && lVel2 > sleepLVThreshold2) )
		{
			sleeping = false;

			cpBodySetVel( body, cpvmult( vel, physicsParticle->linearDamping ));
			cpBodySetAngVel( body, cpBodyGetAngVel( body ) * physicsParticle->angularDamping );
		}
		
		//
		//	Synchronize fluid particle to physics particle state, and expand BB to fit
		//
		
		fluidParticle->position = v2( cpBodyGetPos( body ));
		fluidParticle->physicsRadius = physicsParticle->currentRadius;
		fluidParticle->drawRadius = physicsParticle->visualRadiusScale * physicsParticle->currentRadius;
		fluidParticle->angle = cpBodyGetAngle( body );		

		cpBBExpand( bounds, fluidParticle->position, fluidParticle->drawRadius );
	}
	
	setAabb( bounds );
	_sleeping = sleeping;
}

void Fluid::emit( const particle_init &init )
{
	physics_particle particle;

	particle.radius = init.radius;
	particle.visualRadiusScale = init.visualRadiusScale;
	particle.linearDamping = init.linearDamping;
	particle.angularDamping = init.angularDamping;
	particle.lifespan = init.lifespan;
	particle.age = 0;
	particle.entranceDuration = init.entranceDuration;
	particle.exitDuration = init.exitDuration;

	particle.currentRadius = particle.radiusForCurrentAge();

	const real 
		mass = M_PI * particle.radius * particle.radius * init.density,
		moment = cpMomentForCircle( mass, 0, particle.radius, cpvzero );

	particle.body = cpBodyNew( mass, moment );
	cpBodySetPos( particle.body, cpv( init.position ));
	cpBodySetUserData( particle.body, this );
	cpBodySetVel( particle.body, cpv( init.initialVelocity ) );
	cpSpaceAddBody( _space, particle.body );
	
	particle.shape = cpCircleShapeNew( particle.body, particle.currentRadius, cpvzero );
	cpShapeSetUserData( particle.shape, this );
	cpShapeSetLayers( particle.shape, CollisionLayerMask::FLUID );
	cpShapeSetCollisionType( particle.shape, CollisionType::FLUID );
	cpShapeSetFriction( particle.shape, init.friction );
	cpShapeSetElasticity( particle.shape, init.elasticity );
	cpSpaceAddShape( _space, particle.shape );	
	
	_physicsParticles.push_back( particle );
}

void Fluid::emitCircularVolume( const particle_init &particleInitializer, const Vec2 &volumePosition, real volumeRadius )
{
	//
	// super primitive - I'm not doing circle-packing maths!
	//

	const cpBB bounds = cpBBNewForCircle( cpv(volumePosition), volumeRadius );
	
	const real
		radius = particleInitializer.radius,
		size = radius * 2;

	particle_init init = particleInitializer;
	
	for ( real y = bounds.b - radius; y < bounds.t + radius; y += size )
	{
		for ( real x = bounds.l - radius; x < bounds.r + radius; x += size )
		{
			init.position = Vec2( x,y );
			if ( init.position.distance( volumePosition ) <= volumeRadius )
			{
				emit( init );
			}
		} 
	}	
}

void Fluid::clear()
{
	aboutToDestroyPhysics(this);

	foreach( physics_particle &p, _physicsParticles )
	{
		cpSpaceRemoveShape( _space, p.shape );
		cpShapeFree( p.shape );
		
		cpSpaceRemoveBody( _space, p.body );
		cpBodyFree( p.body );
	}
	
	_physicsParticles.clear();
	_fluidParticles.clear();
}

namespace {

	inline bool particle_alive_test( const Fluid::physics_particle &p )
	{
		return p.body != NULL;
	}

}

void Fluid::_updateLifecycle( const time_state &time, bool &wakeupNeeded )
{
	std::size_t died = 0;
	wakeupNeeded = false;

	for( physics_particle_vec::iterator 
		particle(_physicsParticles.begin()),
		end( _physicsParticles.end());
		particle != end; 
		++particle )
	{
		particle->age += time.deltaT;		
		particle->currentRadius = particle->radiusForCurrentAge();

		if ( particle->currentRadius < particle->radius ) 
		{
			wakeupNeeded = true;
		}

		// verified in chipmunk src that this is a cheap operation
		cpCircleShapeSetRadius( particle->shape, particle->currentRadius );
		
		if ( particle->age > particle->lifespan )
		{
			died++;
			wakeupNeeded = true;

			cpSpaceRemoveShape( _space, particle->shape );
			cpShapeFree( particle->shape );
			particle->shape = NULL;
			
			cpSpaceRemoveBody( _space, particle->body );
			cpBodyFree( particle->body );
			particle->body = NULL;
		}
	}
	
	if ( died > 0 )
	{
		_physicsParticles.erase( 
			std::partition( _physicsParticles.begin(), _physicsParticles.end(), particle_alive_test ),
			_physicsParticles.end() );
	}	
}

void Fluid::_clump( const time_state &time )
{
	//
	//	we're delegating to cpSpaceBBQuery - it has no benefit for small numbers of particles,
	//	but profiling shows that when applied to ~600 particles, CPU use hovers at .1%, compared 
	//	to 18% for the brute force approach.
	//	NOTE: Since clumping is applied to each particle equally, we apply 1/2 the force
	//

	const real clumpingForce = _initializer.clumpingForce * 0.5;

	clump_callback_info info;	
	info.fluid = this;

	cpBB testBounds;

	for ( physics_particle_vec::iterator 
		particle(_physicsParticles.begin()), 
		end( _physicsParticles.end()); 
		particle != end; 
		++particle )
	{
		info.testBody = particle->body;
		info.testShape = particle->shape;
		info.testShapeRadius = particle->currentRadius;
		info.clumpImpulse = clumpingForce * time.deltaT;

		cpBBNewCircle( testBounds, cpBodyGetPos( particle->body ), info.testShapeRadius * 2 );

		cpSpaceBBQuery( _space, testBounds, CollisionLayerMask::FLUID, CP_NO_GROUP, clumpCallback, &info );
	}
}


} // end namespace game
