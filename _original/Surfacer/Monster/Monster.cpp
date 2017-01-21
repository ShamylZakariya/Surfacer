//
//  Monster.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/25/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "Monster.h"

#include "Fluid.h"
#include "Level.h"
#include "Player.h"
#include "GameConstants.h"
#include "GameLevel.h"

#include "InputDispatcher.h"
#include "FastTrig.h"

#include <cinder/ImageIO.h>

#define LOG_STATE_MACHINE_AI 0
#define LOG_WANDER_AI 0
#define LOG_PURSUIT_AI 0

using namespace ci;
using namespace core;
namespace game {

#pragma mark - Monster

/*
		init _initializer;
		cpSpace *_space;
		MonsterController *_controller;
		HealthComponent::injury_info _causeOfDeath;
		real _fudge, _lifecycle;
		bool _attacking;		
		UniversalParticleSystemController::Emitter *_smokeEmitter, *_fireEmitter, *_injuryEmitter;
		Vec2rVec _playerContactAttackPositions;		
*/

Monster::Monster( int type ):
	Entity( type ),
	_space(NULL),
	_controller( NULL ),
	_fudge( Rand::randFloat(0.5,1.5) ),
	_lifecycle(0),
	_attacking(false),
	
	_smokeEmitter(NULL),
	_fireEmitter(NULL),
	_injuryEmitter(NULL)
{
	setName( "Monster" );
	setLayer( RenderLayer::MONSTER );
	setVisibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING );
}

Monster::~Monster()
{
	app::console() << description() << "::dtor" << std::endl;
}

void Monster::initialize( const init &i )
{
	Entity::initialize(i);
	_initializer = i;
}

void Monster::injured( const HealthComponent::injury_info &info )
{
	GameLevel *level = static_cast<GameLevel*>(this->level());

	//
	//	Notify controller
	//

	if ( _controller ) _controller->_injured( info );

	//
	//	Now display an effect
	//

	const real 
		WobbleRadius = 1;
	
	switch( info.type )
	{
		case InjuryType::CUTTING_BEAM_WEAPON:
		case InjuryType::LAVA:
		{
			const Vec2 Wobble( Rand::randFloat( -WobbleRadius, WobbleRadius ), Rand::randFloat( -WobbleRadius, WobbleRadius ) );
			_fireEmitter->emit( 1, info.position + Wobble );
			break;
		}
		
		case InjuryType::CRUSH:
		{
			const int 
				Count = std::sqrt(info.grievousness) * 64;
			
			const real 
				WobbleRadius = cpBBRadius( aabb() ) * 0.1;
						
			Vec2 wobble;
			for ( int i = 0; i < Count; i++ )
			{
				wobble.x = Rand::randFloat( -WobbleRadius, WobbleRadius );
				wobble.y = Rand::randFloat( -WobbleRadius, WobbleRadius );
				_injuryEmitter->emit( 1, info.position + wobble, normalize( wobble ));
			}
						
			break;
		}
			
		default: break;
	}
}

void Monster::died( const HealthComponent::injury_info &info )
{
	app::console() << description() << "::died" << std::endl;
	_causeOfDeath = info;	
}

void Monster::addedToLevel( Level *level )
{
	_space = level->space();
	
	CollisionDispatcher::signal_set &signals = level->collisionDispatcher()->dispatch( CollisionType::MONSTER, CollisionType::PLAYER, this );
	signals.contact.connect(this, &Monster::_monsterPlayerContact );
	signals.postSolve.connect(this, &Monster::_monsterPlayerPostSolve );
	signals.separate.connect(this, &Monster::_monsterPlayerSeparate );
}

void Monster::ready()
{		
	GameLevel *gameLevel = static_cast<GameLevel*>(level());
	_smokeEmitter = gameLevel->smokeEffect()->emitter(32);
	_fireEmitter = gameLevel->fireEffect()->emitter(16);
	_injuryEmitter = gameLevel->monsterInjuryEffect()->emitter(16);
}

