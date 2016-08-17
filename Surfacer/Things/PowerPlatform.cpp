//
//  PowerPlatform.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/4/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "PowerPlatform.h"
#include "GameConstants.h"
#include "PowerCell.h"

using namespace ci;
using namespace core;

#define ENABLE_DEBUG_KEYBOARD_CONTROL 1

namespace game {

namespace {

	#if ENABLE_DEBUG_KEYBOARD_CONTROL
	class PowerPlatformInputComponent : public core::InputComponent
	{
		public:
		
			PowerPlatformInputComponent()
			{
				monitorKey( app::KeyEvent::KEY_UP );
				monitorKey( app::KeyEvent::KEY_DOWN );
				takeFocus();
			}
			virtual ~PowerPlatformInputComponent(){}

			virtual void monitoredKeyDown( int keyCode )
			{
				PowerPlatform *platform = (PowerPlatform*)owner();
				switch( keyCode )
				{
					case app::KeyEvent::KEY_UP:
						platform->setChargeLevel( platform->chargeLevel() + 1 );
						break;

					case app::KeyEvent::KEY_DOWN:
						platform->setChargeLevel( platform->chargeLevel() - 1 );
						break;
				}
			}
		
	};
	#endif

}

CLASSLOAD(PowerPlatform);

/*
		init _initializer;
		cpBody *_body;
		cpShape *_shape;
		real _chargeLevel, _maxChargeLevel;
		
		PowerCell *_currentPowerCell;
		int _injestionPhase;
*/

PowerPlatform::PowerPlatform():
	GameObject( GameObjectType::POWERUP ),
	_body(NULL),
	_shape(NULL),
	_chargeLevel(0),
	_maxChargeLevel(0),
	_currentPowerCell(NULL),
	_injestionPhase(0)
{
	setName( "PowerPlatform" );
	
	// render in front of PowerCells, which are at ENVIRONMENT layer
	setLayer(RenderLayer::ENVIRONMENT + 1 );
	addComponent( new PowerPlatformRenderer());
	setVisibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING );

	#if ENABLE_DEBUG_KEYBOARD_CONTROL
	addComponent( new PowerPlatformInputComponent() );
	#endif

	// default initialization
	initialize( init() );
}

PowerPlatform::~PowerPlatform()
{
	for( std::set< PowerCell* >::iterator powerup(_touchingPowerCells.begin()), end( _touchingPowerCells.end()); 
		powerup != end; 
		++powerup )
	{
		(*powerup)->aboutToDestroyPhysics.disconnect(this);
	}

	if ( _currentPowerCell )
	{
		_currentPowerCell->aboutToDestroyPhysics.disconnect(this);
	}

	aboutToDestroyPhysics(this);
	cpCleanupAndFree(_shape);
	cpCleanupAndFree(_body);
}

void PowerPlatform::initialize( const init &i ) 
{
	GameObject::initialize(i);
	_initializer = i; 
	_maxChargeLevel = _initializer.maxChargeLevel;
	_chargeLevel = std::min<int>( _initializer.chargeLevel, _maxChargeLevel );
}
		
void PowerPlatform::addedToLevel( core::Level *level )
{
	_body = cpBodyNewStatic();
	cpBodySetPos( _body, cpv(_initializer.position) );	
	
	_shape = cpBoxShapeNew( _body, _initializer.size.x, _initializer.size.y );
	cpShapeSetUserData( _shape, this );	
	cpShapeSetCollisionType( _shape, CollisionType::STATIC_DECORATION );
	cpShapeSetLayers( _shape, CollisionLayerMask::POWER_PLATFORM );
	cpShapeSetFriction( _shape, 1 );
	cpShapeSetElasticity( _shape, 0 );

	cpSpace *space = level->space();
	cpSpaceAddShape( space, _shape );

	// PowerPlatform doesn't move, so we only need to do this once
	setAabb( cpShapeGetBB( _shape ));

	//
	//	Listen for contact with powerups, whose collision type is DECORATION, we'll filter in _contactPowerCell
	//

	CollisionDispatcher::signal_set &powerupContactDispatcher =
		level->collisionDispatcher()->dispatch( 
			CollisionType::STATIC_DECORATION, 
			CollisionType::DECORATION,
			this,
			NULL );
		
	powerupContactDispatcher.postSolve.connect( this, &PowerPlatform::_touchingPowerCell );		
	powerupContactDispatcher.contact.connect( this, &PowerPlatform::_contactPowerCell );	
	powerupContactDispatcher.separate.connect( this, &PowerPlatform::_separatePowerCell );	
}

