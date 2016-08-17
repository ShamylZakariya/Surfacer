//
//  GameComponents.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/25/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "GameComponents.h"
#include "Level.h"

using namespace ci;
using namespace core;
namespace game {

#pragma mark - HealthComponent

/*
		real _health, _maxHealth, _resistance, _crushingInjuryMultiplier;
		bool _stompable;
*/

HealthComponent::HealthComponent():
	_health( 0 ),
	_maxHealth( 0 ),
	_resistance( 0 ),
	_crushingInjuryMultiplier(1),
	_stompable(false)
{}

HealthComponent::~HealthComponent()
{}

void HealthComponent::initialize( const init &i )
{
	set( i.initialHealth, i.maxHealth, i.resistance );
	setCrushingInjuryMultiplier( i.crushingInjuryMultiplier );
	setStompable( i.stompable );
}

void HealthComponent::set( real initialHealth, real maxHealth, real resistance )
{
	_health = clamp<real>( initialHealth, 0, maxHealth );
	_maxHealth = std::max<real>( maxHealth, 0 );
	_resistance = saturate( resistance );
}

void HealthComponent::setMaxHealth( real newMaxHealth )
{
	_maxHealth = std::max<real>( newMaxHealth, 0 );
}

void HealthComponent::setResistance( real newResistance )
{
	_resistance = saturate( newResistance );
}

void HealthComponent::setCrushingInjuryMultiplier( real cm )
{
	_crushingInjuryMultiplier = std::max( cm, real(0));
}


real HealthComponent::injure( const injury &inj )
{
	//
	// only the dead shall know the end of war
	//

	if ( dead() ) return 0;

	const seconds_t TimeDuration = inj.timeDuration > 0 ? inj.timeDuration : level()->time().deltaT;
	real delta = (1 - resistance()) * inj.attackStrengthInHealthUnitsPerSecond * real(TimeDuration);
	
	// if it's a crushing injury, apply the crush injury multiplier
	if ( inj.typeOfInjury == InjuryType::CRUSH )
	{
		delta *= _crushingInjuryMultiplier;
	}	
	
	delta = std::min( delta, _health );
	
	if ( delta > Epsilon )
	{
		real previousHealth = _health;

		_health -= delta;
		_health = std::max<real>( _health, 0 );
		
//		app::console() << "HealthComponent::injure - health: " << _health << " delta: " << delta << " will cause death: " << str((_health + delta) <= Epsilon) << std::endl;
		
		//
		//	Post appropriate notifications
		//

		injury_info info;
		info.newHealth = _health;
		info.changeInHealth = -delta;
		info.position = inj.locationWorld;
		info.type = inj.typeOfInjury;
		info.shape = inj.shapeThatWasHit;
		info.causedDeath = _health + delta <= Epsilon;
		info.time = level()->time().time;
		
		real injuryPerSecond = delta / TimeDuration;
		info.grievousness = saturate( injuryPerSecond / previousHealth );
		
		healthChanged( _health, -delta );		
		healthWorsened( info );
		
		if ( std::abs(_health) < Epsilon )
		{
			died( info );
		}	
	}
	
	return -delta;
}

void HealthComponent::heal( real healthToAdd )
{
	healthToAdd = std::min( healthToAdd, maxHealth() - health());
	
	if ( healthToAdd > Epsilon )
	{
		_health += healthToAdd;
		_health = std::min( _health, _maxHealth );

		healthChanged( _health, healthToAdd );
		healthImproved( _health, healthToAdd );
		
		if ( std::abs( _health - _maxHealth ) < Epsilon )
		{
			healthRestored();
		}		
	}
}

void HealthComponent::kill( InjuryType::type killingInjuryType )
{
	if ( alive() )
	{
		real delta = _health;
		_health = 0;
		
		injury_info info;
		info.newHealth = 0;
		info.changeInHealth = -delta;
		info.position = v2r(cpBBCenter( owner()->aabb() ));
		info.type = killingInjuryType;
		info.shape = NULL;
		info.grievousness = 1;
		info.causedDeath = true;
		info.time = level()->time().time;

		healthChanged( _health, -delta );		
		healthWorsened( info );
		died( info );
	}
}

#pragma mark - Entity

/*
		init _initializer;
		HealthComponent *_health;
*/

Entity::Entity( int gameObjectType ):
	GameObject( gameObjectType ),
	_health( new HealthComponent() )
{
	addComponent( _health );
	_health->healthWorsened.connect( this, &Entity::injured );
	_health->died.connect( this, &Entity::died );
}

Entity::~Entity()
{}

void Entity::initialize( const init &i )
{
	GameObject::initialize(i);

	_initializer = i;
	_health->initialize( _initializer.health );
}



} // end namespace game
