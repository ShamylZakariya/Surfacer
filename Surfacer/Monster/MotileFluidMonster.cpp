//
//  MotileFluidMonster.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 1/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "MotileFluidMonster.h"
#include <chipmunk/chipmunk_unsafe.h>


using namespace ci;
using namespace core;
namespace game {

/*
	init _initializer;
	MotileFluid *_motileFluid;
	real _currentSpeed;
*/

MotileFluidMonster::MotileFluidMonster( int gameObjectType ):
	Monster( gameObjectType ),
	_motileFluid(new MotileFluid( gameObjectType ))
{
	setName( "MotileFluidMonster" );
	
	//
	//	Note, the shoggoth doesn't draw -- but the MotileFluidMonsterController draws debug overlays
	//
	
	setLayer( RenderLayer::MONSTER );
	setVisibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING );

	setController( new MotileFluidMonsterController() );
	
	//
	//	the fluid is a child, but we need to make it render on the monster layer
	//

	_motileFluid->setLayer( RenderLayer::MONSTER );
	addChild( _motileFluid );
}

MotileFluidMonster::~MotileFluidMonster()
{
}

void MotileFluidMonster::initialize( const init &i )
{
	Monster::initialize( i );
	_initializer = i;
	
	
	MotileFluid::init mfi = i.fluidInit;
	mfi.position = _initializer.position;
	mfi.userData = this;
	mfi.type = _fluidType();
	mfi.fluidHighlightLookupDistance = mfi.radius;
	mfi.introTime = mfi.extroTime = 0; // disable fluid's lifecycle management
	
	_motileFluid->initialize( mfi );	
}

// GameObject
void MotileFluidMonster::addedToLevel( Level *level )
{
	Monster::addedToLevel(level);
}

void MotileFluidMonster::update( const time_state &time )
{
	Monster::update( time );
	
	// sync fluid's lifecycle to Monster's
	_motileFluid->setLifecycle( this->lifecycle() );

	real direction = controller() ? controller()->direction().x : 0;
	
	_motileFluid->setSpeed( lrp<real>( 0.5, _motileFluid->speed(), (direction * _initializer.speed ) ) );

	//
	//	MotileFluidMonster doesn't draw, but we want to pose as if the fluid were 
	//	the shoggoth so the SHoggothController can emit probe rays, etc
	//

	setAabb( _motileFluid->aabb() );
}

// Monster

void MotileFluidMonster::injured( const HealthComponent::injury_info &info )
{
	real hue = 1 - (health()->health() / health()->maxHealth());	
	_motileFluid->setFluidToneMapHue( std::min<real>( hue, 1 - ALPHA_EPSILON ));

	Monster::injured( info );
}

void MotileFluidMonster::died( const HealthComponent::injury_info &info )
{
	//
	// after death, motile fluids collide only with terrain
	//

	foreach( MotileFluid::physics_particle &particle, _motileFluid->physicsParticles() )
	{
		cpShapeSetLayers( particle.shape, CollisionLayerMask::DEAD_MONSTER );
	}

	Monster::died( info );
}

Vec2r MotileFluidMonster::position() const 
{ 
	return v2r( cpBodyGetPos( _motileFluid->centralBody() ));
}

Vec2r MotileFluidMonster::up() const 
{ 
	return Vec2r(0,1); 
}

Vec2r MotileFluidMonster::eyePosition() const
{
	return position() + up() * cpBBHeight( aabb() ) * 0.5;
}

cpBody *MotileFluidMonster::body() const 
{
	return _motileFluid->centralBody();
}

#pragma mark - MotileFluidMonsterController

MotileFluidMonsterController::MotileFluidMonsterController()
{
	setName( "MotileFluidMonsterController" );
}

void MotileFluidMonsterController::update( const time_state &time )
{
	GroundBasedMonsterController::update( time );
}

void MotileFluidMonsterController::_getMotionProbeRays( cpVect &leftOrigin, cpVect &leftEnd, cpVect &rightOrigin, cpVect &rightEnd )
{
	MotileFluidMonster *motileFluidMonster = this->motileFluidMonster();

	const cpBB bounds = motileFluidMonster->aabb();
	const Vec2r center = motileFluidMonster->position();

	const real 
		extentX = 1.1 * cpBBWidth(bounds),
		height = cpBBHeight(bounds),
		extentY = 3 * height;

	leftOrigin.x = center.x - extentX;
	leftOrigin.y = center.y + height;
	leftEnd.x = center.x - extentX;
	leftEnd.y = center.y - extentY;

	rightOrigin.x = center.x + extentX;
	rightOrigin.y = center.y + height;
	rightEnd.x = center.x + extentX;
	rightEnd.y = center.y - extentY;
}



} // end namespace game
