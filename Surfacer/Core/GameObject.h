#pragma once

//
//  GameObject.h
//  PhysicsIntegration4
//
//  Created by Shamyl Zakariya on 4/9/11.
//  Copyright 2011 Shamyl Zakariya. All rights reserved.
//

#include "Common.h"
#include "Classloader.h"
#include "Components.h"
#include "InputDispatcher.h"
#include "Notification.h"
#include "RenderState.h"
#include "SignalsAndSlots.h"
#include "TimeState.h"
#include "Viewport.h"

#include <cinder/Rand.h>

namespace core {

class Behavior;
class GameObject;
class Level;

typedef std::set<Behavior*> BehaviorSet;
typedef std::vector<Behavior*> BehaviorVec;

typedef std::set<GameObject*> GameObjectSet;
typedef std::map<std::size_t, GameObject*> GameObjectsByInstanceId;
typedef std::map<std::string, GameObject*> GameObjectsById;
typedef std::vector<GameObject*> GameObjectVec;

#pragma mark -
#pragma mark Object

/**
	Base class for Behavior and GameObject
*/
class Object : public signals::receiver, public NotificationListener, public Classloadable {
	public:
	
		Object( const std::string &name ):
			_name( name )
		{}
		
		Object()
		{}
		
		virtual ~Object(){}
		
		const std::string &name() const { return _name; }
		void setName( const std::string &name ) { _name = name; }
		
		/**
			Override to provide meaningful/stateful descriptions of objects at runtime
		*/
		virtual std::string description() const;
						
	private:
	
		std::string _name;

};


#pragma mark - Behavior

/**
	@class Behavior
	Base class for things which run in a core::Level, but which don't draw.
	A behavior is updated by Level until it reports that it is done, via Behavior::done();
	at that point the behavior is deleted and removed from the level.
*/

class Behavior : public Object
{
	public:
	
		signals::signal< void(Behavior*, Level*) > willBeAddedToLevel;
		signals::signal< void(Behavior*, Level*) > wasAddedToLevel;

		signals::signal< void(Behavior*, Level*) > willBeRemovedFromLevel;
		signals::signal< void(Behavior*, Level*) > wasRemovedFromLevel;

		signals::signal< void(Behavior*) > aboutToBeDestroyed;

	public:
	
		Behavior():
			_finished(false),
			_level(NULL)
		{};
		
		virtual ~Behavior()
		{
			aboutToBeDestroyed(this);
		}

		/**
			Called by Level in Level::ready(), after all objects have been added.
			Note: If an object is added during gameplay, ready() will be called on it immediately.
		*/
		virtual void ready(){}
		
		/**
			timestep update function
		*/
		virtual void update( const time_state &time ) = 0;
		
		/**
			Set to true to make this object be removed and deleted in the next timestep.
		*/
		virtual void setFinished( bool f ) { _finished = f; }
		
		/**
			If true, this object will be removed and deleted in the next timetstep
		*/
		inline bool finished() const { return _finished; }

		/**
			Get the level this Behavior has been added to
		*/
		Level *level() const { return _level; }

		/**
			Callback called when this Behavior has been added to a Level.
			Note: level() is valid and refers to the new level when this is called.
		*/
		virtual void addedToLevel( Level *level ){}

		/**
			Callback called when this Behavior is removed from a Level, passing the level removed from
			Note: level() is valid and refers to the level being removed from when this is called.
		*/
		virtual void removedFromLevel( Level *removedFrom ){}
		
		/**
			Get the notification dispatcher instance for broadcast messaging
		*/
		NotificationDispatcher *notificationDispatcher() const;

	protected:
	
		friend class Level;

		virtual void _setLevel( Level *l ) 
		{ 
			if ( _level ) 
			{
				removedFromLevel( _level );
			}
			
			_level = l;

			if ( l ) 
			{
				addedToLevel( l );
			}
		}
		
	protected:
	
		bool _finished;
		Level *_level;
};

#pragma mark - Visibility Determination Mode

/**
	Describe the approaches to visibility determination for a GameObject.
*/
namespace VisibilityDetermination 
{
	enum style {
	
		ALWAYS_DRAW,
		NEVER_DRAW,
		FRUSTUM_CULLING
	
	};
}


#pragma mark - BatchDrawDelegate

/**
	@class BatchDrawDelegate
	
	Some GameObjects are part of a larger whole ( terrain::Island, for example ) and it is a great
	help to efficiency to mark them as being part of a single batch to be rendered contiguously. In
	this case, the render system will group the batch together and then when drawing them, will
	for the first call prepareForBatchDraw on the first's batchDrawDelegate, draw each, and then
	call cleanupAfterBatchDraw on the last of the contiguous set.	
*/
class BatchDrawDelegate 
{
	public:
	