void PowerPlatform::update( const core::time_state &time )
{
	//
	//	Keep any touching powerups awake
	//	

	for( std::set< PowerCell* >::iterator powerup(_touchingPowerCells.begin()), end( _touchingPowerCells.end()); 
		powerup != end; 
		++powerup )
	{
		cpBodyActivate( (*powerup)->body() );
	}

	//
	//	Now, move the current powerup into position, and injest it
	//

	if ( _currentPowerCell )
	{
		//
		//	We want to move the powerup to the top-center of the platform, unrotating it, and then disposing it
		//
		
		cpSpace *space = level()->space();
		if ( cpSpaceContainsShape( space, _currentPowerCell->shape() ))
		{
			cpSpaceRemoveShape( space, _currentPowerCell->shape() );
		}
		
		//
		// Notify that we've taking hold
		//
		
		_currentPowerCell->onInjestedByPowerPlatform(_currentPowerCell);

		{			
			cpBody *powerupBody = _currentPowerCell->body();

			const Vec2r 
				CurrentPosition = v2r( cpBodyGetPos( powerupBody )), 
				InitialTargetPosition = position() + Vec2r( 0, _initializer.size.y * 0.5 + _currentPowerCell->initializer().size * 0.5 ),
				InjestedTargetPosition = position() + Vec2r( 0, _initializer.size.y * 0.5 - _currentPowerCell->initializer().size );

			const real 
				Rate = 0.05,
				CurrentAngle = cpBodyGetAngle( powerupBody ),
				TargetAngle = 0,
				NewAngle = lrpRadians<real>( Rate * 2, CurrentAngle, TargetAngle ),
				NewAngVel = (NewAngle - CurrentAngle) * time.deltaT;
				
			Vec2r targetPosition = InitialTargetPosition;
			if ( _injestionPhase == 0 )
			{
				if ( CurrentPosition.distanceSquared(InitialTargetPosition) < 0.05 )
				{
					_injestionPhase = 1;
				}
			}	
			
			if ( _injestionPhase == 1 )
			{
				targetPosition = InjestedTargetPosition;
				if ( CurrentPosition.distanceSquared( targetPosition ) < 0.05 )
				{
					// we're done -- destroy the powerup
					_currentPowerCell->setFinished(true);
					_currentPowerCell = NULL;
					_injestionPhase = 0;
					return;
				}
			}
							
			const Vec2r
				NewPosition = lrp(Rate, CurrentPosition, targetPosition ),
				NewVel = (NewPosition-CurrentPosition) * time.deltaT;		

			cpBodySetPos( powerupBody, cpv(NewPosition) );
			cpBodySetVel( powerupBody, cpv(NewVel) );
			cpBodySetAngle( powerupBody, NewAngle );
			cpBodySetAngVel( powerupBody, NewAngVel );			
		}		
	}
}

void PowerPlatform::setChargeLevel( int cl )
{
	if ( cl != chargeLevel() )
	{
		_chargeLevel = clamp<real>( cl, 0, _maxChargeLevel );
		chargeChanged(this);
		if ( charged() ) fullyCharged(this);
	}
}

void PowerPlatform::setMaxChargeLevel( int mcl )
{
	_maxChargeLevel = std::max<int>( mcl, real(0) );
	_chargeLevel = std::min( _chargeLevel, _maxChargeLevel );
}

void PowerPlatform::_contactPowerCell( const core::collision_info &info, bool &discard )
{
	//app::console() << description() << "::_contactPowerCell" << std::endl;

	if ( info.b->type() == GameObjectType::POWERCELL && _touchingPowerCells.insert( static_cast<PowerCell*>(info.b)).second )
	{
		info.b->aboutToDestroyPhysics.connect( this, &PowerPlatform::_removePowerCellFromTouchingSet );
	}
}

void PowerPlatform::_removePowerCellFromTouchingSet( GameObject *obj )
{
	_touchingPowerCells.erase( static_cast<PowerCell*>(obj) );
}

void PowerPlatform::_separatePowerCell( const core::collision_info &info )
{
	//app::console() << description() << "::_separatePowerCell" << std::endl;

	_touchingPowerCells.erase( static_cast<PowerCell*>(info.b) );
}