void Monster::update( const time_state &time )
{
	if ( alive() )
	{
		//
		//	If touching the player, and alive, injure!
		//
	
		if ( !_playerContactAttackPositions.empty() ) 
		{
			const real InjuryScale = real(1) / real(_playerContactAttackPositions.size());

			for ( Vec2rVec::const_iterator c(_playerContactAttackPositions.begin()), end( _playerContactAttackPositions.end());
				c != end; ++c )
			{
				playerContactAttack( *c, InjuryScale );
			}
			
			_playerContactAttackPositions.clear();
		}
	}
	else
	{
		//
		//	Run death effect
		//

		const real Radius = cpBBRadius(aabb());
		const std::size_t particleCount = Radius * lifecycle() * 2;

		if ( particleCount > 0 )
		{
			UniversalParticleSystemController::Emitter *emitter = _smokeEmitter;
			switch( _causeOfDeath.type )
			{
				case InjuryType::CRUSH:
				case InjuryType::STOMP:
					emitter = _injuryEmitter;
					break;
					
				default: break;					
			}
			
			const Vec2 Pos = this->position();
			for ( std::size_t i = 0; i < particleCount; i++ )
			{
				emitter->emit( 1, Pos + Rand::randVec2f() * Rand::randFloat() * Radius * 0.5 );
			}
		}
		else
		{
			//
			// wait for the injury particle system to be empty - when it is, we're done.
			//

			if ( time.time - timeOfDeath() > _initializer.extroTime )
			{
				setFinished(true);
			}
		}
	}

	//
	//	Update the lifecycle - it goes from 0 when born to 1 when in adulthood. and after death, it goes back to zero.
	//

	{
		const seconds_t age = this->age();

		if ( age < _initializer.introTime )
		{
			_lifecycle = real(age / _initializer.introTime);
		}
		else
		{
			_lifecycle = 1.0;
		}
				
		if ( !alive() )
		{
			_lifecycle = saturate<real>(1 - (( level()->time().time - timeOfDeath() ) / _initializer.extroTime));
		}
	}	
}

void Monster::setController( MonsterController *newController )
{
	if ( _controller )
	{
		removeComponent( _controller );
		delete _controller;
	}
	
	_controller = newController;

	if ( _controller ) 
	{
		addComponent( _controller );
	}	
}

void Monster::playerContactAttack( const Vec2 &positionWorld, real injuryScale )
{
	Player *player = static_cast<GameLevel*>(level())->player();
	if ( player )
	{
		player->health()->injure(injury( 
			InjuryType::MONSTER_CONTACT, 
			_initializer.attackStrength * injuryScale, 
			positionWorld, 
			NULL ));
	}
}

void Monster::restrainPlayer()
{
	Player *player = static_cast<GameLevel*>(level())->player();

	if ( !player ) return;
	
	if ( player->restrainer() == this )
	{
		return;
	}
	
	if ( player->restrainer() )
	{
		player->restrainer()->releasePlayer();
	}

	if ( player->alive() )
	{
		player->setRestrainer( this );
	}
}

void Monster::releasePlayer()
{
	GameLevel *level = static_cast<GameLevel*>(this->level());
	if ( level )
	{
		Player *player = level->player();
		if ( player && player->restrainer() == this )
		{
			player->setRestrainer( NULL );
		}
	}
}

bool Monster::restrainingPlayer() const
{
	Player *player = static_cast<GameLevel*>(level())->player();
	return (player && player->restrainer() == this );
}


void Monster::_monsterPlayerContact( const core::collision_info &, bool &discard )
{}

void Monster::_monsterPlayerPostSolve( const collision_info &info )
{
	for ( std::size_t i = 0, Count = info.contacts(); i < Count; i++ )
	{
		_playerContactAttackPositions.push_back( info.position(i));
	}
}

void Monster::_monsterPlayerSeparate( const core::collision_info & )
{}


#pragma mark - Raycasting

namespace {

	struct CanMoveRaycastQueryData {
		cpFloat dist;
		bool hitTerrain;
		bool hitFluid;
		
		CanMoveRaycastQueryData():
			dist( FLT_MAX ),
			hitTerrain(false),
			hitFluid(false)
		{}
		
	};

	void CanMoveRaycastQueryFilter(cpShape *shape, cpFloat t, cpVect n, void *data)
	{
		CanMoveRaycastQueryData *query = (CanMoveRaycastQueryData*) data; 
		cpCollisionType type = cpShapeGetCollisionType( shape );
		
		if ( t < query->dist )
		{
			query->dist = t;

			switch( type )
			{
				case CollisionType::TERRAIN:
					query->hitTerrain = true;
					query->hitFluid = false;
					break;
					
				case CollisionType::FLUID:
					query->hitFluid = true;
					query->hitTerrain = false;
					break;

				default: break;
			}
		}		
	}
	