		BatchDrawDelegate(){}
		virtual ~BatchDrawDelegate(){}
	
		/**
			Called before a batch of like objects are rendered.
			Passes the render_state, and the first object in the series. This is to simplify
			situations where a single object is BatchDrawDelegate to multiple target batches.
		*/
		virtual void prepareForBatchDraw( const render_state &, GameObject *firstInBatch ){}
		virtual void cleanupAfterBatchDraw( const render_state &, GameObject *firstInBatch, GameObject *lastInBatch ){}

};


#pragma mark - GameObject


/**
	@class GameObject
	Base class for game objects.
*/
class GameObject : public Object
{
	public:

		struct init : public core::util::JsonInitializable 
		{
			std::string identifier;

			init()
			{}

			//JsonInitializable
			virtual void initialize( const ci::JsonTree &v )
			{
				JSON_READ(v,identifier);
			}

		};
	
		signals::signal< void(GameObject*, Level*) > willBeAddedToLevel;
		signals::signal< void(GameObject*, Level*) > wasAddedToLevel;

		signals::signal< void(GameObject*, Level*) > willBeRemovedFromLevel;
		signals::signal< void(GameObject*, Level*) > wasRemovedFromLevel;

		signals::signal< void(GameObject*) > aboutToBeDestroyed;
		signals::signal< void(GameObject*) > aboutToDestroyPhysics;

	public:
	
		GameObject( int gameObjectType );
		virtual ~GameObject();

		JSON_INITIALIZABLE_INITIALIZE();
		
		// initialization
		virtual void initialize( const init & );
		const init &initializer() const { return _initializer; }

		// Object
		virtual std::string description() const;

		
		/**
			Get the unique instance ID of this GameObject.
			The instance ID is a number automatically assigned at run-time guaranteed to be unique.
		*/
		std::size_t instanceId() const { return _instanceId; }

		/**
			Get the unique identifer of this GameObject.
			The identifier is assigned during initialize() and is guaranteed to be unique for a given level. 
			If a GameObject with this identifier already exists in the level when this instance is added, an
			exception will be thrown.
		*/

		const std::string &identifier() const { return _identifier; }
		
		void setVisibilityDetermination( VisibilityDetermination::style style );
		VisibilityDetermination::style visibilityDetermination() const { return _visibilityDetermination; }
		
		inline const cpBB &aabb() const { return _aabb; }
		virtual void setAabb( const cpBB &bb );

		/**
			return the position of this GameObject in world coordinates.
			Default implementation returns the center of the object's AABB. Subclasses may
			choose to make a more meaningful implementation.
		*/
		virtual Vec2r position() const { return v2r(cpBBCenter(_aabb)); }
				
		inline int layer() const { return _layer; }
		virtual void setLayer( int l ) { _layer = l; }
		
		inline int type() const { return _type; }
		
		inline const ci::ColorA &debugColor() const { return _debugColor; }
		void setDebugColor( const ci::ColorA &c ) { _debugColor = c; }
								
		/**
			Set to true to make this object be removed and deleted in the next timestep.
		*/
		virtual void setFinished( bool f ) { _finished = f; }
		
		/**
			If true, this object will be removed and deleted in the next timetstep
		*/
		virtual bool finished() const { return _finished; }
		
		virtual void update( const time_state &time ){}
		virtual void draw( const render_state &state ){}
		
		/**
			Check if this GameObject was visible in the last update
		*/
		bool visible() const;
		
		/**
			Called by Level in Level::ready(), after all objects have been added.
			Note: If an object is added during gameplay, ready() will be called on it immediately.
		*/
		virtual void ready(){}
		
		/**
			@return the parent GameObject of this GameObject
		*/
		GameObject *parent() const { return _parent; }
		
		/**
			@return the parent-most root GameObject of this GameObject
		*/
		GameObject *root()
		{
			GameObject *p = _parent;
			if ( p )
			{
				while( p->parent() )
				{
					p = p->parent();
				}
				
				return p;
			}
			
			// if we have no parent, we're root
			return this;
		}
		
		/**
			Add a child GameObject to this GameObject.
			Return true if it was added, false if it was
			already a child and nothing changed
		*/
		virtual bool addChild( GameObject *child );

