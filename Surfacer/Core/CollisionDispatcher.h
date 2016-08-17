#pragma once

//
//  CollisionDispatcher.h
//  Surfacer
//
//  Created by Zakariya Shamyl on 8/26/11.
//  Copyright 2011 TomorrowPlusX. All rights reserved.
//

#include "GameObject.h"
#include <cinder/Function.h>
#include <boost/bind.hpp>

namespace core {

/**
	@struct collision_info
	Passed to subscribers to CollisionDispatcher events. Provides
	a lot of information regarding the collision that occurred.
*/
struct collision_info
{
	GameObject *a, *b;
	cpShape *shapeA, *shapeB;
	cpBody *bodyA, *bodyB;
	bool bodyAStatic, bodyBStatic;
	cpCollisionType typeA, typeB;
	const cpArbiter *arbiter;

	collision_info( 
		GameObject *A, GameObject *B, 
		cpShape *ShapeA, cpShape *ShapeB, 
		cpCollisionType TypeA, cpCollisionType TypeB,
		const cpArbiter *Arbiter ):

		a(A),
		b(B),
		shapeA(ShapeA),
		shapeB(ShapeB),
		typeA(TypeA),
		typeB(TypeB),
		arbiter(Arbiter)
	{
		cpArbiterGetBodies( arbiter, &bodyA, &bodyB );
		bodyAStatic = bodyA ? cpBodyIsStatic(bodyA) : true;		
		bodyBStatic = bodyB ? cpBodyIsStatic(bodyB) : true;		
	}
	
	collision_info( cpArbiter *arb, struct cpSpace *space, void *data );

	// get the number of contact points in the collision
	int contacts() const { return cpArbiterGetCount(arbiter); }
	Vec2r position( int i = 0 ) const { return v2r(cpArbiterGetPoint(arbiter,i)); }
	Vec2r normal( int i = 0 ) const { return v2r(cpArbiterGetNormal(arbiter,i)); }
	real depth( int i = 0 ) const { return cpArbiterGetDepth(arbiter,i); }

	real elasticity() const { return cpArbiterGetElasticity(arbiter); }
	real friction() const { return cpArbiterGetFriction(arbiter); }
	Vec2r velocity() const { return v2r(cpArbiterGetSurfaceVelocity(arbiter)); }

	bool firstContact() const { return cpArbiterIsFirstContact(arbiter); }

	/**
		return the velocity of the collision in A's reference frame
	*/
	Vec2r relativeVelocityFrameA() const {
		return v2r( cpBodyGetVel( bodyB ) - cpBodyGetVel( bodyA ));
	}

	/**
		return the velocity of the collision in B's reference frame
	*/
	Vec2r relativeVelocityFrameB() const {
		return v2r( cpBodyGetVel( bodyA ) - cpBodyGetVel( bodyB ));
	}
	
	real relativeVelocity() const {
		return cpvlength( cpBodyGetVel( bodyB ) - cpBodyGetVel( bodyA ) );
	}
	
	/**
		return the sliding velocity of the two bodies at the contact point.
		For example, if a wheel is rolling across the ground without slipping
		the slide velocity is zero. If on the other hand a block is sliding down
		the hill and not rolling, the relative velicy will be high.
	*/
	Vec2r slideVelocity( int i = 0 ) const {
		cpVect p = cpArbiterGetPoint(arbiter,i);
		return v2r(cpvsub(cpBodyGetVelAtWorldPoint( bodyA, p), cpBodyGetVelAtWorldPoint( bodyB, p))); 
	}

};


class CollisionDispatcher
{
	public:
	
		struct collision_pair 
		{
			cpCollisionType typeA, typeB;
						
			inline collision_pair():
				typeA(0),
				typeB(0)
			{}
		
			inline collision_pair( cpCollisionType TypeA, cpCollisionType TypeB ):
				typeA(TypeA),
				typeB(TypeB)
			{}
			
			inline bool operator < ( const collision_pair &other ) const
			{
				if ( typeA != other.typeA ) return typeA < other.typeA;
				return typeB < other.typeB;
			}
		};


		struct hinted_collision_pair : public collision_pair
		{
			GameObject *aHint, *bHint;
						
			inline hinted_collision_pair():
				aHint(NULL),
				bHint(NULL)
			{}
		
			inline hinted_collision_pair( cpCollisionType TypeA, cpCollisionType TypeB, GameObject *AHint, GameObject *BHint ):
				collision_pair(TypeA,TypeB),
				aHint(AHint),
				bHint(BHint)
			{}
			
			inline bool operator < ( const hinted_collision_pair &other ) const
			{
				if ( typeA != other.typeA ) return typeA < other.typeA;
				if ( typeB != other.typeB ) return typeB < other.typeB;
				if ( aHint != other.aHint ) return aHint < other.aHint;
				return bHint < other.bHint;
			}
		};
		
		typedef signals::signal< void( const collision_info &, bool & ) > contact_signal, pre_solve_signal;
		typedef signals::signal< void( const collision_info & ) > post_solve_signal, separate_signal;
		
		struct signal_set {
			contact_signal contact;
			pre_solve_signal preSolve;
			post_solve_signal postSolve;
			separate_signal separate;
		};
		
		typedef std::map< hinted_collision_pair, signal_set > SignalsByHintedCollisionPair;

	public:
	
		CollisionDispatcher( cpSpace *space );
		virtual ~CollisionDispatcher();
		
