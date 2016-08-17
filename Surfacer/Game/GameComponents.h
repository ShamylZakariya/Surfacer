#pragma once

//
//  GameComponents.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/25/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "Components.h"
#include "GameObject.h"
#include "GameConstants.h"

namespace game {

struct injury 
{
	InjuryType::type typeOfInjury;
	real attackStrengthInHealthUnitsPerSecond;
	Vec2r locationWorld;
	cpShape *shapeThatWasHit;
	seconds_t timeDuration;
	
	injury( 
		InjuryType::type TypeOfInjury,
		real AttackStrengthInHealthUnitsPerSecond,
		const Vec2r &LocationWorld,
		cpShape *ShapeHit,
		seconds_t TimeDuration = 0
	):
		typeOfInjury(TypeOfInjury),
		attackStrengthInHealthUnitsPerSecond(AttackStrengthInHealthUnitsPerSecond),
		locationWorld(LocationWorld),
		shapeThatWasHit(ShapeHit),
		timeDuration(TimeDuration)
	{}
	
};

/**
	@class HealthComponent
	
	Health is made up of three factors:
		-health
		-max health
		-resistance
	
*/
class HealthComponent : public core::Component, public core::util::JsonInitializable 
{
	public:
	
		/**
			initialHealth - the Entity's initial health when creted
			maxHealth - the max health the Entity can have
			resistance - the resistance to injury.
			crushingInjuryMultiplier - if < 1, resistance to crushing injury goes to 0. > 1 multiplies crushing injury
			stompable - if true, the Entity can be killed by being stomped by the player
		*/
		struct init : public core::util::JsonInitializable {
			
			real initialHealth, maxHealth, resistance, crushingInjuryMultiplier;
			bool stompable;
			
			init():
				initialHealth(1),
				maxHealth(1),
				resistance(0),
				crushingInjuryMultiplier(1),
				stompable(false)
			{}	
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				JSON_READ(v, initialHealth);
				JSON_READ(v, maxHealth);
				JSON_READ(v, resistance);
				JSON_READ(v, crushingInjuryMultiplier);
				JSON_READ(v, stompable);
			}

		};
	
	
		struct injury_info {
		
			real newHealth;
			real changeInHealth;
			real grievousness; // the intensity of the injury - the likelyhood that one-second of such injury would kill
			Vec2r position; // in world coordinates
			cpShape *shape; // the shape that was contacted to cause injury
			InjuryType::type type; 
			bool causedDeath; // if true, the injury killed the Entity
			seconds_t time; // time injury occurred
			
			injury_info():
				newHealth(0),
				changeInHealth(0),
				grievousness(0),
				position(0,0),
				shape(NULL),
				type( InjuryType::NONE ),
				causedDeath(false),
				time(0)
			{}
					
		};
	
	
		// passes new health, and delta
		signals::signal< void(real,real) > healthChanged;
		
		// passes new health, and delta
		signals::signal< void(real,real) > healthImproved;

		// passes injury info struct decribing the injury
		signals::signal< void( const injury_info & ) > healthWorsened;
		
		// death, passes the injury info which led to death
		signals::signal< void( const injury_info & ) > died;

		// health completely restored
		signals::signal< void() > healthRestored;
	
	public:
	
		HealthComponent();
		virtual ~HealthComponent();
				
		JSON_INITIALIZABLE_INITIALIZE();
				
		// configures, and fires no signals
		void initialize( const init &i );
		
		// configures, and fires no signals
		void set( real initialHealth, real maxHealth, real resistance );
		
		real health() const { return _health; }
		bool dead() const { return !alive(); }
		bool alive() const { return _health > Epsilon; }
		bool fullHealth() const { return std::abs( _maxHealth - _health ) < Epsilon; }
		
		virtual void setMaxHealth( real newMaxHealth );
		real maxHealth() const { return _maxHealth; }
		
		virtual void setResistance( real newResistance );
		real resistance() const { return _resistance; }
		
		virtual void setCrushingInjuryMultiplier( real cm );
		real crushingInjuryMultiplier() const { return _crushingInjuryMultiplier; }
		
		virtual void setStompable( bool s ) { _stompable = s; }
		bool stompable() const { return _stompable; }
		
		/**
			Computes how much damage to incure based on an attack of strength
			@return the change in health (will be negative)
		*/
		
		virtual real injure( const injury &inj );
				
		/**
			Add to health
		*/
		virtual void heal( real healthToAdd );
				
		/**
			Cause health to fall to zero, sending appropriate signals.
		*/
		virtual void kill( InjuryType::type killingInjuryType = InjuryType::NONE );
										
	private:
	
		real _health, _maxHealth, _resistance, _crushingInjuryMultiplier;
		bool _stompable;

};


#pragma mark - Entity

/**
	Entity is the base class for things which are "alive" and have a HealthComponent and a lifecycle.
*/

class Entity : public core::GameObject
{
	public:
	
		struct init : public core::GameObject::init
		{
			HealthComponent::init health;
			
			init(){}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				core::GameObject::init::initialize(v);
				JSON_READ(v,health);
			}

		};

	public:
	
		Entity( int gameObjectType );
		virtual ~Entity();

		JSON_INITIALIZABLE_INITIALIZE();

		virtual void initialize( const init &i );
		const init & initializer() const { return _initializer; }

		HealthComponent *health() const { return _health; }
		virtual void injured( const HealthComponent::injury_info &info ){}
		virtual void died( const HealthComponent::injury_info &info ){}
		bool alive() const { return _health->alive(); }
		bool dead() const { return _health->dead(); }
		
	private:

		init _initializer;
		HealthComponent *_health;
	

};




} // end namespace game