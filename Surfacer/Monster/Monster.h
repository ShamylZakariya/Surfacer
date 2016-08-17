#pragma once

//
//  Monster.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/25/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "Core.h"

#include "Fluid.h"
#include "GameLevel.h"
#include "GameComponents.h"
#include "ParticleSystem.h"

#include <list>

namespace game {

#pragma mark - Prototypes

class Player;
class Weapon;
class MonsterController;

#pragma mark - Monster

class Monster : public Entity
{
	public:
	
		struct init : public Entity::init
		{
			// how long the animation 'introducing' the monster runs
			seconds_t introTime;

			// how long the animation killng or disposing of the monster runs
			seconds_t extroTime;

			// how many health points per second the monster inflicts on contact with the player
			real attackStrength;

			// position in world of the monster when it's created
			Vec2r position;
			
			// times the player must reverse direction per second to escape restraint
			real playerWiggleRateToEscapeRestraint;
			
			// the number of seconds the player must be wiggling at or above playerWiggleRateToEscapeRestraint to escape
			seconds_t playerWiggleTimeToEscape;
			
			init():
				introTime(1),
				extroTime(1),
				attackStrength(0),
				position(0,0),
				playerWiggleRateToEscapeRestraint(2),
				playerWiggleTimeToEscape(2)
			{}
						
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				Entity::init::initialize(v);
				JSON_READ(v, position);
				JSON_READ(v, introTime);
				JSON_READ(v, extroTime);
				JSON_READ(v, attackStrength);
				JSON_READ(v, playerWiggleRateToEscapeRestraint);
			}

		};

	public:
	
		Monster( int type );
		virtual ~Monster();
		
		JSON_INITIALIZABLE_INITIALIZE();
		
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }
		
		// Entity
		virtual void injured( const HealthComponent::injury_info &info );
		virtual void died( const HealthComponent::injury_info &info );


		// GameObject
		virtual void addedToLevel( core::Level *level );
		virtual void ready();
		virtual void update( const core::time_state &time );

		// Monster

		MonsterController *controller() const { return _controller; }
		void setController( MonsterController *newController );
		
		/**
			The monster's current up vector
		*/
		virtual Vec2r up() const = 0;
		
		/**
			The location of the monster's eye - for player visibility determination.
		*/
		virtual Vec2r eyePosition() const = 0;
		
		/**
			Called by player's weapon to verify the this monster can be shot on @a shape by @a weapon.
			Default implementation ignores shape and weapon and simply returns whether this monster
			is alive.
		*/
		virtual bool allowWeaponHit( Weapon *weapon, cpShape *shape ) const { return alive(); }
		
		/**
			get the monster's time of death.
			if the monster is still alive, this returns 0.
		*/
		seconds_t timeOfDeath() const { return _causeOfDeath.time; }
				
		/**
			Every monster gets a unique random number [0.5,1.5].
			Use this to scale animation cycles, animation extents.
		*/
		real fudge() const { return _fudge; }
		
		/**
			Every monster has a lifecycle. At birth, its zero, and ramps to 1 as monster grows through its
			initializer().introTime, and at death, ramps back down to zero across the monster's initializer().extroTime
		*/
		real lifecycle() const { return _lifecycle; }
		
		/**
			Called by MonsterController implementations to signal when a Monster should
			enable its range attack. Firing lasers, spitting acid, whatever.
		*/
		virtual void attack( bool doAttack ) { _attacking = doAttack; }
		
		/**
			@return true if this monster's range attack is enabled.
		*/
		bool attacking() const { return _attacking; }
		
		/**
			Called when this Monster touches a player.
			Default implementation injures player by amount proportional
			to initializer().attackStrength
			
			@param locationWorld Location in world coordinates of the injury contact site
			@param injuryScale A value from 0 to 1 denoting how to scale the particular injury
			
		*/
		virtual void playerContactAttack( const Vec2r &locationWorld, real injuryScale );
				
		virtual cpBody *body() const = 0;
		
		cpSpace *space() const { return _space; }

		virtual void restrainPlayer();
		virtual void releasePlayer();
		virtual bool restrainingPlayer() const;
							
	protected:
	
		virtual void _monsterPlayerContact( const core::collision_info &, bool &discard );
		virtual void _monsterPlayerPostSolve( const core::collision_info & );
		virtual void _monsterPlayerSeparate( const core::collision_info & );
		const Vec2rVec &_playerContactPositions() const { return _playerContactAttackPositions; }
				
	private:

		init _initializer;
		cpSpace *_space;
		MonsterController *_controller;
		HealthComponent::injury_info _causeOfDeath;
		real _fudge, _lifecycle;
		bool _attacking;		
		UniversalParticleSystemController::Emitter *_smokeEmitter, *_fireEmitter, *_injuryEmitter;
		Vec2rVec _playerContactAttackPositions;		

};

