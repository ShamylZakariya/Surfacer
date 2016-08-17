//
//  GameLevel.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/13/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "Level.h"
#include "Scenario.h"

#include <cinder/app/AppBasic.h>
#include <cinder/gl/gl.h>


using namespace ci;
namespace core {

/*
		bool					_ready, _paused;

		init					_initializer;
	
		Scenario				*_scenario;
		ResourceManager			*_resourceManager;
		cpSpace					*_space;
		CollisionDispatcher		*_collisionDispatcher;
		GameObjectSet           _objects;
		GameObjectsByInstanceId	_objectsByInstanceId;
		GameObjectsById         _objectsById;
		BehaviorSet             _behaviors;
		
		time_state				_time;
		DrawDispatcher          _drawDispatcher;
		seconds_t				_lastStepInterval;
*/


Level::Level():
	_ready(false),
	_paused(false),
	_scenario( NULL ),
	_resourceManager( NULL ),
	_space(NULL),
	_collisionDispatcher(NULL),
	_time(0,0,0),
	_lastStepInterval(0)
{}

Level::~Level()
{
	//
	//	Kill the collision dispatcher first - if we don't it can be notified as shapes are removed from the space
	//

	delete _collisionDispatcher;
	_collisionDispatcher = NULL;
	
	//
	//	Delete all objects; we can't just delete everything in _objects
	//	because some are owned by the root objects (who are responsible for
	//	deleting children. So gather roots, kill them, and assert emptiness.
	//	Note: some complex objects may orphan children on removal/destruction instead
	//	of deleting them - this adding new root objects. Hence the looping.
	//

	GameObjectSet roots;
	rootObjects( roots );

	while (!roots.empty() )
	{
		foreach( GameObject *rootObject, roots )
		{
			removeObject( rootObject );
			delete rootObject;
		}

		rootObjects( roots );
	}

	assert( _objects.empty() );
	assert( _objectsByInstanceId.empty());
	assert( _objectsById.empty());
	
	cpSpaceFree( _space );
	
	// both must be valid, or neither.
	assert( (_scenario && _resourceManager) || (!_scenario && !_resourceManager));
	
	if ( _scenario && _resourceManager )
	{
		_scenario->resourceManager()->removeChild( _resourceManager );
		delete _resourceManager;
	}
}	

void Level::initialize( const init &initializer )
{
	_initializer = initializer;
	
	if ( _space )
	{
		throw InitException( "Level has already been initialized" );
	}

	_space = cpSpaceNew();
	cpSpaceSetIterations( _space, 20 );
	cpSpaceSetGravity( _space, cpv( _initializer.gravity ) );
	cpSpaceSetDamping( _space, _initializer.damping );
	cpSpaceSetSleepTimeThreshold( _space, 1 );
	
	_collisionDispatcher = new CollisionDispatcher( _space );
}

cpBB Level::bounds() const
{
	std::cerr << "Level::bounds - default implementation needs to be overrided" << std::endl;
	return cpBBNew(0, 0, 4096, 4096);
}

void Level::resize( const Vec2i &newSize )
{}

void Level::step( seconds_t deltaT )
{
	if ( !_ready )
	{
		ready();
	}

	if ( !_paused && _space ) 
	{
		_lastStepInterval = lrp<seconds_t>(0.15, _lastStepInterval, deltaT );
		
		//
		//	Update physics. Note: we're "locking" here to make certain nobody adds or removes
		//	GameObject or Behavior instances while updating. This is mainly a sanity check,
		//	but by no means is it a guarantee.
		//
		
		cpSpaceStep( _space, _lastStepInterval );
	}
}

void Level::update( const time_state &time )
{
	if ( !_paused )
	{
		_time = time;
	}

	if ( !_ready )
	{
		ready();
	}
	
	if ( !_paused )
	{
		{
			bool behaviorCleanupNeeded = false;
			BehaviorSet moribundBehaviors;

			for( BehaviorSet::iterator behaviorIt(_behaviors.begin()),end(_behaviors.end()); behaviorIt!=end; ++behaviorIt )
			{
				Behavior *behavior = *behaviorIt;

				if ( !behavior->finished() ) 
				{
					behavior->update( time );
				}
				else
				{
					behaviorCleanupNeeded = true;
					moribundBehaviors.insert( behavior );
				}
			}
			
			if ( behaviorCleanupNeeded )
			{
				foreach( Behavior *mb, moribundBehaviors )
				{
					removeBehavior( mb );
					delete mb;
				}
			}
		}
		
		{
			bool gameObjectCleanupNeeded = false;
			GameObjectSet moribundGameObjects;
			
			for( GameObjectSet::iterator objIt(_objects.begin()),end(_objects.end()); objIt != end; ++objIt )
			{
				GameObject *obj = *objIt;

				if ( !obj->finished())
				{
					obj->dispatchUpdate( time );
				}
				else
				{
					gameObjectCleanupNeeded = true;
					moribundGameObjects.insert( obj );
				}
			}
			
			if ( gameObjectCleanupNeeded )
			{
				foreach( GameObject *g, moribundGameObjects )
				{
					removeObject( g );
					delete g;
				}
			}
		}
	}
	
	//
	//	cull visibility sets
	//

	_drawDispatcher.cull( _scenario->renderState() );
}

void Level::draw( const render_state &state )
{	
	//
	//	This may look odd, but Level is responsible for Pausing, not the owner Scenario. This means that
	//	when the game is paused, the level is responsible for sending a mock 'paused' time object that reflects
	//	the time when the game was paused. I can't do this at the Scenario level because Scenario may have UI
	//	which needs correct time updates.
	//

	render_state localState = state;
	localState.time = _time.time;
	localState.deltaT = _time.deltaT;

	_drawDispatcher.draw( localState );
}

void Level::addObject( GameObject *object )
{
	assert( !cpSpaceIsLocked( _space ));

	object->_checkIdentifier();

	if ( _objectsById[object->identifier()])
	{
		throw Exception( "Level::addObject - An object with ID " + object->identifier() + " has already been added to this level" );
	}

	if ( object->level() )
	{
		if ( object->level() == this ) return;
		object->level()->removeObject( object );
	}

	_objects.insert(object);
	_objectsByInstanceId[object->instanceId()] = object;
	_objectsById[object->identifier()] = object;
	_drawDispatcher.addObject( object );

	object->willBeAddedToLevel( object, this );
	object->_setLevel(this);
	object->wasAddedToLevel( object, this );
		
	if ( _ready ) 
	{
		object->ready();
		foreach( Component *c, object->components())
		{
			c->ready();
		}
	}
}

bool Level::removeObject( GameObject *object )
{
	assert( !cpSpaceIsLocked( _space ) );

	if ( object->level() == this )
	{
		object->willBeRemovedFromLevel( object, this );

		_objects.erase( object );
		_objectsByInstanceId.erase( object->instanceId() );
		_objectsById.erase( object->identifier() );
		_drawDispatcher.removeObject( object );
		
		object->_setLevel(NULL);
		object->wasRemovedFromLevel( object, this );

		return true;
	}
	
	return false;
}

void Level::rootObjects( GameObjectSet &roots ) const
{
	roots.clear();
	foreach( GameObject *obj, _objects )
	{
		if ( !obj->parent() ) 
		{
			roots.insert(obj);
		}
	}
}

GameObject *Level::objectByInstanceId( std::size_t instanceId ) const
{
	GameObjectsByInstanceId::const_iterator pos(_objectsByInstanceId.find(instanceId));
	return ( pos == _objectsByInstanceId.end()) ? NULL : pos->second;
}

GameObject *Level::objectById( const std::string &identifier ) const
{
	GameObjectsById::const_iterator pos(_objectsById.find(identifier));
	return ( pos == _objectsById.end()) ? NULL : pos->second;
}

void Level::addBehavior( Behavior *behavior )
{
	assert( !cpSpaceIsLocked( _space ) );

	if ( behavior->level() )
	{
		if ( behavior->level() == this ) return;
		behavior->level()->removeBehavior( behavior );
	}

	_behaviors.insert(behavior);

	behavior->willBeAddedToLevel( behavior, this );
	behavior->_setLevel(this);		
	behavior->wasAddedToLevel( behavior, this );
		
	if ( _ready ) 
	{
		behavior->ready();
	}
}

bool Level::removeBehavior( Behavior *behavior )
{
	assert( !cpSpaceIsLocked( _space ) );

	if ( behavior->level() == this )
	{
		behavior->willBeRemovedFromLevel( behavior, this );

		_behaviors.erase( behavior );
		
		behavior->_setLevel(NULL);
		behavior->wasRemovedFromLevel( behavior, this );

		return true;
	}
	
	return false;
}

void Level::ready()
{
	foreach( GameObject *obj, _objects )
	{	
		obj->ready();	
		foreach( Component *c, obj->components()) c->ready();
	}

	foreach( Behavior *b, _behaviors )	
	{	
		b->ready();	
	}
	
	_ready = true;
}

const Viewport &Level::camera() const
{
	return _scenario->camera();
}

Viewport &Level::camera()
{
	return _scenario->camera();
}

void Level::setPaused( bool paused )
{
	if ( paused != _paused )
	{
		_paused = paused;
		if ( _paused ) levelWasPaused(this);
		else levelWasUnpaused(this);
	}
}

void Level::_addedToScenario( Scenario *s )
{
	_scenario = s;		
	_resourceManager = new ResourceManager(_scenario->resourceManager());

	addedToScenario( _scenario );
}

void Level::_removedFromScenario( Scenario *s )
{
	if ( _scenario )
	{
		_scenario->resourceManager()->removeChild( _resourceManager );
	}

	delete _resourceManager;
	_resourceManager = NULL;

	_scenario = NULL;
	removedFromScenario( s );
}


} // end namespace core