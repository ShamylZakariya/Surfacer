//
//  CollisionDispatcher.cpp
//  Surfacer
//
//  Created by Zakariya Shamyl on 8/26/11.
//  Copyright 2011 TomorrowPlusX. All rights reserved.
//

#include "CollisionDispatcher.h"

namespace core {

/*
	GameObject *a, *b;
	cpShape *shapeA, *shapeB;
	cpBody *bodyA, *bodyB;
	cpCollisionType typeA, typeB;
	const cpArbiter *arbiter;
*/

collision_info::collision_info( cpArbiter *arb, struct cpSpace *space, void *data ):
	arbiter(arb)
{
	cpArbiterGetShapes( arb, &shapeA, &shapeB );
	typeA = cpShapeGetCollisionType( shapeA );
	typeB = cpShapeGetCollisionType( shapeB );

	cpArbiterGetBodies( arb, &bodyA, &bodyB );
	bodyAStatic = bodyA ? cpBodyIsStatic(bodyA) : true;		
	bodyBStatic = bodyB ? cpBodyIsStatic(bodyB) : true;		

	//
	//	Try to get the associated GameObjects
	//

	a = (GameObject*) cpShapeGetUserData( shapeA );
	if ( !a ) a = (GameObject*) cpBodyGetUserData( bodyA );
	
	b = (GameObject*) cpShapeGetUserData( shapeB );
	if ( !b ) b = (GameObject*) cpBodyGetUserData( bodyB );	
}


/*
		cpSpace *_space;
		std::map< collision_pair, signal_set > _signals;
		std::set< collision_pair > _bindings;
*/

CollisionDispatcher::CollisionDispatcher( cpSpace *space ):
	_space( space )
{}

CollisionDispatcher::~CollisionDispatcher()
{
	foreach( collision_pair pair, _bindings )
	{
		cpSpaceRemoveCollisionHandler( _space, pair.typeA, pair.typeB );
	}
}

#pragma mark -
#pragma mark Setup chipmunk space collision callbacks

// Friend functions can't go into a private namespace

cpBool CollisionDispatcher_cpCollisionBegin(cpArbiter *arb, struct cpSpace *space, void *data)
{
	collision_info info( arb, space,  data );
	CollisionDispatcher *dispatcher = (CollisionDispatcher*) data;

	CollisionDispatcher::SignalsByHintedCollisionPair::iterator catchAll, hinted1, hinted2, hinted3;
	dispatcher->_getSignals(info, catchAll, hinted1, hinted2, hinted3 );

	bool discard = false;
	if ( catchAll != dispatcher->_signals.end() )
	{
		catchAll->second.contact( info, discard );
	}

	if ( hinted1 != dispatcher->_signals.end() )
	{
		hinted1->second.contact( info, discard );
	}

	if ( hinted2 != dispatcher->_signals.end() )
	{
		hinted2->second.contact( info, discard );
	}

	if ( hinted3 != dispatcher->_signals.end() )
	{
		hinted3->second.contact( info, discard );
	}
	
	return !discard;
}

cpBool CollisionDispatcher_cpCollisionPreSolve(cpArbiter *arb, struct cpSpace *space, void *data)
{
	collision_info info( arb, space,  data );
	CollisionDispatcher *dispatcher = (CollisionDispatcher*) data;

	CollisionDispatcher::SignalsByHintedCollisionPair::iterator catchAll, hinted1, hinted2, hinted3;
	dispatcher->_getSignals(info, catchAll, hinted1, hinted2, hinted3 );

	bool discard = false;
	if ( catchAll != dispatcher->_signals.end() )
	{
		catchAll->second.preSolve( info, discard );
	}

	if ( hinted1 != dispatcher->_signals.end() )
	{
		hinted1->second.preSolve( info, discard );
	}

	if ( hinted2 != dispatcher->_signals.end() )
	{
		hinted2->second.preSolve( info, discard );
	}

	if ( hinted3 != dispatcher->_signals.end() )
	{
		hinted3->second.preSolve( info, discard );
	}
	
	return !discard;
}

void CollisionDispatcher_cpCollisionPostSolve(cpArbiter *arb, struct cpSpace *space, void *data)
{
	collision_info info( arb, space,  data );
	CollisionDispatcher *dispatcher = (CollisionDispatcher*) data;

	CollisionDispatcher::SignalsByHintedCollisionPair::iterator catchAll, hinted1, hinted2, hinted3;
	dispatcher->_getSignals(info, catchAll, hinted1, hinted2, hinted3 );

	if ( catchAll != dispatcher->_signals.end() )
	{
		catchAll->second.postSolve( info );
	}

	if ( hinted1 != dispatcher->_signals.end() )
	{
		hinted1->second.postSolve( info );
	}

	if ( hinted2 != dispatcher->_signals.end() )
	{
		hinted2->second.postSolve( info );
	}

	if ( hinted3 != dispatcher->_signals.end() )
	{
		hinted3->second.postSolve( info );
	}
}

void CollisionDispatcher_cpCollisionSeparate(cpArbiter *arb, struct cpSpace *space, void *data)
{
	collision_info info( arb, space,  data );
	CollisionDispatcher *dispatcher = (CollisionDispatcher*) data;

	CollisionDispatcher::SignalsByHintedCollisionPair::iterator catchAll, hinted1, hinted2, hinted3;
	dispatcher->_getSignals(info, catchAll, hinted1, hinted2, hinted3 );

	if ( catchAll != dispatcher->_signals.end() )
	{
		catchAll->second.separate( info );
	}

	if ( hinted1 != dispatcher->_signals.end() )
	{
		hinted1->second.separate( info );
	}

	if ( hinted2 != dispatcher->_signals.end() )
	{
		hinted2->second.separate( info );
	}

	if ( hinted3 != dispatcher->_signals.end() )
	{
		hinted3->second.separate( info );
	}
}


void CollisionDispatcher::_bindChipmunkCallbacks( const collision_pair &pair )
{	
	//	well, this is ugly but chipmunk doesn't distinguish between TypeA•TypeB and TypeB•TypeA.
	//	this means that if one user binds to PLAYER|DECORATION and another binds to DECORATION|PLAYER
	//	the latter will deleted the callbacks for PLAYER|DECORATION - i tried working around this, but
	//	it got too ugly, too quickly. better to fail noisily.

	assert( pair.typeA <= pair.typeB );

	if ( !_bindings.count( pair ))
	{
		_bindings.insert(pair);

		cpSpaceAddCollisionHandler( 
			_space,
			pair.typeA, 
			pair.typeB,
			&CollisionDispatcher_cpCollisionBegin,
			&CollisionDispatcher_cpCollisionPreSolve,
			&CollisionDispatcher_cpCollisionPostSolve,
			&CollisionDispatcher_cpCollisionSeparate,
			this
		);		
	}
}

void CollisionDispatcher::_unbindChipmunkCallbacks( const collision_pair &pair )
{
	cpSpaceRemoveCollisionHandler( _space, pair.typeA, pair.typeB );
	_bindings.erase( pair );
}

}