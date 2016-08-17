//
//  Barnacle.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 1/11/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Barnacle.h"

#include "ChipmunkDebugDraw.h"
#include "Fronds.h"
#include "GameConstants.h"
#include "GameNotifications.h"
#include "Player.h"
#include "Spline.h"

#include <chipmunk/chipmunk_unsafe.h>

using namespace ci;
using namespace core;
namespace game {

CLASSLOAD(Barnacle);

/*
		friend class BarnacleRenderer;

		init _initializer;

		cpBody *_body;
		cpBodySet _bodies;
		cpShapeSet _shapes;
		cpConstraintSet _constraints;

		Vec2r _position, _up;
		real _speed, _footRadius;
		bool _deathAcidDispatchNeeded;

		FrondEngine *_whiskers;
		Tongue *_tongue;
*/

Barnacle::Barnacle():
	Monster( GameObjectType::BARNACLE ),
	_body(NULL),
	_position(0,0),
	_up(0,1),
	_speed(0),
	_footRadius(0),
	_deathAcidDispatchNeeded(false),
	_whiskers(NULL),
	_tongue(NULL)
{
	setName( "Barnacle" );
	addComponent( new BarnacleRenderer() );
	setController( new BarnacleController() );
}

Barnacle::~Barnacle()
{
	if ( space() )
	{	
		aboutToDestroyPhysics(this);
		
		cpCleanupAndFree( _constraints );
		cpCleanupAndFree( _shapes );
		cpCleanupAndFree( _bodies );					
	}
}

void Barnacle::initialize( const init &initializer )
{
	Monster::initialize( initializer );
	_initializer = initializer;
}

void Barnacle::addedToLevel( Level *level )
{
	Monster::addedToLevel( level );
	
	//
	//	Make three circles in an upside-down equilateral triangle, with center of gravity at (topside) base
	//

	const real 
		BodyCircleRadius = _initializer.size * 0.5,
		BodyTriangleHeight = std::sqrt( std::pow( 2*BodyCircleRadius, 2 ) - BodyCircleRadius ),
		BodyTriangleYOffset = BodyCircleRadius;
		
	const Vec2r
		TongueAttachmentPoint = _initializer.position + Vec2r(0,-(BodyTriangleHeight+BodyCircleRadius));

	real totalMass = 0;

	{
		_footRadius = BodyCircleRadius;

		const cpVect shapeOffsets[3] = {
			cpv( 0, -BodyTriangleHeight - BodyTriangleYOffset ),
			cpv( -BodyCircleRadius, -BodyTriangleYOffset ),
			cpv( +BodyCircleRadius, -BodyTriangleYOffset )
		}; 
		
		//
		//	create body and shapes
		//

		const real
			Density = 100,
			Mass = M_PI * BodyCircleRadius * BodyCircleRadius * Density,
			Moment =
				cpMomentForCircle( Mass, 0, BodyCircleRadius, shapeOffsets[0] ) + 
				cpMomentForCircle( Mass, 0, BodyCircleRadius, shapeOffsets[1] ) + 
				cpMomentForCircle( Mass, 0, BodyCircleRadius, shapeOffsets[2] ); 
			
		_body = cpBodyNew( Mass, Moment );
		cpBodySetPos( _body, cpv( _initializer.position ));
		_bodies.insert( _body );


		cpShape *shape = cpCircleShapeNew( _body, BodyCircleRadius, shapeOffsets[0]);
		_shapes.insert( shape );

		shape = cpCircleShapeNew( _body, BodyCircleRadius, shapeOffsets[1]);
		_shapes.insert( shape );
		
		shape = cpCircleShapeNew( _body, BodyCircleRadius, shapeOffsets[2] );
		_shapes.insert( shape );
		
		totalMass += Mass;
	}

	foreach( cpBody *body, _bodies )
	{
		cpBodySetUserData( body, this );
		cpSpaceAddBody( space(), body );
	}

	foreach( cpShape *shape, _shapes )
	{
		cpShapeSetUserData( shape, this );
		cpShapeSetCollisionType( shape, CollisionType::MONSTER );
		cpShapeSetLayers( shape, CollisionLayerMask::MONSTER );
		cpShapeSetGroup( shape, cpGroup(this));
		cpSpaceAddShape( space(), shape );
	}
	
	foreach( cpShape *shape, _shapes )
	{
		cpShapeSetFriction( shape, 1.5 );
		cpShapeSetElasticity( shape, 0.5 );
	}

	foreach( cpConstraint *constraint, _constraints )
	{
		cpConstraintSetUserData( constraint, this );
		cpSpaceAddConstraint( space(), constraint );
	}
	
	//
	//	Apply anti-gravity impulse to make it stick to ceiling
	//
	
	cpBodyApplyForce( _body, cpvmult( cpSpaceGetGravity( space() ), -2 * totalMass ), cpvzero );
			
	//
	//	Create tongue
	//
	
	if ( true )
	{
		Tongue::init tongueInit;
		static_cast<Monster::init>(tongueInit) = static_cast<Monster::init>(_initializer);
		tongueInit.position = TongueAttachmentPoint;
		tongueInit.thickness = _initializer.size * 0.25;
		tongueInit.length = _initializer.size * 16;
		tongueInit.color = _initializer.color;
	
		_tongue = new Tongue();
		_tongue->initialize( tongueInit );
		_tongue->setLayer( this->layer() - 1 ); // draw behind me
		_tongue->attachTo( this, _body, TongueAttachmentPoint );
	}
			

	//
	//	Create whiskers
	//

	if ( true )
	{
		_whiskers = new FrondEngine();
		FrondEngine::init frondEngineInit;
		frondEngineInit.color = _initializer.color;
		_whiskers->initialize( frondEngineInit );
		_whiskers->setLayer( this->layer() - 1 );
		addChild( _whiskers );

		
		if ( true )
		{
			FrondEngine::frond_init frondInit;
			frondInit.count = 3;

			for ( int i = 0, N = 16; i < N; i++ )
			{
				const real Rads = Rand::randFloat(-0.1,0.1) - (M_PI * static_cast<real>(i) / N);

				const Vec2r 
					Dir( std::cos( Rads ), std::sin( Rads ));
										
				frondInit.length = _initializer.size * 2 * Rand::randFloat( 0.8, 1.2 );
				frondInit.thickness = frondInit.length * 0.1 * Rand::randFloat( 0.8, 1.2 );
				frondInit.stiffness = 0.5;//Rand::randFloat(1)

				_whiskers->addFrond( frondInit, _body, v2r(cpBodyGetPos(_body)) + Dir * _initializer.size * 0.5, Dir );
			}
		}
	}
}

void Barnacle::update( const time_state &time )
{
	BarnacleController *controller = (BarnacleController *) this->controller();
	Player *player = controller->player();

	Monster::update( time );
	
	//
	//	We died, burp out some acid blood
	//

	if ( _deathAcidDispatchNeeded )
	{
		//
		//	emit what acid we have available
		//

		for ( int i = 0, N = 16 * _initializer.size * _initializer.size; i < N; i++ )
		{
			((GameLevel*)level())->emitAcid( 
				_position + Rand::randVec2f() * Rand::randFloat(0.5), 
				Vec2r( (Rand::randBool() ? -1 : 1) * Rand::randFloat(2), 0 ) );
		}
		
		_deathAcidDispatchNeeded = false;
	}

	real 
		lifecycle = this->lifecycle(),
		aggression = 0,
		fear = 0,
		direction = 0;
		
	if ( controller )
	{
		aggression = controller->aggression();
		fear = controller->fear();
		direction = controller->direction().x;
	}

	Vec2r dir = v2r( cpBodyGetRot( _body ));
	_up = rotateCCW( dir );
	_position = v2r( cpBodyGetPos( _body ));	
	
	cpBodySetAngVel( _body, cpBodyGetAngVel( _body) * 0.975 );
	
	//
	//	Animate lifecycle
	//
	
	
	if ( lifecycle < 1 )
	{
		const real 
			footRadius = _footRadius * lifecycle;
			
		for( cpShapeSet::iterator 
			shape( _shapes.begin()),end( _shapes.end() );
			shape != end;
			++shape )
		{
			cpCircleShapeSetRadius( *shape, footRadius );
		}

		if ( _whiskers )
		{
			ci::ColorA color = _initializer.color;
			color.a *= lifecycle;
			_whiskers->setColor( color );
		}
	}
		
	//
	//	Move
	//

	if ( alive() )
	{
		real
			oscillationPeriod = fudge() * 2,
			oscillation = std::sin( M_PI * time.time / oscillationPeriod ) + 0.25*std::sin( M_PI_2 * time.time / (2*oscillationPeriod) );
	
		_speed = lrp<real>( 0.01, _speed, direction * _initializer.speed );
		cpVect sv = cpv( dir * (_speed + oscillation));
		
		for( cpShapeSet::const_iterator shape(_shapes.begin()), end(_shapes.end()); shape != end; ++shape )
		{
			cpShapeSetSurfaceVelocity( *shape, sv );
		}
		
	}
	
	//
	//	Update AABB
	//
	
	cpBB bounds = cpBBInvalid;
	for( cpShapeSet::const_iterator shape(_shapes.begin()), end(_shapes.end()); shape != end; ++shape )
	{
		cpBBExpand( bounds, cpShapeGetBB( *shape ));
	}	

	setAabb( bounds );
}

void Barnacle::died( const HealthComponent::injury_info &info )
{	
	_deathAcidDispatchNeeded = true;

	foreach( cpShape *shape, _shapes )
	{
		cpShapeSetLayers( shape, CollisionLayerMask::DEAD_MONSTER );
	}

	Monster::died( info );
}

#pragma mark - BarnacleRenderer

/*
		GLuint _vbo;
		ci::gl::GlslProg _shader;
		core::SvgObject _svg, _bulb, _body;
		std::vector< core::SvgShape* > _eyes;

		spline_rep _splineRep;
		vertex_vec _triangleStrip;

		Vec2f _svgScale;
*/

BarnacleRenderer::BarnacleRenderer():
	_vbo(0)
{}

BarnacleRenderer::~BarnacleRenderer()
{
	if ( _vbo )
	{
		glDeleteBuffers( 1, &_vbo );
	}
}

void BarnacleRenderer::draw( const render_state &state )
{
	DrawComponent::draw( state );
}

void BarnacleRenderer::_drawGame( const render_state &state )
{
	if ( !_shader )
	{
		_shader = level()->resourceManager()->getShader( "SvgPassthroughShader" );
	}
	
	_shader.bind();

	glEnable( GL_BLEND );
	_drawBody( state );
	
	_shader.unbind();
}

void BarnacleRenderer::_drawDebug( const render_state &state )
{
	Barnacle *barnacle = this->barnacle();

	_debugDrawChipmunkShapes( state, barnacle->shapes() );
	_debugDrawChipmunkConstraints( state, barnacle->constraints() );
	_debugDrawChipmunkBodies( state, barnacle->bodies() );
}

void BarnacleRenderer::_drawBody( const render_state &state )
{
	Barnacle *barnacle = this->barnacle();
	const real lifecycle = barnacle->lifecycle();

	if ( !_svg )
	{
		_svg = level()->resourceManager()->getSvg( "Barnacle.svg" );

		Vec2r documentSize = _svg.documentSize();
		real documentRadius = std::max( documentSize.x, documentSize.y ) * 0.5;
		_svgScale.x = _svgScale.y = barnacle->size() / documentRadius;

		_bulb = _svg.find( "bulb" );
		assert( _bulb );

		_base = _svg.find( "root" );
		assert( _base );

		foreach( core::SvgObject eye, _bulb.find("eyes").children())
		{
			_eyes.push_back(eye);
		}
			
		assert( !_eyes.empty() );

	}

	//
	//	Blink eyes
	//
	
	{
		// phase goes smoothly from 0->1, 0->1
		const real 
			Fudge = barnacle->fudge(),
			Sign = Fudge > 1 ? 1 : -1,
			DefaultPeriod = 2 * Fudge,
			AggressivePeriod = 0.25 * Fudge,
			DefaultPhase = fract<real>( Sign * state.time / DefaultPeriod),
			AggressivePhase = fract<real>( state.time / AggressivePeriod),
			Phase = lrp<real>(std::max( barnacle->controller()->aggression(),barnacle->controller()->fear()), DefaultPhase, AggressivePhase ),
			Fractional = static_cast<real>(1)/_eyes.size(),
			Range = 1 * Fractional;

		real
			Midpoint = Fractional/2;
				
		for ( std::vector< core::SvgObject >::iterator eye( _eyes.begin()), end(_eyes.end()); eye != end; ++eye )
		{
			//
			//	The closer Phase is to the midway point, the brighter the eye is
			//
			
			const real 
				Dist = distance( Phase, Midpoint ),
				Dist2 = distance( Phase - 1, Midpoint ), 
				Intensity = 1 - saturate(Dist / Range),
				Intensity2 = 1 - saturate(Dist2 / Range),
				Alpha = lrp<real>(Intensity, 0.125, 1 ) + lrp<real>(Intensity2, 0.125, 1 );
			
			eye->setOpacity(Alpha);

			Midpoint += Fractional;
		}
	}
	
			
	const real
		Agression = barnacle->controller()->aggression(),
		Fear = barnacle->controller()->fear(),
		SquishPhase = state.time * M_PI * barnacle->fudge(),
		SquishCycle = std::cos( SquishPhase ),
		WidthPulse = lrp<real>( Fear, SquishCycle * 0.125 + 1, 0.875 ),
		HeightPulse = lrp<real>( Fear, (1-SquishCycle) * 0.125 + 1, 0.5 );
	
	
	_svg.setPosition( barnacle->position() );
	_svg.setScale( _svgScale * lifecycle );
	_svg.setAngle( cpBodyGetAngle( barnacle->body() ) );
	_svg.setOpacity( barnacle->dead() ? lifecycle : 1 );

	// apply squish cycle to body
	_bulb.setScale( Vec2r(WidthPulse, HeightPulse) );


	_svg.draw( state );	
}

#pragma mark - BarnacleController

BarnacleController::BarnacleController()
{
	setName( "BarnacleController" );
}

void BarnacleController::update( const time_state &time )
{
	GroundBasedMonsterController::update( time );
}

void BarnacleController::_getMotionProbeRays( cpVect &leftOrigin, cpVect &leftEnd, cpVect &rightOrigin, cpVect &rightEnd )
{
	cpBody *body = monster()->body();
	cpBB bounds = monster()->aabb();
	real 
		radius = barnacle()->initializer().size,
		extentX = 2 * radius,
		extentY = 2 * radius;

	Vec2r center(0,0);


	leftOrigin = cpBodyLocal2World( body, cpv(center.x - extentX, center.y - extentY ));
	leftEnd = cpBodyLocal2World( body, cpv( center.x - extentX, center.y + extentY ));

	rightOrigin = cpBodyLocal2World( body, cpv(center.x + extentX, center.y - extentY ));
	rightEnd = cpBodyLocal2World( body, cpv( center.x + extentX, center.y + extentY ));	
}



} // end namespace game