#pragma once

//
//  MotileFluidMonster.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 1/25/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Monster.h"
#include "MotileFluid.h"

namespace game {

class MotileFluidMonster : public Monster
{
	public:
	
		struct init : public Monster::init {

			MotileFluid::init fluidInit;
			real speed;
			
			init():
				speed(3)
			{
				// HealthComponent::init
				health.initialHealth = health.maxHealth = 100;
				health.resistance = 0.5;

				// Monster::init
				introTime = 2;
				extroTime = 5;
				
				// MotileFluid - disable its lifecycle management
				fluidInit.extroTime = fluidInit.introTime = 0;
				
				fluidInit.collisionType = CollisionType::MONSTER;
				fluidInit.collisionLayers = CollisionLayerMask::MONSTER;
				fluidInit.friction = 2;
				fluidInit.elasticity = 0;
				fluidInit.fluidToneMapLumaPower = 0.5;
			}
			
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				Monster::init::initialize(v);
				JSON_READ(v, fluidInit);
				JSON_READ(v, speed);
			}

			
		};

	public:
	
		MotileFluidMonster( int gameObjectType );
		virtual ~MotileFluidMonster();
		
		JSON_INITIALIZABLE_INITIALIZE();
		
		// Initialization
		virtual void initialize( const init &i );
		const init &initializer() const { return _initializer; }

		// GameObject
		virtual void addedToLevel( core::Level *level );
		virtual void update( const core::time_state & );

		// Monster
		virtual void injured( const HealthComponent::injury_info &info );
		virtual void died( const HealthComponent::injury_info &info );
		virtual Vec2r position() const;
		virtual Vec2r up() const;
		virtual Vec2r eyePosition() const;
		virtual cpBody *body() const;

			// MotileFluidMonster
		MotileFluid *fluid() const { return _motileFluid; }
			
	protected:
	
		virtual MotileFluid::fluid_type _fluidType() const { return MotileFluid::PROTOPLASMIC_TYPE; }
			
	private:

		init _initializer;
		MotileFluid *_motileFluid;		
};

class MotileFluidMonsterController : public GroundBasedMonsterController
{
	public:
	
		MotileFluidMonsterController();
		virtual ~MotileFluidMonsterController(){}
		
		MotileFluidMonster *motileFluidMonster() const { return (MotileFluidMonster*) owner(); }
		virtual void update( const core::time_state &time );
		
	private:

		virtual void _getMotionProbeRays( cpVect &leftOrigin, cpVect &leftEnd, cpVect &rightOrigin, cpVect &rightEnd );

};


} // end namespace game