		/**
			Remove a child from this group.
			Return true if it was a child and was removed.
			Return false if it was not, and nothing changed.
			
			@note If this GameObject is in a level, the child will be removed from that level.
		*/
		virtual bool removeChild( GameObject *child );
		
		/**
			Remove all children from this GameObject
		*/
		void removeAllChildren();
		
		/**
			Makes a child no longer a child of this GameObject, but leaves it in its level.
			Return true if it was a child of this object and was orphaned
			Return false if it was not, and nothing changed.
		*/
		virtual bool orphanChild( GameObject *child );
		
		/**
			Orphans all children of this GameObject
		*/
		void orphanAllChildren();

		/**
			@return all direct-desendent children
		*/
		const GameObjectSet &children() const { return _children; }
		
		GameObjectSet &children() { return _children; }

		/**
			check if we have a direct-descendent child
		*/
		bool hasChild( GameObject *child ) const { return _children.count(child); }
		
		template < class T >
		void addChildren( const T &children ) { foreach( GameObject *child, children ) { addChild( child ); }}

		template < class T >
		void removeChildren( const T &children ) { foreach( GameObject *child, children ) { removeChild( child ); }}

		/**
			removes each child and deletes it. works recursively across childrens' children.
		*/
		void destroyChildren();

		/**
			Collect all GameObjects reachable from this GameObject instance
		*/
		void gather( GameObjectSet &all )
		{
			root()->_rootGather(all);
		}
		
		/**
			Get the level this GameObject has been added to
		*/
		Level *level() const { return _level; }

		/**
			Get the notification dispatcher instance for broadcast messaging
		*/
		NotificationDispatcher *notificationDispatcher() const;
		
		/**
			Get the number of seconds elapsed since this GameObject was added to a Level
		*/
		seconds_t age() const { return _age; }

		/**
			Callback called when this GameObject has been added to a Level.
			Note: level() is valid and refers to the new level when this is called.
		*/
		virtual void addedToLevel( Level *level ){}

		/**
			Callback called when this GameObject is removed from a Level, passing the level removed from
			Note: level() is valid and refers to the level being removed from when this is called.
		*/
		virtual void removedFromLevel( Level *removedFrom ){}
		
		/**
			Add a component to this GameObject
			You would add a DrawComponent for example to render.
		*/
		virtual void addComponent( Component * );
		
		/**
			Remove a component. You're responsible for reparenting or deleting it.
		*/
		virtual void removeComponent( Component * );
		
		/**
			Get all the components
		*/
		const std::set< Component* > &components() const { return _components; }
		
		virtual void dispatchUpdate( const time_state &time );
		virtual void dispatchDraw( const render_state &state );		
		
		void setBatchDrawDelegate( BatchDrawDelegate *bdd ) { _batchDrawDelegate = bdd; }
		BatchDrawDelegate *batchDrawDelegate() const { return _batchDrawDelegate; }
		
		/**
			Set the number of draw passes this GameObject needs. Defaults to 1.
			draw() will be called on this object once for each pass, with the render_state::pass set to 0,1,2,etc
		*/
		void setDrawPasses( std::size_t passes ) { _drawPasses = passes; }
		std::size_t drawPasses() const { return _drawPasses; }
		
		
	protected:
	
		friend class Level;
		friend class DrawDispatcher;

		virtual void _setLevel( Level *l );
		virtual void _setParent( GameObject *p ) { _parent = p; }	
		void _rootGather( GameObjectSet &all );
		void _checkIdentifier();
				
	private:

		init _initializer;
		bool _finished;
		VisibilityDetermination::style _visibilityDetermination;
		int _layer, _type;
		cpBB _aabb;
		ci::ColorA _debugColor;
		std::size_t _instanceId, _drawPasses;
		std::string _identifier;

		BatchDrawDelegate *_batchDrawDelegate;
		std::set< Component * > _components;
		GameObjectSet _children;
		GameObject *_parent;
		Level *_level;
		seconds_t _age;
		
		static std::size_t _instanceIdCounter;
		
};

/**
	Draw a chipmunk cpBB - assumes the modelview matrix to be the camera transform only. If you need to
	draw an AABB while transformed into a local coordinate space, see the alternate version below.
*/
void cpBBDraw( cpBB bb, const ci::ColorA &color, real padding = 0 );

/**
	Draws a cpBB regardless of the current transformation matrix
*/
void cpBBDraw( const cpBB &bb, const render_state &state, const ci::ColorA &color, real padding = 0 );

} // namespace core

#pragma mark -

inline std::ostream &operator << ( std::ostream &os, core::RenderMode::mode mode )
{
	return os << core::RenderMode::toString(mode);
}



