//
//  PowerCell.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 6/15/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "PowerCell.h"

#include "Fluid.h"
#include "GameLevel.h"
#include "GameConstants.h"

#include <cinder/gl/gl.h>


using namespace ci;
using namespace core;
namespace game {

CLASSLOAD(PowerCell);

/*
		friend class PowerCellRenderer;
	
		init _initializer;
		cpBody *_body;
		cpShape *_shape;
		bool _burned;
		int _charge;
		real _opacity;

		UniversalParticleSystemController::Emitter *_fireEmitter;
*/

PowerCell::PowerCell():
	GameObject( GameObjectType::POWERCELL ),
	_body(NULL),
	_shape(NULL),
	_burned(false),
	_charge(0),
	_opacity(1),
	_fireEmitter(NULL)
{
	setName( "PowerCell" );
	setLayer(RenderLayer::ENVIRONMENT);
	addComponent( new PowerCellRenderer());
	setVisibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING );
}

PowerCell::~PowerCell()
{
	aboutToDestroyPhysics(this);
	cpShapeCleanupAndFree( &_shape );
	cpBodyCleanupAndFree( &_body );
}
		
void PowerCell::initialize( const init &i )
{
	GameObject::initialize(i);
	_initializer = i;
	_charge = _initializer.charge;
}
		
void PowerCell::addedToLevel( Level *level )
{
	const real 
		Width = _initializer.size,
		Height = _initializer.size,
		Density = 500, 
		Mass = Width * Height * Density,
		Moment = cpMomentForBox( Mass, Width, Height );
		
	_body = cpBodyNew(Mass, Moment);
	cpBodySetPos( _body, cpv(_initializer.position) );	
	
	_shape = cpBoxShapeNew( _body, Width, Height );
	cpShapeSetUserData( _shape, this );	
	cpShapeSetCollisionType( _shape, CollisionType::DECORATION );
	cpShapeSetLayers( _shape, CollisionLayerMask::POWERUP );
	cpShapeSetFriction( _shape, 1 );
	cpShapeSetElasticity( _shape, 0 );

	cpSpace *space = level->space();
	cpSpaceAddBody( space, _body );
	cpSpaceAddShape( space, _shape );
	
	level->collisionDispatcher()->postSolve(CollisionType::FLUID, CollisionType::DECORATION, NULL, this )
		.connect( this, &PowerCell::_contactFluid );
}

void PowerCell::ready()
{
	//
	//	Get an emitter for the global fire effect
	//
	
	GameLevel *gameLevel = static_cast<GameLevel*>(level());
	_fireEmitter = gameLevel->fireEffect()->emitter(16);	
}

void PowerCell::update( const time_state &time )
{
	if ( _burned && _charge < Epsilon )
	{
		if ( _opacity < ALPHA_EPSILON )
		{
			setFinished(true);
		}
		else
		{
			_opacity *= 0.95;
		}	
	}

	setAabb( cpShapeGetBB( _shape ));
}

int PowerCell::discharge()
{
	if ( _charge > 0 )
	{
		onDischarge(this);

		real dischargeAmount = _charge;
		_charge = 0;
	
		return static_cast<int>(dischargeAmount);
	}
	
	return 0;
}

void PowerCell::_contactFluid( const collision_info &info )
{
	if ( _charge > 0 )
	{
		assert( (PowerCell*)info.b == this );
		Fluid *fluid = (Fluid*) info.a;
		
		if ( (fluid->attackStrength() > 0) && (fluid->injuryType() == InjuryType::LAVA))
		{
			_burned = true;
			
			const real 
				Injury = level()->time().deltaT * fluid->attackStrength();

			_charge = std::max<real>( _charge - Injury, 0 );
			
			const Vec2 Pos = v2(cpBodyGetPos( _body ));
			const real Size = _initializer.size * 0.75;
			
			for ( int i = 0, N = info.contacts(); i < N; i++ )
			{
				_fireEmitter->emit( 1, Pos + Vec2( Rand::randFloat( -Size , Size ), Rand::randFloat( -Size, Size )));
			}
		}
	}
}