	struct PlayerRaycastQueryData {
		cpFloat dist;
		bool hitPlayer;
	
		PlayerRaycastQueryData():
			dist( FLT_MAX ),
			hitPlayer(false)
		{}				
	};
	
	void CanSeePlayerRaycastQueryFilter( cpShape *shape, cpFloat t, cpVect n, void *data )
	{
		PlayerRaycastQueryData *query = (PlayerRaycastQueryData*) data;
		cpCollisionType type = cpShapeGetCollisionType( shape );
		
		if ( type == CollisionType::TERRAIN ||
		     type == CollisionType::FLUID ||
			 type == CollisionType::PLAYER )
		{
			if ( t < query->dist )
			{
				query->dist = t;
				switch( type )
				{
					case CollisionType::TERRAIN:
					case CollisionType::FLUID:
						query->hitPlayer = false;					
						break;

					case CollisionType::PLAYER:
						query->hitPlayer = true;
						break;
				}				
			}
		}
	}
}

#pragma mark - MonsterController

/*
		action_state _actionState;
		seconds_t _timeInActionState, _nextPlayerProbeTime, _nextMotionProbeTime, _lastPlayerVisibleTime, _shortTermMemory;
		bool _enabled, _playerVisible, _wandering;
		Vec2 _direction, _lastKnownPlayerPosition;
		real _fear, _aggression, _fudge, _visalRange;
*/

MonsterController::MonsterController():
	_actionState( ACTION_NONE ),
	_timeInActionState(0),
	_nextPlayerProbeTime(0),
	_nextMotionProbeTime(0),
	_lastPlayerVisibleTime(0),
	_shortTermMemory(5),
	_enabled(true),
	_playerVisible(false),
	_direction(0,0),
	_lastKnownPlayerPosition(-1000,-1000),
	_fear(0),
	_aggression(0),
	_fudge( Rand::randFloat(0.5,1.5) ),
	_visualRange(100)
{
	setName( "MonsterController" );
}

void MonsterController::draw( const render_state &state )
{
	switch ( state.mode ) 
	{
		case RenderMode::DEBUG:
		{
			if ( state.time - _lastPlayerVisibleTime < shortTermMemory() )
			{
				// draw last known player position
				Vec2 pp = lastKnownPlayerPosition();

				gl::color( playerVisible() ? Color(0,1,0) : Color(1,0,0) );
				gl::drawLine( monster()->eyePosition(), pp );
				gl::drawStrokedCircle( pp, 10 * state.viewport.reciprocalZoom(), 16 );
			}
			
			break;
		}

		default:
			break;
	}
}

void MonsterController::update( const time_state &time )
{
	Monster *monster = this->monster();
	Player *player = this->player();
	
	//
	//	Determine if we can move left or right
	//

	if ( time.time > _nextMotionProbeTime )
	{
		_updateFreedomOfMotion();
		_nextMotionProbeTime = time.time + 0.25 * _fudge;
	}
	
	if ( monster->alive() )
	{
		if ( player )
		{
			if ( time.time > _nextPlayerProbeTime )
			{
				
				bool visible = _findPlayer( player, _lastKnownPlayerPosition );
				
				#if LOG_STATE_MACHINE_AI
				if ( visible != _playerVisible )
				{
					app::console() << description() << "::update - " 
						<< (visible? "PLAYER BECAME VISIBLE" : "PLAYER BECAME HIDDEN")
						<< std::endl;
				}
				#endif
				
				
				_playerVisible = visible;
				_nextPlayerProbeTime = time.time + 0.1 * _fudge;
			}

			if ( playerVisible() ) 
			{
				_lastPlayerVisibleTime = time.time;
			}
				
			//
			//	If the player's NOT been visible for at least this monster's
			//	short term memory, forget about him an start wandering. Otherwise,
			//	we're in pursuit.
			//

			if ( time.time - _lastPlayerVisibleTime > shortTermMemory() )
			{
				#if LOG_STATE_MACHINE_AI
				if ( _actionState != ACTION_WANDERING )
				{
					app::console() << description() << "::update - lost sight of player -> ACTION_WANDERING" << std::endl;
				}
				#endif

				setActionState( ACTION_WANDERING );
			}
			else
			{
				#if LOG_STATE_MACHINE_AI
				if ( _actionState != ACTION_PURSUIT )
				{
					app::console() << description() << "::update - saw player! -> ACTION_PURSUIT" << std::endl;
				}
				#endif

				setActionState( ACTION_PURSUIT );
			}
			
			_updateAggression( time );
		}
		else
		{
			setActionState( ACTION_WANDERING );
		}
	}
	else
	{
		#if LOG_STATE_MACHINE_AI
		if ( _actionState != ACTION_NONE )
		{
			app::console() << description() << "::update - died -> ACTION_NONE" << std::endl;
		}
		#endif

		setActionState( ACTION_NONE );	
	}

	//
	//	Fear ramps down to zero over time
	//

	setFear( lrp<real>( 0.0125, fear(), 0 ));
	
	_timeInActionState += time.deltaT;
}

