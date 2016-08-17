//
//  Urchin.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 12/10/11.
//  Copyright (c) 2011 __MyCompanyName__. All rights reserved.
//

#include "Urchin.h"
#include "FastTrig.h"
#include "Fronds.h"

#include <chipmunk/chipmunk_unsafe.h>

using namespace ci;
using namespace core;
namespace game {

CLASSLOAD(Urchin);

/*
		cpBody *_body;

		cpShape *_segmentShape;
		cpShapeSet _shapes;
		init _initializer;
		Vec2r _position, _up;	
		real _speed, _radius;

		bool _flipped;
		seconds_t _timeFlipped;	
		
		FrondEngine *_whiskers;
*/

Urchin::Urchin():
	Monster( GameObjectType::URCHIN ),
	_body(NULL),
	_segmentShape(NULL),
	_position(0,0),
	_up(0,1),
	_speed(0),
	_radius(0),
	_flipped(false),
	_timeFlipped(0),
	_whiskers(NULL)
{
	setName( "Urchin" );
	addComponent( new UrchinRenderer() );
	setController( new UrchinController() );
}

Urchin::~Urchin()
{
	aboutToDestroyPhysics(this);		
	cpCleanupAndFree(_shapes);
	cpCleanupAndFree(_body);
}

void Urchin::initialize( const init &initializer )
{
	Monster::initialize( initializer );
	_initializer = initializer;
}

void Urchin::addedToLevel( Level *level )
{
	Monster::addedToLevel( level );

	//
	//	Physics form is nothing but a rounded segment
	//

	const real
		Length = _initializer.size,
		Radius = _initializer.size * 0.5,
		Density = 10,
		Mass = Length * Radius * 2 * Density,
		Moment = cpMomentForBox(Mass, Length, Radius * 2);
		
	_centerOfMassOffset.x = 0;
	_centerOfMassOffset.y = Radius/2;

	_body = cpBodyNew(Mass, Moment);
	cpBodySetUserData( _body, this );
	cpBodySetPos( _body, cpv( _initializer.position ));
	cpSpaceAddBody( space(), _body );

	_radius = Radius;
	_segmentShape = cpSegmentShapeNew(
		_body, 
		cpv(-Length/2 + _centerOfMassOffset.x, _centerOfMassOffset.y ), 
		cpv( Length/2 + _centerOfMassOffset.x, _centerOfMassOffset.y ), 
		0 );

	_shapes.insert( _segmentShape );

	foreach( cpShape *shape, _shapes )
	{
		cpShapeSetCollisionType( shape, CollisionType::MONSTER );
		cpShapeSetLayers( shape, CollisionLayerMask::MONSTER );
		cpShapeSetUserData( shape, this );
		cpShapeSetFriction( shape, 2 );
		cpShapeSetElasticity( shape, 0 );
		cpShapeSetGroup( shape, cpGroup(this));
		cpSpaceAddShape( space(), shape );
	}
		
	if ( true )
	{
		_whiskers = new FrondEngine();
		FrondEngine::init frondEngineInit;
		frondEngineInit.color = ci::ColorA(0.1,0.1,0.1,1);
		_whiskers->initialize( frondEngineInit );
		addChild( _whiskers );

		FrondEngine::frond_init frondInit;
		frondInit.count = 3;
		frondInit.length = _initializer.size * 2;
		frondInit.thickness = frondInit.length * 0.1;

		for ( int i = 0, N = 16; i < N; i++ )
		{
			const real Rads = Rand::randFloat(-0.1,0.1) + (M_PI * static_cast<real>(i) / N);
			const Vec2r Dir( std::cos( Rads ), std::sin( Rads ));
									
			frondInit.length = _initializer.size * Rand::randFloat( 1.75, 2.25 );
			_whiskers->addFrond( frondInit, _body, v2r(cpBodyGetPos(_body)) + _centerOfMassOffset + Dir * 0.1, Dir );
		}
	}	
}

void Urchin::update( const time_state &time )
{
	Monster::update( time );

	Vec2r dir = v2r( cpBodyGetRot( _body ));
	_up = rotateCCW( dir );
	_position = v2r( cpBodyGetPos( _body )) + _centerOfMassOffset;	
	_speed = lrp<real>( 0.1, _speed, ((controller() ? controller()->direction().x : 0) * _initializer.speed ) );
	
	cpBodySetAngVel( _body, cpBodyGetAngVel( _body ) * 0.98 );
	
	//
	//	Check if we are flipped onto back - and if so increment time counter
	//

	{
		const seconds_t TimeThresholdForFlip = 2;
	
		if( _up.y < 0 )
		{
			_timeFlipped += time.deltaT;
			if ( _timeFlipped > TimeThresholdForFlip )
			{
				_timeFlipped = TimeThresholdForFlip;
				_flipped = true;
			}
			
			if ( _flipped )
			{
				//
				//	apply off-center force to kick into air with rotation -- we're scaling by how flipped
				//	we are, so as to prevent spinning.
				//

				const float
					ForceScaling = _up.y * _up.y * 0.5;

				const cpVect 
					Force = cpvmult(cpSpaceGetGravity(space()), ForceScaling * -1 * time.deltaT * cpBodyGetMass( _body ) ),
					Offset = cpv(_initializer.size * 4 * (instanceId() % 2 ? 1 : -1),0);

				cpBodyApplyImpulse(_body, Force, Offset );
			}
		}
		else
		{
			_timeFlipped -= time.deltaT;
			
			if ( _timeFlipped <= 0 )
			{
				_timeFlipped = 0;
				_flipped = false;
			}			
		}
	}
	
	//
	//	Motion!
	//
	
	if ( alive() )
	{
		cpVect sv = cpv( dir * _speed );
		cpShapeSetSurfaceVelocity( _segmentShape, sv );
	}

	//
	//	Apply lifecycle size and color scaling
	//
	
	if ( lifecycle() < 1 )
	{
		cpSegmentShapeSetRadius(_segmentShape, _radius * lifecycle());
	
		if ( _whiskers )
		{
			ci::ColorA color = _whiskers->color();
			color.a = lifecycle();
			_whiskers->setColor( color );
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

void Urchin::died( const HealthComponent::injury_info &info )
{
	foreach( cpShape *shape, _shapes )
	{
		cpShapeSetLayers( shape, CollisionLayerMask::DEAD_MONSTER );
	}

	Monster::died( info );
}

#pragma mark - UrchinRenderer

/*
		ci::gl::GlslProg _shader;
		core::SvgObject _svg;
		Vec2r _svgScale;
		real _velocityAccumulator, _aggressionAccumulator, _fearAccumulator;
*/

UrchinRenderer::UrchinRenderer():
	_svgScale(1,1),
	_velocityAccumulator(0),
	_aggressionAccumulator(0),
	_fearAccumulator(0)
{}

void UrchinRenderer::_drawGame( const render_state &state )
{
	Urchin *urchin = this->urchin();

	if ( !_shader )
	{
		_shader = level()->resourceManager()->getShader( "SvgPassthroughShader" );
	}
	
	if ( !_svg )
	{
		_svg = level()->resourceManager()->getSvg( "Urchin.svg" );	
		_urchin = _svg.find( "/urchin" );	
		assert(_urchin);

		foreach( core::SvgObject eye, _urchin.find("eyes").children())
		{
			_eyes.push_back(eye);
		}
			
		assert( !_eyes.empty() );
		

		Vec2r documentSize = _svg.documentSize();
		real documentRadius = std::max( documentSize.x, documentSize.y ) * 0.5;
		_svgScale.x = _svgScale.y = 1.75 * urchin->size() / documentRadius;
	}
	
	//
	//	Blink eyes
	//
	
	{
		// phase goes smoothly from 0->1, 0->1
		const real 
			Fudge = urchin->fudge(),
			Sign = Fudge > 1 ? 1 : -1,
			DefaultPeriod = 2 * Fudge,
			AggressivePeriod = 0.25 * Fudge,
			DefaultPhase = fract<real>( Sign * state.time / DefaultPeriod),
			AggressivePhase = fract<real>( state.time / AggressivePeriod),
			Phase = lrp<real>(std::max( urchin->controller()->aggression(),urchin->controller()->fear()), DefaultPhase, AggressivePhase ),
			Fractional = static_cast<real>(1)/_eyes.size(),
			Range = 2 * Fractional;

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
	
	//
	//	Pulse body to reflect aggression and fear
	//
	
	{
		const real
			DefaultPeriod = 1 * urchin->fudge(),
			DefaultPhase = std::cos( state.time / DefaultPeriod ),
			AggressivePeriod = 0.125 * urchin->fudge(),
			AggressivePhase = std::cos( state.time / AggressivePeriod );
	
		Vec2r scale(1,1);
		scale.x = 1 - 0.125 * lrp<real>( urchin->controller()->aggression(), DefaultPhase, AggressivePhase );
		scale.y = 1 + 0.125 * lrp<real>( urchin->controller()->aggression(), DefaultPhase, AggressivePhase );
	
		_urchin.setScale(scale);
	}
	
	
		
		
	//
	//	Finally, draw the damn thing
	//
		
	const real Angle = cpBodyGetAngle( urchin->body() );
	const Vec2r Scale = _svgScale * urchin->lifecycle();
		
	_shader.bind();
	
	_svg.setPosition( urchin->position());
	_svg.setScale( Scale );
	_svg.setAngle( Angle );
	_svg.setOpacity( urchin->dead() ? urchin->lifecycle() : 1 );
	
	_svg.draw( state );
	
	
	_shader.unbind();
}

void UrchinRenderer::_drawDebug( const render_state &state )
{
	Urchin *beetle = (Urchin*) owner();
	
	_debugDrawChipmunkShapes( state, beetle->shapes() );
	_debugDrawChipmunkBody( state, beetle->body() );
}

#pragma mark - UrchinController

UrchinController::UrchinController()
{
	setName( "UrchinController" );
}

void UrchinController::update( const time_state &time )
{
	GroundBasedMonsterController::update( time );
}



} // end namespace game