//
//  GameBehaviors.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/2/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "GameBehaviors.h"

#include "Fluid.h"
#include "GameComponents.h"
#include "GameConstants.h"
#include "Level.h"
#include "Terrain.h"

using namespace ci;
using namespace core;
namespace game {

#pragma mark - FluidInjuryDispatcher

/*
		typedef std::vector< injury > injury_vec;
		std::map< Entity*, injury_vec > _injuriesByEntity;
*/

FluidInjuryDispatcher::FluidInjuryDispatcher()
{
	setName( "FluidInjuryDispatcher" );
}

FluidInjuryDispatcher::~FluidInjuryDispatcher()
{}

void FluidInjuryDispatcher::addedToLevel( Level *level )
{
	CollisionDispatcher *dispatcher = level->collisionDispatcher();

	dispatcher->contact( CollisionType::FLUID, CollisionType::MONSTER )
		.connect( this, &FluidInjuryDispatcher::_Fluid_Monster_contact );

	dispatcher->postSolve( CollisionType::FLUID, CollisionType::MONSTER )
		.connect( this, &FluidInjuryDispatcher::_Fluid_Entity_postSolve );

	dispatcher->postSolve( CollisionType::FLUID, CollisionType::PLAYER )
		.connect( this, &FluidInjuryDispatcher::_Fluid_Entity_postSolve );
}

void FluidInjuryDispatcher::update( const core::time_state & )
{
	for( std::map< Entity*, injury_vec >::iterator it(_injuriesByEntity.begin()),end(_injuriesByEntity.end()); it != end; ++it )
	{
		injury_vec &injuries = it->second;
		if ( !injuries.empty())
		{
			HealthComponent *health = it->first->health();
			for ( injury_vec::const_iterator injury(injuries.begin()), end( injuries.end()); injury != end; ++injury )
			{
				health->injure( *injury );
			}

			injuries.clear();
		}
	}
}

void FluidInjuryDispatcher::_Fluid_Monster_contact( const collision_info &info, bool &discard )
{
	if ( ((Fluid*) info.a)->injuryType() == InjuryType::ACID )
	{
		discard = true;
	}
}

void FluidInjuryDispatcher::_Fluid_Entity_postSolve( const collision_info &info )
{
	Fluid *fluid = (Fluid*) info.a;
	Entity *entity = (Entity*)info.b;
	
	
	if ( fluid->attackStrength() > 0 )
	{
		if ( _injuriesByEntity.find(entity) == _injuriesByEntity.end())
		{
			entity->aboutToBeDestroyed.connect(this, &FluidInjuryDispatcher::_forget );
		}
	
		injury_vec &injuries = _injuriesByEntity[entity];
		for ( int i = 0, N = info.contacts(); i < N; i++ )
		{
			injuries.push_back( injury(
				fluid->injuryType(), 
				fluid->attackStrength(), 
				info.position(i), 
				info.shapeA
			));

//			entity->health()->injure(injury(
//				fluid->injuryType(), 
//				fluid->attackStrength(), 
//				info.position(i), 
//				info.shapeA
//			));
		}
	}
}

void FluidInjuryDispatcher::_forget( GameObject *obj )
{
	_injuriesByEntity.erase( static_cast<Entity*>(obj));
}

#pragma mark - CrushingInjuryDispatcher

CrushingInjuryDispatcher::CrushingInjuryDispatcher()
{
	setName( "CrushingInjuryDispatcher" );
}

CrushingInjuryDispatcher::~CrushingInjuryDispatcher()
{}

void CrushingInjuryDispatcher::addedToLevel( core::Level *level )
{
	/*
		Note,
		I used to bind monster/terrain contact, and would discard when the monster was dead. The reasoning being that
		once the monster died falling rocks would pass right through. Cool, right? Well, this meant dead monsters
		would fall through the solid floor because I didn't think it through.
	*/

	CollisionDispatcher::signal_set 
		&playerToTerrain		= level->collisionDispatcher()->dispatch(CollisionType::PLAYER,		CollisionType::TERRAIN),
		&playerToDecoration		= level->collisionDispatcher()->dispatch(CollisionType::PLAYER,		CollisionType::DECORATION),
		&monsterToTerrain		= level->collisionDispatcher()->dispatch(CollisionType::MONSTER,	CollisionType::TERRAIN),
		&monsterToDecoration	= level->collisionDispatcher()->dispatch(CollisionType::MONSTER,	CollisionType::DECORATION),
		&monsterToPlayer		= level->collisionDispatcher()->dispatch(CollisionType::MONSTER,	CollisionType::PLAYER);

	monsterToTerrain.postSolve.connect( this, &CrushingInjuryDispatcher::_entityPostSolve );
	monsterToDecoration.postSolve.connect( this, &CrushingInjuryDispatcher::_entityPostSolve );
	
	playerToTerrain.postSolve.connect( this, &CrushingInjuryDispatcher::_entityPostSolve ); 
	playerToDecoration.postSolve.connect( this, &CrushingInjuryDispatcher::_entityPostSolve );
	
	monsterToPlayer.postSolve.connect( this, &CrushingInjuryDispatcher::_entityPossiblyBeingStompedByPlayer );
}

void CrushingInjuryDispatcher::update( const core::time_state &time )
{
	/*
		http://chipmunk-physics.net/forum/viewtopic.php?f=1&t=1585&hilit=crushing
		
		This is derived from the above. The priciple idea is that we sum the impulse vector and magnitude.
		If the length of the vector sum is less than the magnitude sum, we have forces squeezing the entity,
		and that translates to some kind of crushing action. 
		
		For monster's that's enough.
		
		The player however needs more nuance. Say the player has run into a cravasse too narrow requiring crouching.
		The above system will treat that as crushing and injure the player, which is too punitive. So, for the player
		we're tallying kinetic energy of impactors, and scaling that by the crushing force. This prevents injury by
		squeezing between static geometry, but still allows falling rocks to injure. 
	*/
	
	for( std::map< Entity*, crush_accumulator >::iterator ecac(_crushAccumulators.begin()),end(_crushAccumulators.end()); ecac != end; ++ecac )
	{
		Entity *entity = ecac->first;
		HealthComponent *health = entity->health();
		crush_accumulator &acc(ecac->second);
		
		if ( acc.impulseMagnitude > Epsilon )
		{
			const real 
				ImpulseTotal = cpvlength(acc.impulseTotal),
				CrushFactor = real(1) - ImpulseTotal/acc.impulseMagnitude;

			if ( entity->type() == GameObjectType::PLAYER )
			{		
				//
				//	If the entity is the player, use more expensive injury computation which takes into
				//	account kinetic energy of the impactors
				//

				if ( !acc.contacts.empty() )
				{
					const real 
						InjuryExtent = CrushFactor * _initializer.kineticEnergyToInjuryRatio * acc.totalKE * level()->time().deltaT,
						InjuryExtentPerContact = InjuryExtent / acc.contacts.size();

					if ( InjuryExtent > _initializer.minInjuryThreshold )
					{
						for ( crush_contact_vec::const_iterator contact( acc.contacts.begin()), end(acc.contacts.end()); contact != end; ++contact )
						{
							health->injure( injury(
								InjuryType::CRUSH, 
								InjuryExtentPerContact, 
								contact->position, 
								contact->shape ));
						}			
					}
				}
			}
			else
			{
				//
				//	Monsters get cheapo computation that computes how squished they are
				//
				
				const real 
					InjuryExtent = CrushFactor * _initializer.impulseToInjuryRatio * (acc.impulseMagnitude-ImpulseTotal) * level()->time().deltaT,
					InjuryExtentPerContact = InjuryExtent / acc.contacts.size();
			
				if ( InjuryExtent > _initializer.minInjuryThreshold )
				{
					for ( crush_contact_vec::const_iterator contact( acc.contacts.begin()), end(acc.contacts.end()); contact != end; ++contact )
					{
						health->injure( injury(
							InjuryType::CRUSH, 
							InjuryExtentPerContact, 
							contact->position, 
							contact->shape ));
					}			
				}
			}
		}
		
//		if ( acc.stompImpulseMagnitude > Epsilon )
//		{
//			app::console() << entity->description() << " stomp impulse mag: " << acc.stompImpulseMagnitude << std::endl;
//		}
		
		if ( (acc.stompImpulseMagnitude > _initializer.killingStompImpulse) && health->stompable() )
		{
			health->kill( InjuryType::STOMP );
		}
				
		acc.impulseMagnitude = 0;
		acc.impulseTotal = cpvzero;
		acc.stompImpulseMagnitude = 0;
		acc.stompImpulseTotal = cpvzero;
		acc.totalKE = 0;
		acc.contacts.clear();
	}
}

void CrushingInjuryDispatcher::_entityPostSolve( const core::collision_info &info )
{
	Entity *entity = static_cast< Entity* >(info.a);
	crush_accumulator &accumulator(_crushAccumulatorForEntity(entity));
	
	//
	//	Sum the impulse vector and magnitudes
	//

	const cpVect Impulse = cpArbiterTotalImpulseWithFriction( info.arbiter );
	accumulator.impulseTotal = cpvadd( accumulator.impulseTotal, Impulse );	
	accumulator.impulseMagnitude += cpvlength( Impulse );
		
	//
	//	We do fine-grained crush compuations for the player - monsters get cheaper, simpler approach
	//	directly from the chipmunk forums
	//

	if ( entity->type() == GameObjectType::PLAYER )
	{
		if ( !info.bodyBStatic )
		{
			
			//
			//	Compute velocity of contact point, if > 0, dot that with the direction to player
			//	to determine whether it represents a potential injury. If so, compute KE of this 
			//	contact and add it
			//
			
			const cpVect 
				ContactPos = cpv(info.position()),
				ContactVelB = cpBodyGetVelAtWorldPoint( info.bodyB, ContactPos );
				
			const float 
				ContactLV2B = cpvlengthsq( ContactVelB );

			//
			//	If the contact point is moving, it might be transfering KE to receiver
			//

			if ( ContactLV2B > Epsilon )
			{
				const real
					ContactLVB = std::sqrt( ContactLV2B );

				const cpVect
					DirToA = cpvnormalize( cpvsub( cpBodyGetPos( info.bodyA ), ContactPos )),
					ContactDir = cpvmult( ContactVelB, 1/ContactLVB );

				const real 
					CollisionFactor = cpvdot( DirToA, ContactDir );

				//
				//	The contact point is moving TOWARDS the receiver
				//

				if ( CollisionFactor > 0 )
				{
					const cpVect	
						ContactVelA = cpBodyGetVelAtWorldPoint( info.bodyA, ContactPos );
					
					const real
						ContactLVA = cpvlength(ContactVelA),
						LVSum = ContactLVA+ContactLVB;
						
					if ( LVSum > 0 )
					{
						//
						//	Finally! The transfer is the amount of impact force to apply, compute KE accordingly
						//
						const real Transfer = real(1) - cpvlength(cpvadd(ContactVelA,ContactVelB))/LVSum;
						
						if ( Transfer > 0 )
						{
							const real ContactKE = real(0.5) * cpBodyGetMass(info.bodyB) * LVSum*LVSum;
						
							accumulator.totalKE += Transfer * ContactKE;
							accumulator.contacts.push_back( crush_contact(v2r(ContactPos), info.shapeA ));
						}
					}
				}
			}
		}
	}
	else
	{
		//
		//	Just copy over contact information for monsters
		//

		for ( int i = 0, N = info.contacts(); i < N; i++ )
		{
			accumulator.contacts.push_back( crush_contact( info.position(i), info.shapeA ));
		}
	}
}

void CrushingInjuryDispatcher::_entityPossiblyBeingStompedByPlayer( const core::collision_info &info )
{
	assert( GameObjectType::isMonster(info.a->type()));	
	assert( info.b->type() == GameObjectType::PLAYER );

	Entity *entity = static_cast< Entity* >(info.a);
	crush_accumulator &accumulator(_crushAccumulatorForEntity(entity));
	
	//
	//	Add impulse imparted by player landing on the monster - we care only about y component, and it must be negative
	//

	const cpVect 
		Impulse = cpArbiterTotalImpulseWithFriction( info.arbiter ),
		StompImpulse = cpv(0,std::min<cpFloat>(Impulse.y,0));
		
	accumulator.stompImpulseTotal = cpvadd( accumulator.stompImpulseTotal, StompImpulse );
	accumulator.stompImpulseMagnitude += cpvlength( StompImpulse );
}

void CrushingInjuryDispatcher::_forget( GameObject *obj )
{
	_crushAccumulators.erase( static_cast<Entity*>(obj));
}

CrushingInjuryDispatcher::crush_accumulator &CrushingInjuryDispatcher::_crushAccumulatorForEntity( Entity *entity )
{
	std::map< Entity*, crush_accumulator >::iterator ecacPos = _crushAccumulators.find(entity);
	if ( ecacPos == _crushAccumulators.end() )
	{
		_crushAccumulators[entity] = crush_accumulator();
		ecacPos = _crushAccumulators.find(entity);
		entity->aboutToBeDestroyed.connect( this, &CrushingInjuryDispatcher::_forget );
	}
	
	return ecacPos->second;
}


#pragma mark - TerrainImpactPenetrator

TerrainImpactPenetrator::TerrainImpactPenetrator()
{}

TerrainImpactPenetrator::~TerrainImpactPenetrator()
{}

void TerrainImpactPenetrator::addedToLevel( Level *level )
{
	level->collisionDispatcher()->contact( CollisionType::TERRAIN, CollisionType::TERRAIN )
		.connect( this, &TerrainImpactPenetrator::_contact );
}

void TerrainImpactPenetrator::update( const time_state &time )
{}

void TerrainImpactPenetrator::_contact( const core::collision_info &info, bool &discard )
{
	bool 
		shouldDiscard = false;
	
	const real
		LV_A = cpvlength( cpBodyGetVel( info.bodyA )),
		LV_B = cpvlength( cpBodyGetVel( info.bodyB )),
		KE_A = info.bodyAStatic ? 0 : (cpBodyGetMass( info.bodyA ) * LV_A * LV_A * 0.5),
		KE_B = info.bodyBStatic ? 0 : (cpBodyGetMass( info.bodyB ) * LV_B * LV_B * 0.5),
		Strength = 1;
		
	if ( KE_A > _initializer.kineticEnergyToPenetrate ||
	     KE_B > _initializer.kineticEnergyToPenetrate )
	{
		terrain::Island 
			*islandA = (terrain::Island*) info.a,
			*islandB = (terrain::Island*) info.b,
			*cuttingIsland = !info.bodyAStatic ? islandA : islandB,
			*otherIsland = cuttingIsland == islandA ? islandB : islandA;
								
		terrain::Terrain 
			*theTerrain = islandA->group()->terrain();

		unsigned int 
			result = theTerrain->cutTerrain( cuttingIsland, Strength, TerrainCutType::FRACTURE, otherIsland );

		if ( result & terrain::Terrain::CUT_AFFECTED_VOXELS )
		{
			shouldDiscard = true;
		}

		if ( !info.bodyAStatic )
		{
			cpBodySetVel( info.bodyA, cpBodyGetVel( info.bodyA ) * _initializer.penetrationDamping );
			cpBodySetAngVel( info.bodyA, cpBodyGetAngVel( info.bodyA ) * _initializer.penetrationDamping);
		}

		if ( !info.bodyBStatic )
		{
			cpBodySetVel( info.bodyB, cpBodyGetVel( info.bodyB ) * _initializer.penetrationDamping );
			cpBodySetAngVel( info.bodyB, cpBodyGetAngVel( info.bodyB ) * _initializer.penetrationDamping);
		}		
	}
	
	if ( shouldDiscard ) 
	{
		discard = true;
	}
		
}

}