#pragma once

//
//  GameLevel.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/13/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "GameObject.h"

#include "CollisionDispatcher.h"
#include "DrawDispatcher.h"
#include "FilterStack.h"
#include "ResourceManager.h"

namespace core {

class Scenario;

class Level : public signals::receiver, public NotificationListener, public core::util::JsonInitializable
{
	public:

		struct init : public core::util::JsonInitializable
		{
			std::string name;
			Vec2 gravity;
			real damping;
						
			init():
				gravity( 0,-9.8 ),
				damping(0.98)
			{}
				
			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				JSON_READ(v,gravity);
				JSON_READ(v,damping);
			}

							
		};
		
		signals::signal< void(Level*) > levelWasPaused;
		signals::signal< void(Level*) > levelWasUnpaused;

	public:
	
		Level();
		virtual ~Level();
		
		JSON_INITIALIZABLE_INITIALIZE();
		
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }

		cpSpace* space() const { return _space; }
		bool spaceIsLocked() const { return cpSpaceIsLocked( _space ); }
		CollisionDispatcher *collisionDispatcher() const { return _collisionDispatcher; }
		
		/**
			Get the name of this level
		*/
		const std::string &name() const { return _initializer.name; }

		/**
			Get the Scenario this Level is running in
		*/
		Scenario *scenario() const { return _scenario; }
		
		/**
			Get this Level's ResourceManager. It is a child resource manager of the Scenario's root ResourceManager
		*/
		ResourceManager *resourceManager() const { return _resourceManager; }
		
		/**
			return the bounding rect for the level.
		*/
		virtual cpBB bounds() const;
		
		
		virtual void resize( const Vec2i &newSize );
		virtual void step( seconds_t deltaT );
		virtual void update( const time_state &time );
		virtual void draw( const render_state &state );
		
		virtual void addObject( GameObject *object );
		bool removeObject( GameObject *object );
		const GameObjectSet &objects() const { return _objects; }
		void rootObjects( GameObjectSet &roots ) const;
		GameObject *objectByInstanceId( std::size_t instanceId ) const;
		GameObject *objectById( const std::string &identifier ) const;

		virtual void addBehavior( Behavior *behavior );
		bool removeBehavior( Behavior *behavior );
		const BehaviorSet &behaviors() const { return _behaviors; }
		
		virtual void addedToScenario( Scenario *s ){}
		virtual void removedFromScenario( Scenario *s ){}
		
		virtual void ready();
				
		const DrawDispatcher &drawDispatcher() const { return _drawDispatcher; }
		DrawDispatcher &drawDispatcher() { return _drawDispatcher; }
		
		const time_state &time() const { return _time; }
		const Viewport &camera() const;
		Viewport &camera();
		
		void setPaused( bool paused );
		bool paused() const { return _paused; }
			
	protected:
	
		friend class Scenario;
		void _addedToScenario( Scenario *s );
		void _removedFromScenario( Scenario *s );
	
	protected:
	
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
				
};


}