void PowerPlatform::_touchingPowerCell( const core::collision_info &info )
{
	//app::console() << description() << "::_touchingPowerCell" << std::endl;

	if ( !_currentPowerCell && (info.b->type() == GameObjectType::POWERCELL))
	{		
		PowerCell *powerCell = static_cast<PowerCell*>(info.b);
		if ( powerCell->charge() > 0 && !charged() )
		{
			app::console() << description() << " - discharging powercell " << powerCell->description() << " and beginning injestion" << std::endl;
		
			setChargeLevel( chargeLevel() + powerCell->discharge() );
			_currentPowerCell = powerCell;
			_injestionPhase = 0;
			_currentPowerCell->aboutToDestroyPhysics.connect(this, &PowerPlatform::_releaseCurrentPowerCell );
		}		
	}
}


void PowerPlatform::_releaseCurrentPowerCell( GameObject *obj )
{
	if ( obj == _currentPowerCell )
	{
		_currentPowerCell->aboutToDestroyPhysics.disconnect(this);
		_currentPowerCell = NULL;
		_injestionPhase = 0;
	}
}

#pragma mark - PowerPlatformRenderer

/*
		ci::gl::GlslProg _svgShader;
		core::SvgObject _svg, _lights, _halo;
		real _chargeLevel;
*/

PowerPlatformRenderer::PowerPlatformRenderer():
	_chargeLevel(0)
{
	setName( "PowerPlatformRenderer" );
}

PowerPlatformRenderer::~PowerPlatformRenderer()
{}

void PowerPlatformRenderer::addedToGameObject( core::GameObject * )
{}

void PowerPlatformRenderer::update( const core::time_state &time )
{
	PowerPlatform *platform = this->platform();
	_chargeLevel = lrp<real>( 0.1, _chargeLevel, static_cast<real>(platform->chargeLevel())/ static_cast<real>( platform->maxChargeLevel())); 
}

void PowerPlatformRenderer::_drawGame( const core::render_state &state )
{
	PowerPlatform *platform = this->platform();	
	
	if ( !_svgShader )
	{
		_svgShader = level()->resourceManager()->getShader( "SvgPassthroughShader" );
	}
		
	if ( !_svg )
	{
		_svg = level()->resourceManager()->getSvg( "PowerPlatform.svg" );
		Vec2r documentSize = _svg.documentSize();
		Vec2r scale = platform->initializer().size / documentSize;
		_svg.setScale( scale );
		_svg.setPosition( platform->position() );	
		
		while( true )
		{
			core::SvgObject light = _svg.find("Light" + str(_lightRings.size()));
			if ( !light ) break;

			light.setBlendMode( BlendMode( GL_SRC_ALPHA, GL_ONE ));
			light.setOpacity( 0 );
			_lightRings.push_back(light);
		}

		_halo = _svg.find("Halo");
		assert( _halo );
		_halo.setBlendMode( BlendMode( GL_SRC_ALPHA, GL_ONE ));
		_halo.setOpacity( 0 );
	}

	const real Phase = 1 + 0.0625 * std::cos( state.time * M_PI_2 );
		
	_halo.setOpacity( _chargeLevel * _chargeLevel * 0.025 * Phase );
	_halo.setScale( _chargeLevel * _chargeLevel * 4 * Phase );

	{
		//
		//	Now illuminate light rings to match charge level
		//

		const real 
			RangeIncrement = 1 / static_cast<real>(_lightRings.size()),
			LightMin = 0.125,
			LightRange = 0.875;

		real 
			rangeStart = 0,
			rangeEnd = RangeIncrement;

		for ( int i = 0, N = _lightRings.size(); i < N; i++ )
		{
			real amountLit = LightMin + LightRange * saturate((_chargeLevel-rangeStart) / RangeIncrement);
			rangeStart += RangeIncrement;
			rangeEnd += RangeIncrement;

			_lightRings[i].setOpacity( amountLit );
		}
	}
	
	gl::enableAlphaBlending();

	_svgShader.bind();
	_svg.draw( state );	
	_svgShader.unbind();
}

void PowerPlatformRenderer::_drawDebug( const core::render_state &state )
{
	PowerPlatform *platform = this->platform();
	_debugDrawChipmunkShape( state, platform->shape(), platform->debugColor() );
	_debugDrawChipmunkBody( state, platform->body() );
}


}