void MonsterController::setActionState( action_state a )
{
	if ( a != _actionState )
	{
		#if LOG_STATE_MACHINE_AI
		app::console() 
			<< monster()->description() << ".controller::setActionState" 
			<< " state: " << (a == ACTION_WANDERING ? "ACTION_WANDERING" : "ACTION_PURSUIT") 
			<< std::endl;
		#endif
	
		_actionState = a;
		_timeInActionState = 0;

		_actionStateChanged(_actionState);
	}
}	

void MonsterController::_injured( const HealthComponent::injury_info &info )
{
	setFear( lrp<real>( info.grievousness, fear(), 1 ));
}

Vec2 MonsterController::_directionToPlayer() const
{
	return normalize(lastKnownPlayerPosition() - monster()->position());
}
		
bool MonsterController::_findPlayer( Player *player, Vec2 &playerPosition ) const
{
	const cpLayers layers = 
		CollisionLayerMask::Layers::TERRAIN_BIT | 
		CollisionLayerMask::Layers::FLUID_BIT | 
		CollisionLayerMask::Layers::PLAYER_BIT;
	
	Monster *monster = this->monster();
	cpSpace *space = monster->level()->space();

	const cpBB 
		bounds = monster->aabb(),
		playerBounds = player->aabb();
		
	const real 
		playerHeight = playerBounds.t - playerBounds.b,
		playerQuarterHeight = playerHeight * 0.25;

	cpVect 
		eyePos = cpv( monster->eyePosition() ),
		playerCenter = cpBBCenter( playerBounds ),
		dirs[2] = { 
			cpvnormalize( cpvsub( cpv(playerCenter.x, playerCenter.y + playerQuarterHeight ), eyePos )),
			cpvnormalize( cpvsub( cpv(playerCenter.x, playerCenter.y - playerQuarterHeight ), eyePos ))
		},
		targets[2] = {
			cpvadd( eyePos, cpvmult(dirs[0], _visualRange ) ),
			cpvadd( eyePos, cpvmult(dirs[1], _visualRange ) )
		};

	PlayerRaycastQueryData query1;

	cpSpaceSegmentQuery( 
		space, 
		eyePos,
		targets[0],
		layers,
		CP_NO_GROUP, 
		CanSeePlayerRaycastQueryFilter,
		&query1 );

	bool visible = query1.hitPlayer;
	if ( visible )
	{
		playerPosition = player->position();	
	}
	else
	{
		// if not visible run another query
		
		PlayerRaycastQueryData query2;
		cpSpaceSegmentQuery( 
			space, 
			eyePos,
			targets[1],
			layers,
			CP_NO_GROUP, 
			CanSeePlayerRaycastQueryFilter,
			&query2 );

		visible = query2.hitPlayer;
		if ( visible )
		{
			playerPosition = player->position();	
		}
	}
	
	return visible;
}

#pragma mark - GroundBasedMonsterController


/*
		bool _canMoveLeft, _canMoveRight;
		real _averageAbsIntendedVelocity, _averageAbsActualVelocity, _frustration;
		seconds_t _timeSpentInThisDirection, _nextReversalTime;
*/

GroundBasedMonsterController::GroundBasedMonsterController():
	_canMoveLeft(true),
	_canMoveRight(true),
	_averageAbsIntendedVelocity(0),
	_averageAbsActualVelocity(0),
	_frustration(0),
	_timeSpentInThisDirection(0),
	_nextReversalTime(0)
{
	setName( "GroundBasedMonsterController" );
}