		/**
			Get the dispatcher's signal_set for a collision between typeA and typeB.
			If you provide hinting, the signal_set will be filtered for matches against those hints.
			The signal_set provides four signals:
				- contact
				- preSolve
				- postSolve
				- separate
		*/
		signal_set &dispatch( cpCollisionType typeA, cpCollisionType typeB, GameObject *typeAHint = NULL, GameObject *typeBHint = NULL )
		{
			_bindChipmunkCallbacks( collision_pair(typeA, typeB) );			
			return _signals[ hinted_collision_pair( typeA, typeB, typeAHint, typeBHint ) ];
		}
		
		/**
			Get the collision signal which will be fired when two objects of collision type typeA 
			and typeB collide. You can then connect that signal to callback code to do whatever - 
			emit effects, play sound, etc.
			
			If typeAHint or typeBHint are not null, they act as filters. For example, suppose you have a
			monster which you want to receive a callback for collision with terrain. You want that
			monster only to respond to the callback, but any terrain instance will do. You'd call something like:
			
			Monster::setup()
			{
				collisionDispatcher()->contact( MONSTER_TYPE, TERRAIN_TYPE, this, NULL )
					.connect( this, &Monster::contactTerrain )
			}
			
			Monster::contactTerrain( const collision_info &info, bool &discard )
			{
				assert( this == info.a ); // this is guaranteed!
			}
			
			This insures that if you have several of these monsters, the callback is filtered such that
			monster #foo is only notified when its instance touches terrain. Others only get the callback
			when they contact terrain.
			
			NOTE: typeA MUST be < typeB; this is because chipmunk's hashing doesn't distinguish and so we must be diligent.
			
						
		*/
		contact_signal &contact( cpCollisionType typeA, cpCollisionType typeB, GameObject *typeAHint = NULL, GameObject *typeBHint = NULL )
		{
			return dispatch( typeA, typeB, typeAHint, typeBHint ).contact;
		}

		pre_solve_signal &preSolve( cpCollisionType typeA, cpCollisionType typeB, GameObject *typeAHint = NULL, GameObject *typeBHint = NULL )
		{
			return dispatch( typeA, typeB, typeAHint, typeBHint ).preSolve;
		}		

		post_solve_signal &postSolve( cpCollisionType typeA, cpCollisionType typeB, GameObject *typeAHint = NULL, GameObject *typeBHint = NULL )
		{
			return dispatch( typeA, typeB, typeAHint, typeBHint ).postSolve;
		}		

		separate_signal &separate( cpCollisionType typeA, cpCollisionType typeB, GameObject *typeAHint = NULL, GameObject *typeBHint = NULL )
		{
			return dispatch( typeA, typeB, typeAHint, typeBHint ).separate;
		}		
		
		/**
			Disconnect all signals connected to a particular receiver. This is a catch-all disconnect.
		*/
		template< typename T >
		void disconnect( T *obj )
		{
			for( SignalsByHintedCollisionPair::iterator 
				it( _signals.begin()), end( _signals.end());
				it != end; ++it )
			{
				it->second.contact.disconnect( obj );
				it->second.preSolve.disconnect( obj );
				it->second.postSolve.disconnect( obj );
				it->second.separate.disconnect( obj );
			}
		}
		
		/**
			Disconnect the signal emitted when typeA and typeB collide from receiver obj.
			The values of typeA, typeB, typeAHint and typeBHint must match the connection originally made.
		*/
		template< typename T >
		void disconnect( cpCollisionType typeA, cpCollisionType typeB, GameObject *typeAHint, GameObject *typeBHint, T *obj )
		{
			SignalsByHintedCollisionPair::iterator it = 
				_signals.find( hinted_collision_pair( typeA, typeB, typeAHint, typeBHint ));

			if ( it != _signals.end() )
			{
				it->second.contact.disconnect( obj );
				it->second.preSolve.disconnect( obj );
				it->second.postSolve.disconnect( obj );
				it->second.separate.disconnect( obj );
			}
		}
		
		cpSpace *space() const { return _space; }
			
	protected:
	
		virtual void _bindChipmunkCallbacks( const collision_pair &pair );
		virtual void _unbindChipmunkCallbacks( const collision_pair &pair );
	
	private:
	
		friend cpBool CollisionDispatcher_cpCollisionBegin(cpArbiter *arb, struct cpSpace *space, void *data);
		friend cpBool CollisionDispatcher_cpCollisionPreSolve(cpArbiter *arb, struct cpSpace *space, void *data);
		friend void CollisionDispatcher_cpCollisionPostSolve(cpArbiter *arb, struct cpSpace *space, void *data);
		friend void CollisionDispatcher_cpCollisionSeparate(cpArbiter *arb, struct cpSpace *space, void *data);

		inline void _getSignals( const collision_info &info,
			SignalsByHintedCollisionPair::iterator &catchAll,
			SignalsByHintedCollisionPair::iterator &hinted1,
			SignalsByHintedCollisionPair::iterator &hinted2,
			SignalsByHintedCollisionPair::iterator &hinted3 )
		{
			catchAll = _signals.find( CollisionDispatcher::hinted_collision_pair( info.typeA, info.typeB, NULL, NULL ));
			hinted1 = _signals.find( CollisionDispatcher::hinted_collision_pair( info.typeA, info.typeB, info.a, NULL ));
			hinted2 = _signals.find( CollisionDispatcher::hinted_collision_pair( info.typeA, info.typeB, NULL, info.b ));
			hinted3 = _signals.find( CollisionDispatcher::hinted_collision_pair( info.typeA, info.typeB, info.a, info.b ));
		}
	
		cpSpace *_space;
		
		SignalsByHintedCollisionPair _signals;
		std::set< collision_pair > _bindings;

};


}