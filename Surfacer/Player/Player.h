#pragma once

//
//  Player.h
//  Surfacer
//
//  Created by Zakariya Shamyl on 10/11/11.
//  Copyright 2011 TomorrowPlusX. All rights reserved.
//

#include "Core.h"
#include "CuttingBeam.h"
#include "GameComponents.h"
#include "MagnetoBeam.h"
#include "ParticleSystem.h"


#include <cinder/gl/Texture.h>

namespace game {

class Monster;
class PlayerInputComponent;
class PlayerSvgAnimator;
class PlayerPhysics;
class PlayerRenderer;

class Player : public Entity
{
	public:
	
		struct init : public Entity::init {
		
			Vec2r position;
			real height;
			real width;
			real density;
			real walkingSpeed;
			real runMultiplier;
			real batteryPower;
			CuttingBeam::init cuttingBeamInit;
			MagnetoBeam::init magnetoBeamInit;
			
			init():
				position(0,0),
				height(2.5),
				width(1.1),
				density(20),
				walkingSpeed(8),
				runMultiplier(2),
				batteryPower(10)
			{
				health.initialHealth = 10;
				health.maxHealth = 100;
				health.resistance = 0;
				health.crushingInjuryMultiplier = 1;
				health.stompable = false;
			}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				Entity::init::initialize(v);
				JSON_READ(v,position);
				JSON_READ(v,height);
				JSON_READ(v,width);
				JSON_READ(v,density);
				JSON_READ(v,walkingSpeed);
				JSON_READ(v,runMultiplier);
				JSON_READ(v,batteryPower);
				JSON_READ(v,cuttingBeamInit);
				JSON_READ(v,magnetoBeamInit);
			}

			
			operator bool() const { return height > 0 && width > 0; }
		
		};
		
		signals::signal< void( Player*, Weapon* ) > weaponSelected;

	public:
	
		Player();
		virtual ~Player();

		JSON_INITIALIZABLE_INITIALIZE();
		
		// Entity
		virtual void injured( const HealthComponent::injury_info &info );
		virtual void died( const HealthComponent::injury_info &info );

		// GameObject
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
		virtual void addedToLevel( core::Level *level );
		virtual void removedFromLevel( core::Level *removedFrom );
		virtual void ready();
		virtual void update( const core::time_state & );
		Vec2r position() const;

		// Player
		Vec2r groundSlope() const;
		bool touchingGround() const;
		cpBody *body() const;
		cpBody *footBody() const;
		cpGroup group() const;
		
		// returns -1->+1 based on direction player is walking
		real motion() const;
		bool crouching() const;
		bool jumping() const;
		
		PlayerPhysics *physics() const { return _physics; }		

		// Weapon Management
		
		Battery *battery() const { return _battery; }
		
		void selectWeaponType( WeaponType::type weaponType );
		WeaponType::type selectedWeaponType() const { return _weaponType; }

		Weapon *weapon() const { return _weapons[_weaponType]; }
		Weapon *weapon( WeaponType::type t ) { return _weapons[t]; }
		const std::vector< Weapon* > &weapons() const { return _weapons; };
		std::vector< Weapon* > &weapons() { return _weapons; };
		
		// Restaint - enemies can "restrain" the player
		void setRestrainer( Monster *monster );
		Monster * restrainer() const { return _restrainer; }
		bool restrained() const { return _restrainer != NULL; }
										
	private:
			
		void _shotSomethingWithCuttingBeam( const Weapon::shot_info &info );
		void _shotSomethingWithMagnetoBeam( const Weapon::shot_info &info );
		void _unrestrain( GameObject * );
				
	private:
			
		init _initializer;
		
		PlayerPhysics *_physics;
		PlayerInputComponent *_input;
		PlayerRenderer *_renderer;
		
		Battery *_battery;
		WeaponType::type _weaponType;
		std::vector< Weapon *>_weapons;

		UniversalParticleSystemController::Emitter *_smokeEmitter, *_fireEmitter, *_injuryEmitter;
		
		Monster *_restrainer;
		seconds_t _restrainedTime, _timeWiggling;
		std::size_t _wiggleCount;
		int _wiggleDirection;
};

class PlayerRenderer : public core::DrawComponent
{
	public:
	
		PlayerRenderer();
		virtual ~PlayerRenderer();
		
		Player *player() const { return (Player*) owner(); }
		virtual void update( const core::time_state &time );
		virtual void ready();

		Vec2r barrelEndpointWorld();
		
	protected:
	
		virtual void _drawGame( const core::render_state & );
		virtual void _drawDebug( const core::render_state & );
		
	private:
	
		ci::gl::GlslProg _shader;
		core::SvgObject _svg;
		PlayerSvgAnimator *_svgAnimator;
		
};

class PlayerInputComponent : public core::InputComponent
{
	private:
	
		int _keyRun, _keyLeft, _keyRight, _keyJump, _keyCrouch, _keyFire, _keyNextWeapon;

	public:
	
		PlayerInputComponent();
		virtual ~PlayerInputComponent();
		
		Player *player() const { return (Player*) owner(); }

		virtual void monitoredKeyDown( int keyCode );
		
		// actions
		bool isRunning() const;
		bool isGoingRight() const;
		bool isGoingLeft() const;
		bool isJumping() const;
		bool isCrouching() const;
				
};


} // end namespace game