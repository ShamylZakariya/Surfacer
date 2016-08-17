#pragma once

//
//  Components.h
//  Surfacer
//
//  Created by Shamyl Zakariya on 11/11/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "InputDispatcher.h"
#include "SignalsAndSlots.h"
#include "RenderState.h"
#include "TimeState.h"

namespace core {

class Component;
class GameObject;
class Level;

typedef std::set<Component*> ComponentSet;
typedef std::vector<Component*> ComponentVec;

#pragma mark - Component

class Component : public signals::receiver
{
	public:
	
		Component():
			_owner(NULL)
		{}
		
		virtual ~Component()
		{}
		
		void setName( const std::string &name ) { _name = name; }
		const std::string &name() const { return _name; }
		
		inline GameObject *owner() const { return _owner; }
		virtual std::string description() const;

		void setOwner( GameObject *newOwner )
		{
			if ( _owner != newOwner )
			{
				removedFromGameObject( _owner );

				_owner = newOwner;
				
				if ( _owner )
				{
					addedToGameObject( _owner );
				}
			}
		}
		
		Level *level() const;
		
		
		virtual void update( const time_state &time ){}
		virtual void draw( const render_state &state ){}
		virtual void ready(){}

		virtual void removedFromGameObject( GameObject * ) {}
		virtual void addedToGameObject( GameObject * ) {}
		
	private:
	
		GameObject *_owner;
		std::string _name;

};


#pragma mark - DrawComponent

/**
	@class DrawComponent
	Base class for rendering components for GameObject instances. 
	
	We're separating the game objects from their renderers to avoid spaghetti, and also
	to make it easy to have different 'levels' of rendering, say, one batch of nice
	shaded renderers for desktop, and a simpler set of renderers for iOS.
	
*/
class DrawComponent : public Component
{
	public:
	
		DrawComponent();
		virtual ~DrawComponent(){}

		/**
			Dispatches draw call to
				- _drawGame
				- _drawDevelopment (which punts to _drawGame)
				- _drawDebug
		*/
		virtual void draw( const render_state & );

	protected:

		virtual void _drawGame( const render_state & ){}
		virtual void _drawDevelopment( const render_state &s ){ _drawGame(s); }
		virtual void _drawDebug( const render_state & ){}
		
		void _debugDrawChipmunkBody( const render_state &state, cpBody *body );
		void _debugDrawChipmunkConstraint( const render_state &state, cpConstraint *constraint, const ci::ColorA &color );
		void _debugDrawChipmunkShape( const render_state &state, cpShape *shape, const ci::ColorA &fillColor, const ci::ColorA &strokeColor = ci::ColorA(0,0,0,0.5) );

		void _debugDrawChipmunkShapes( const render_state &state, const cpShapeSet &shapes );
		void _debugDrawChipmunkShapes( const render_state &state, const cpShapeVec &shapes );
		void _debugDrawChipmunkConstraints( const render_state &state, const cpConstraintSet &constraints );
		void _debugDrawChipmunkConstraints( const render_state &state, const cpConstraintVec &constraints );
		void _debugDrawChipmunkBodies( const render_state &state, const cpBodySet &bodies );
		void _debugDrawChipmunkBodies( const render_state &state, const cpBodyVec &bodies );
		
};

#pragma mark - InputComponent

/**
	Base class for input delegating components
*/

class InputComponent : public Component, public InputListener
{
	public:
	
		InputComponent();
		virtual ~InputComponent(){}
		
		void monitorKey( int keyCode );
		void ignoreKey( int keyCode );
		
		/**
			Called when a monitored key is pressed
		*/
		virtual void monitoredKeyDown( int keyCode ){}

		/**
			Called when a monitored key is released
		*/
		virtual void monitoredKeyUp( int keyCode ){}
		
		/**
			return true if any monitored keys are pressed.
			this ignores any keys that haven't been registered for monitoring by monitorKey()
		*/
		virtual bool isKeyDown( int keyCode ) const;						

		virtual bool keyDown( const ci::app::KeyEvent &event );
		virtual bool keyUp( const ci::app::KeyEvent &event );
		virtual void ready();
		
	protected:
	
		std::map< int, bool > _monitoredKeyStates;

};

}