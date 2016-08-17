#pragma once 

//
//  Weapon.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/24/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "Core.h"


namespace game {

namespace WeaponType {
	enum type {
	
		CUTTING_BEAM,
		MAGNETO_BEAM,
		COUNT
	
	};
	
	std::string displayName( int weaponType );
}

/**
	@class Battery
	The Player owns a "Battery" which is a way of representing power available to weapons. Each weapon
	consumed an amount of power per second of firing, which is drawn from the player's battery.
*/
class Battery
{
	public:
	
		Battery():
			_power(0)
		{}
		
		virtual ~Battery()
		{}
		
		virtual void setPower( real np ) { _power = std::max(np,real(0)); }
		real power() const { return _power; }		
		bool empty() const { return _power <= Epsilon; }
						
	private:
	
		real _power;
		
};


class Weapon : public core::GameObject 
{
	public:

		struct init : public core::GameObject::init
		{
			real maxRange;
			real powerPerSecond;
			
			init():
				maxRange(INFINITY),
				powerPerSecond(0.01)
			{}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				core::GameObject::init::initialize(v);
				JSON_READ(v, maxRange);
				JSON_READ(v, powerPerSecond);
			}

			
		};
		
		struct shot_info 
		{
			Weapon *weapon;
			GameObject *target;
			cpShape *shape;
			Vec2r position, normal;
		};

		/**
			Signal emitted when a GameObject is hit 
				- the Weapon instance
				- the GameObject which was hit
				- the position in world space of the hit
		*/
		signals::signal< void( const shot_info & ) > objectWasHit;
		
		signals::signal< void( Weapon* ) > attemptToFireEmptyWeapon;

	public:
	
		Weapon();
		virtual ~Weapon();	

		// Initialization
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
		
		// GameObject
		virtual void update( const core::time_state &time );
		
		// Weapon		
		void setBattery( Battery *b ) { _battery = b; }
		Battery *battery() const { return _battery; }

		// set the position of the weapon. This is where -- physically -- the weapon emits
		virtual void setPosition( const Vec2r &p ) { _position = p; }
		Vec2r position() const { return _position; }
		
		// set the position where the beam appears to emit. this is for rendering, not physics - it might be the same as position()
		virtual void setEmissionPosition( const Vec2r &p ) { _emissionPosition = p; }
		Vec2r emissionPosition() const { return _emissionPosition; }
		
		virtual void setDirection( const Vec2r &dir ) { _direction = normalize(dir); }
		Vec2r direction() const { return _direction; }

		virtual void setFiring( bool on );
		bool firing() const { return _firing; }
		virtual bool drawingPower() const { return firing(); }
		virtual real drawingPowerPerSecond() const { return initializer().powerPerSecond; }

		virtual void setRange( real r ) { _range = std::min(r, _initializer.maxRange ); }
		real range() const { return _range; }
		
		virtual WeaponType::type weaponType() const = 0;
		
		// called when user puts this weapon away.
		virtual void wasPutAway(){}
		
		// called when user equips this weapon
		virtual void wasEquipped(){};
		
		// returns true if the player is currently using this weapon
		bool active() const { return _active; }
		
	protected:
	
		friend class Player;

		void _wasPutAway() 
		{ 
			_active = false; 
			wasPutAway(); 
		}

		void _wasEquipped() 
		{ 
			_active = true; 
			wasEquipped(); 
		}
		
	private:
	
		bool _active;
		Battery *_battery; 
		init _initializer;
		bool _firing;
		Vec2r _position, _emissionPosition, _direction;
		real _range;

};

class BeamWeapon : public Weapon
{
	public:
	
		struct init : public Weapon::init
		{
			real width;				//	visual width of beam
			real attackStrength;	//	the strength of the attack (in health-per-second) when shooting enemies
			
			init():
				width(1),
				attackStrength(0)
			{}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				Weapon::init::initialize(v);
				JSON_READ(v,width);
				JSON_READ(v,attackStrength);
			}

			
		};
		
		struct raycast_target 
		{
			GameObject *object;
			cpShape *shape;
			cpBody *body;
			Vec2r position;
			Vec2r normal;
			
			raycast_target():
				object(NULL),
				shape(NULL),
				body(NULL)
			{}
						
			void reset()
			{
				object = NULL;
				shape = NULL;
				body = NULL;
			}
			
			operator bool() const { return object && shape && body; }
			
			bool operator == ( const raycast_target &other ) const 
			{ 
				return (object == other.object);
			}
			
			bool operator != ( const raycast_target &other ) const
			{
				return (object != other.object);
			}
		};

		struct raycast_query_filter {
			cpCollisionType collisionType, ignoreCollisionType;
			unsigned int collisionTypeMask;
			cpLayers layers;
			cpGroup group;
			
			raycast_query_filter():
				collisionType(0),
				ignoreCollisionType(0),
				collisionTypeMask(0),
				layers( CP_ALL_LAYERS ),
				group( CP_NO_GROUP )
			{}
			
		};
				
		/**
			Perform a raycasting query.
			@param level the Level to run the query in
			@param Weapon if non-null any enemies hit by raycast will be queried if they'll allow a hit by this weapon
			@param origin The origin of the raycast
			@param end The endpoint of the raycast
			@param filter filter parameters for racyast
			@param ignore If non-null, this cpShape will be explicitly ignored
		*/
		static raycast_target raycast( 
			core::Level *level, 
			Weapon *weapon,
			const Vec2r &origin, 
			const Vec2r &end, 
			const raycast_query_filter &filter = raycast_query_filter(),
			cpShape *ignore = NULL );
		
	public:
	
		BeamWeapon();
		virtual ~BeamWeapon();

		// Initialization
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
		
		// GameObject
		virtual void update( const core::time_state & );
		
		// BeamWeapon
		virtual Vec2r beamOrigin() const { return position(); }
		virtual Vec2r beamEnd() const { return _beamEnd; }
		virtual real beamLength() const { return position().distance( _beamEnd ); }

		/**
			Get the object, if any, the beam is aiming/firing at.
			This can only be safely called in your BeamWeapon subclass's update() method.
			You should NOT hold on to anything returned here.
		*/
		virtual const raycast_target &beamTarget() const { return _beamTarget; }
		
	protected:
		
		/**
			The _raycast() function filters collisions
		*/
		void _setRaycastQueryFilter( raycast_query_filter rqf ) { _raycastQF = rqf; }
		raycast_query_filter _raycastQueryFilter() const { return _raycastQF; }
		raycast_target _raycast( const Vec2r &origin, const Vec2r &end, cpShape *ignore = NULL ) const;
		
	private:
	
		init _initializer;
		Vec2r _beamEnd;
		raycast_target _beamTarget;
		raycast_query_filter _raycastQF;
	

};


} //end namespace game
