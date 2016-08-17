#pragma once

//
//  GameBehaviors.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/2/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Core.h"
#include "GameComponents.h"

namespace game {

class Entity;
class HealthComponent;

/**
	Behavior which dispatches injury to Player or Monsters when they contact a Fluid with
	non-zero attack strength.
*/

class FluidInjuryDispatcher : public core::Behavior 
{
	public:

		FluidInjuryDispatcher();
		virtual ~FluidInjuryDispatcher();

		virtual void addedToLevel( core::Level *level );
		virtual void update( const core::time_state & );

	protected:

		void _Fluid_Monster_contact( const core::collision_info &info, bool &discard ); 
		void _Fluid_Entity_postSolve( const core::collision_info &info );
		void _forget( core::GameObject *obj );

	private:

		typedef std::vector< injury > injury_vec;
		std::map< Entity*, injury_vec > _injuriesByEntity;
};

/**
	Behavior which dispatches injury to Player or Monsters when crushed by heavy terrain.
*/
class CrushingInjuryDispatcher : public core::Behavior 
{
	public:
	
		struct init {
		
			real kineticEnergyToInjuryRatio;
			real impulseToInjuryRatio;
			real minInjuryThreshold;
			real killingStompImpulse;
			
			init():
				kineticEnergyToInjuryRatio(0.05),
				impulseToInjuryRatio(0.25),
				minInjuryThreshold(1),
				killingStompImpulse(100)
			{}
		
		};

	public:
		
		CrushingInjuryDispatcher();
		virtual ~CrushingInjuryDispatcher();
		
		virtual void initialize( const init &i ) { _initializer = i; }
		const init &initializer() const { return _initializer; }

		virtual void addedToLevel( core::Level *level );
		virtual void update( const core::time_state &time );

	protected:
	
		void _entityPostSolve( const core::collision_info &info );
		void _entityPossiblyBeingStompedByPlayer( const core::collision_info &info );
		void _forget( core::GameObject *obj );
		
	private:
	
		struct crush_contact {
			Vec2r position;
			cpShape *shape;
			
			crush_contact(){}
			crush_contact( const Vec2r &p, cpShape *s ):
				position(p),
				shape(s)
			{}
		};
		
		typedef std::vector< crush_contact > crush_contact_vec;
	
		struct crush_accumulator 
		{
			cpFloat impulseMagnitude, stompImpulseMagnitude, totalKE;
			cpVect impulseTotal, stompImpulseTotal;
			crush_contact_vec contacts;
			
			crush_accumulator():
				impulseMagnitude(0),
				stompImpulseMagnitude(0),
				totalKE(0),
				impulseTotal(cpvzero),
				stompImpulseTotal(cpvzero)
			{}
		};
		
		crush_accumulator &_crushAccumulatorForEntity( Entity* );
		
	private:
	
		init _initializer;
		std::map< Entity*, crush_accumulator > _crushAccumulators;		
		
};

/**
	Behavior which allows kinematic hard terrain to penetrate softer terrain.
	This is primarily for cool stuff like stalactites with hard tips to penetrate
	and stick in the ground when severed.
	
	"Hard" terrain is defined as terrain where the underlying voxel has a hardness approaching 254,
	where 255 is the magic static/uncuttable flag.	
*/
class TerrainImpactPenetrator : public core::Behavior
{
	public:
	
		struct init {
			real kineticEnergyToPenetrate;
			real penetrationDamping;
			
			init():
				kineticEnergyToPenetrate(5000),
				penetrationDamping(0.9)
			{}
			
		};

	public:
	
		TerrainImpactPenetrator();
		virtual ~TerrainImpactPenetrator();
		
		virtual void initialize( const init &i ){ _initializer = i; }
		const init &initializer() const { return _initializer; }

		virtual void addedToLevel( core::Level *level );
		virtual void update( const core::time_state &time );

	protected:
		
		void _contact( const core::collision_info &info, bool &discard );
				
	private:
	
		init _initializer;
				
};

} // end namespace game