#pragma mark - MonsterController

class MonsterController : public core::Component
{
	public:
	
		enum action_state {
		
			ACTION_NONE,
			ACTION_WANDERING,
			ACTION_PURSUIT
		
		};

	public:
	
		MonsterController();
		virtual ~MonsterController(){}
		
		GameLevel *level() const { return (GameLevel*) owner()->level(); }
		Player *player() const { return level()->player(); }
		Monster *monster() const { return (Monster*) owner(); }

		virtual void draw( const core::render_state &state );
		virtual void update( const core::time_state &time );

		virtual void setAggression( real agg ) { _aggression = saturate(agg); }
		real aggression() const { return _aggression; }
		
		virtual void setFear( real fear ) { _fear = saturate(fear); }
		real fear() const { return _fear; }
		
		virtual void setDirection( const Vec2r &d ) { _direction = d; }
		Vec2r direction() const { return _direction; }
		
		virtual void setActionState( action_state a );
		action_state actionState() const { return _actionState; }
		
		void enable( bool e ) { _enabled = e; }
		bool enabled() const { return _enabled; }

		/**
			Check if the player was recently visible to the Monster.
			note: MonsterController checks for visibility of the player about once per second.
		*/
		virtual bool playerVisible() const { return _playerVisible; }
		
		/**
			Get the position of the player last time the monster saw the player
			note: MonsterController checks for visibility of the player about once per second.
		*/
		virtual Vec2r lastKnownPlayerPosition() const { return _lastKnownPlayerPosition; }
		
		/**
			Return the number of seconds the monster will remember where the player was 
			once the player is no longer visible. After this time, the monster will "forget"
			where the player is and will no longer pursue the last known position.
		*/

		virtual void setShortTermMemory( seconds_t ssm ) { _shortTermMemory = std::max<seconds_t>( ssm, 0 ); }
		seconds_t shortTermMemory() const { return _shortTermMemory; }	
		
		virtual void setVisualRange( real range ) { _visualRange = std::max<real>( range, 0 ); }
		real visualRange() const { return _visualRange; }
		
		real fudge() const { return _fudge; }

	protected:
	
		friend class Monster;
		
		virtual void _actionStateChanged( action_state newState ){}
		
		virtual void _updateAggression( const core::time_state & ){}

		/**
			Called by Monster when injured. Default implementation ramps up fear by injury grievousness
		*/
		virtual void _injured( const HealthComponent::injury_info &info );
	
		/**
			Compute the direction in world space to the last known player position.
		*/
		virtual Vec2r _directionToPlayer() const;

		/*
			Figure out which direction(s) the monster can travel in
		*/
		virtual void _updateFreedomOfMotion() = 0;
		
		/**
			Cast ray(s) to player position and determine if player is visible.
			Writes whether player is visible into @a visible. If visible, writes player position into @a playerPosition
		*/
		virtual bool _findPlayer( Player *player, Vec2r &playerPosition ) const;
						
	private:

		action_state _actionState;
		seconds_t _timeInActionState, _nextPlayerProbeTime, _nextMotionProbeTime, _lastPlayerVisibleTime, _shortTermMemory;
		bool _enabled, _playerVisible;
		Vec2r _direction, _lastKnownPlayerPosition;
		real _fear, _aggression, _fudge, _visualRange;

};


class GroundBasedMonsterController : public MonsterController
{
	public:
	
		GroundBasedMonsterController();
		virtual ~GroundBasedMonsterController(){}
		
		virtual void draw( const core::render_state &state );
		virtual void update( const core::time_state &time );

	protected:
			
		virtual void _getMotionProbeRays( cpVect &leftOrigin, cpVect &leftEnd, cpVect &rightOrigin, cpVect &rightEnd );
		virtual void _actionStateChanged( action_state newState );
		virtual void _updateFreedomOfMotion();
		virtual void _updateAggression( const core::time_state & );
		virtual void _updatePursuit( const core::time_state &time );
		virtual void _updateWandering( const core::time_state &time );
				

				
	private:

		bool _canMoveLeft, _canMoveRight;
		real _averageAbsIntendedVelocity, _averageAbsActualVelocity, _frustration;
		seconds_t _timeSpentInThisDirection, _nextReversalTime;

};

} // end namespace game