void GroundBasedMonsterController::draw( const render_state &state )
{
	MonsterController::draw(state);

	switch ( state.mode ) 
	{
		case RenderMode::DEBUG:
		{
			const ci::Color GoColor(0,1,0), NoGoColor(1,0,0);

			cpVect leftOrigin, leftEnd, rightOrigin, rightEnd;
			_getMotionProbeRays( leftOrigin, leftEnd, rightOrigin, rightEnd );
			
			gl::color( _canMoveLeft ? GoColor : NoGoColor );
			gl::drawLine( v2(leftOrigin), v2(leftEnd));

			gl::color( _canMoveRight ? GoColor : NoGoColor );
			gl::drawLine( v2(rightOrigin), v2(rightEnd));	
			
			break;
		}

		default:
			break;
	}
}

void GroundBasedMonsterController::update( const time_state &time )
{
	MonsterController::update(time);
	
	if ( enabled())
	{
		switch( actionState())
		{
			case ACTION_WANDERING:
				_updateWandering(time);
				break;
				
			case ACTION_PURSUIT:
				_updatePursuit(time);
				break;
				
			case ACTION_NONE:
				break;
		}
		
		_timeSpentInThisDirection += time.deltaT;
	}
}

void GroundBasedMonsterController::_getMotionProbeRays( cpVect &leftOrigin, cpVect &leftEnd, cpVect &rightOrigin, cpVect &rightEnd )
{
	cpBody *body = monster()->body();

	const cpBB bounds = monster()->aabb();
	const real 
		extentX = 1.25 * cpBBWidth( bounds ),
		extentY = 4 * cpBBHeight( bounds );

	const cpVect center = cpBodyWorld2Local( body, cpBBCenter( bounds ));

	leftOrigin = cpBodyLocal2World( body, cpv(center.x - extentX, center.y + extentY ));
	leftEnd = cpBodyLocal2World( body, cpv( center.x - extentX, center.y - extentY ));

	rightOrigin = cpBodyLocal2World( body, cpv(center.x + extentX, center.y + extentY ));
	rightEnd = cpBodyLocal2World( body, cpv( center.x + extentX, center.y - extentY ));	
}

void GroundBasedMonsterController::_actionStateChanged( action_state newState )
{
	_nextReversalTime = 0; // this marks need to schedule a new reversal time in the next call to updateWander
}

void GroundBasedMonsterController::_updateFreedomOfMotion()
{
	cpVect leftOrigin, leftEnd, rightOrigin, rightEnd;
	_getMotionProbeRays( leftOrigin, leftEnd, rightOrigin, rightEnd );

	const cpLayers layers = 
		CollisionLayerMask::Layers::TERRAIN_BIT | 
		CollisionLayerMask::Layers::FLUID_BIT;

	Monster *monster = this->monster();
	CanMoveRaycastQueryData left, right;

	cpSpaceSegmentQuery( 
		monster->level()->space(), 
		leftOrigin, 
		leftEnd, 
		layers, 
		CP_NO_GROUP, 
		CanMoveRaycastQueryFilter,
		&left );
			
	cpSpaceSegmentQuery( 
		monster->level()->space(), 
		rightOrigin, 
		rightEnd, 
		layers, 
		CP_NO_GROUP, 
		CanMoveRaycastQueryFilter,
		&right );

	_canMoveLeft = left.hitTerrain;
	_canMoveRight = right.hitTerrain;
	
	//
	//	Now, prevent monsters from walking out of level just in case such a path does exist
	//

	cpBB levelBounds = level()->bounds();
	if ( !cpBBContains( levelBounds, leftEnd )) _canMoveLeft = false;
	if ( !cpBBContains( levelBounds, rightEnd )) _canMoveRight = false;	
}

void GroundBasedMonsterController::_updateAggression( const time_state & )
{
	const cpBB 
		Bounds = monster()->aabb();

	const real
		PlayerX = lastKnownPlayerPosition().x,
		DistanceToPlayer = distance( PlayerX, (Bounds.l + Bounds.r) * 0.5 ),
		BoundsWidth = cpBBWidth( Bounds ),
		PersonalSpace = BoundsWidth * 5,
		Aggression = std::sqrt( 1 - saturate(DistanceToPlayer/PersonalSpace));
		
	setAggression( Aggression );
}