#pragma mark - PowerCellRenderer

/*
		ci::gl::GlslProg _svgShader;
		SvgObject _svg, _flickeringFace, _flickeringHalo;
		Vec2 _svgScale;
		real _intensity, _dischargeFlare, _pulseOffset, _pulseFrequency;
		seconds_t _dischargeTime;
*/

PowerCellRenderer::PowerCellRenderer():
	_dischargeFlare(0),
	_pulseOffset( Rand::randFloat(M_PI) ),
	_pulseFrequency( 0.25 ),
	_dischargeTime(-1)
{}

PowerCellRenderer::~PowerCellRenderer()
{}

void PowerCellRenderer::addedToGameObject( GameObject *obj )
{
	powerCell()->onDischarge.connect( this, &PowerCellRenderer::_powerCellDischarged );
}

void PowerCellRenderer::update( const time_state &time )
{
	if ( _dischargeTime > 0 )
	{
		_dischargeFlare = lrp<real>( 0.2, _dischargeFlare, 1 );
	}
	else
	{
		_dischargeFlare = 0;
	}	
}

void PowerCellRenderer::_drawGame( const render_state &state )
{
	PowerCell *powerCell = this->powerCell();

	if ( !_svgShader )
	{
		_svgShader = level()->resourceManager()->getShader( "SvgPassthroughShader" );
		_svg = level()->resourceManager()->getSvg( "PowerCell.svg" );
		
		_flickeringFace = _svg.find("Face");
		assert( _flickeringFace );

		_flickeringHalo = _svg.find("Halo");
		assert( _flickeringHalo );
		
		// powercell is square
		Vec2 documentSize = _svg.documentSize();
		_svgScale.x = _svgScale.y = powerCell->initializer().size / documentSize.x;
	}

	_svgShader.bind();
	
	_svg.setPosition( v2( cpBodyGetPos(powerCell->body()) ));
	_svg.setScale( _svgScale );
	_svg.setAngle( cpBodyGetAngle( powerCell->body() ));
	_svg.setOpacity( powerCell->_opacity );
	
	const real 
		Charge = static_cast<real>(powerCell->charge()) / static_cast<real>(powerCell->initializer().charge),
		Flicker = _currentFlickerValue();

	real
		faceOpacity = lrp<real>(Charge, 0.1, 1 ),
		haloOpacity = Charge * lrp<real>(Flicker, 0.125,0.25),
		haloScale = Charge * lrp<real>( Flicker, 1, 1.75 );

	_flickeringFace.setOpacity( faceOpacity );
		
	if ( _dischargeFlare < 0.5 ) haloOpacity = lrp<real>( _dischargeFlare / 0.5, haloOpacity, 0.5 );
	else haloOpacity = lrp<real>( (_dischargeFlare - 0.5)/0.5, 0.5, 0 );

	haloScale = lrp<real>( _dischargeFlare, haloScale, 8 );

	_flickeringHalo.setOpacity( haloOpacity );
	_flickeringHalo.setScale( haloScale );

	
	gl::enableAlphaBlending();
	_svg.draw( state );
	
	
	_svgShader.unbind();
}

void PowerCellRenderer::_drawDebug( const render_state &state )
{
	PowerCell *powerCell = this->powerCell();
	Mat4 R;
	cpBody_ToMatrix( powerCell->body(), R );
	
	real hs = powerCell->initializer().size * 0.5;
	
	gl::pushMatrices();
	gl::multModelView( R );
	
		gl::color( powerCell->debugColor() );
		gl::drawSolidRect( Rectf( -hs, -hs, +hs, +hs ));
		
	gl::popMatrices();
}

void PowerCellRenderer::_powerCellDischarged( PowerCell * )
{
	_dischargeTime = level()->time().time;
}

real PowerCellRenderer::_currentFlickerValue()
{
	real flicker = std::abs( std::sin( _pulseOffset + (level()->time().time * M_PI * _pulseFrequency)));
	return flicker * flicker * flicker;
}

} // end namespace game