void GroundBasedMonsterController::_updatePursuit( const time_state &time )
{
	Vec2 
		dir = _directionToPlayer();
		
	#if LOG_PURSUIT_AI
	app::console() << description() << "::_updatePursuit step: " << time.step << " - directionToPlayer: " << dir << std::endl;
	#endif

	if ( dir.x < -Epsilon && !_canMoveLeft ) dir.x = 0;
	if ( dir.x > Epsilon && !_canMoveRight ) dir.x = 0;
	if ( fear() > 0.5 ) dir.x *= -1;

	if ( static_cast<int>(sign(dir.x)) != static_cast<int>(sign(direction().x)))
	{
		if ( _timeSpentInThisDirection > 2 * fudge() )
		{
			#if LOG_PURSUIT_AI
			app::console() << description() << "::_updatePursuit - setting direction: " << dir << std::endl;
			#endif

			// snap dir to left or right
			dir.x = sign( dir.x );
			dir.y = 0;		
		
			setDirection( dir );
			_timeSpentInThisDirection = 0;
		}
		else
		{
			#if LOG_PURSUIT_AI
			app::console() << description() << "::_updatePursuit - NEED to set direction, but waiting for timeout" << std::endl;
			#endif
		
		}
	}
}

void GroundBasedMonsterController::_updateWandering( const time_state &time )
{
	cpBody *body = monster()->body();

	real dir = direction().x;
	if ( std::abs( dir ) < Epsilon ) dir = Rand::randBool() ? +1 : -1;
	
	bool updateNextReversalTime = false;
	if ( _nextReversalTime <= 0 || time.time > _nextReversalTime )
	{
		#if LOG_WANDER_AI
		app::console() << description() << "::_updateWander - timed out, reversing." << std::endl;
		#endif

		dir *= -1;
		updateNextReversalTime = true;
	}
	else
	{
		//
		//	Determine if we're actually moving in the direction of our intended velocity.
		//	if not, we're probably up against a wall or a steep hill, or a cliff/lava flow
		//	and should reverse direction
		//

		_averageAbsIntendedVelocity = lrp<real>(0.1, _averageAbsIntendedVelocity, std::abs(dir) );
		_averageAbsActualVelocity = lrp<real>(0.1, _averageAbsActualVelocity, std::abs(cpBodyGetVel(body).x));
		
		const bool 
			TryingToMove = _averageAbsIntendedVelocity > Epsilon,
			ActuallyMoving = _averageAbsActualVelocity > 0.1;
		
		if ( TryingToMove && !ActuallyMoving )
		{
			_frustration = lrp<real>( 0.05, _frustration, 1 );
			if ( _frustration >= 1 - Epsilon )
			{
				#if LOG_WANDER_AI
				app::console() << description() << "::_updateWander - dir is nonzero " << dir << " but we're not moving, so reversing" << std::endl;
				#endif

				dir = -dir;
				updateNextReversalTime = true;
				_frustration = 0;
			}
			else {
				#if LOG_WANDER_AI
				app::console() << description() << "::_updateWander - not moving, frustration: " << _frustration << std::endl;
				#endif
			}
		}
		else
		{
			_frustration = lrp<real>( 0.05, _frustration, 0 );
		}
		
		if ( _canMoveLeft && !_canMoveRight )
		{
			#if LOG_WANDER_AI
			app::console() << description() << "::_updateWander - motion probe says we can only move left, going left." << std::endl;
			#endif

			dir = -1;
			updateNextReversalTime = true;
		}
		else if ( _canMoveRight && !_canMoveLeft )
		{
			#if LOG_WANDER_AI
			app::console() << description() << "::_updateWander - motion probe says we can only move right, going right." << std::endl;
			#endif

			dir = +1;
			updateNextReversalTime = true;
		}
	}

	if ( dir < -Epsilon && !_canMoveLeft ) dir = 0;
	if ( dir > Epsilon && !_canMoveRight ) dir = 0;	
	setDirection(Vec2(dir,0));

	//
	//	If we reversed direction for whatever reason, update the end time for this wander
	//

	if ( updateNextReversalTime )
	{
		_nextReversalTime = time.time + Rand::randFloat(10,20);
	}
}

} // end namespace game
