/*
 *  GameObject.cpp
 *  Surfacer
 *
 *  Created by Shamyl Zakariya on 5/18/11.
 *  Copyright 2011 Shamyl Zakariya. All rights reserved.
 *
 */

#include "GameObject.h"
#include "DrawDispatcher.h"
#include "Level.h"
#include "Scenario.h"

#include <cinder/app/App.h>
#include <cinder/Color.h>
#include <cinder/gl/gl.h>

using namespace ci;
namespace core {

#pragma mark - Object

std::string Object::description() const
{
	std::stringstream stream;
	stream << "[" << name() << " @ " << this << "]";
	return stream.str();
}

#pragma mark - Behavior

NotificationDispatcher *Behavior::notificationDispatcher() const 
{ 
	return (level() && level()->scenario()) ? level()->scenario()->notificationDispatcher() : NULL;
}


#pragma mark - GameObject

std::size_t GameObject::_instanceIdCounter = 0;

/*
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
*/

GameObject::GameObject( int type ):
	_finished( false ),
	_visibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING ),
	_layer(0),
	_type(type),
	_aabb( cpBBInvalid ),
	_debugColor( RandomColor() ),
	_instanceId( _instanceIdCounter++ ),
	_drawPasses(1),
	_batchDrawDelegate(NULL),
	_parent(NULL),
	_level(NULL),
	_age(0)
{}

GameObject::~GameObject()
{
	assert( !_level );

	aboutToBeDestroyed(this);

	foreach( GameObject *child, _children )
	{
		delete child;
	}
	
	foreach( Component *c, _components )
	{
		delete c;
	}
}

void GameObject::initialize( const init &i )
{
	_initializer = i;
	_identifier = _initializer.identifier;
	_checkIdentifier();
}

std::string GameObject::description() const
{
	if ( !identifier().empty() )
	{
		std::stringstream stream;
		stream << "[" << name() << "(" << identifier() << ") @ " << this << "]";
		return stream.str();
	}

	return Object::description();
}

void GameObject::setVisibilityDetermination( VisibilityDetermination::style style ) 
{ 
	if ( _level )
	{
		_level->removeObject( this );
	}
	
	_visibilityDetermination = style; 
	
	if ( _level )
	{
		_level->addObject( this );
	}
}

void GameObject::setAabb( const cpBB &bb )
{
	_aabb = bb; 
	if ( _level )
	{
		_level->drawDispatcher().objectMoved( this );
	}
}

bool GameObject::visible() const
{
	return _level ? _level->drawDispatcher().visible(this) : false;
}

bool GameObject::addChild( GameObject *child )
{
	if ( child->parent() && (child->parent() != this) )
	{
		child->parent()->removeChild( child );
	}
	
	if ( child->level() && (child->level() != _level) )
	{
		child->level()->removeObject( child );
	}

	if ( _children.insert( child ).second )
	{
		child->_setParent( this );
		
		if ( child->level() != _level )
		{
			_level->addObject( child );
		}
		
		return true;
	}
		
	return false;
}

bool GameObject::removeChild( GameObject *child )
{
	assert( child );
	assert( child->level() ? child->level() == this->level() : true );

	if ( child->parent() == this && _children.count(child))
	{
		if ( child->level() )
		{
			child->level()->removeObject( child );
		}
			
		child->_setParent(NULL);
		return _children.erase( child ) > 0;
	}
	
	return false;
}

void GameObject::removeAllChildren()
{
	foreach( GameObject *child, _children )
	{
		if ( child->level() )
		{
			child->level()->removeObject( child );
		}
		
		child->_setParent(NULL);
	}
	
	_children.clear();
}

bool GameObject::orphanChild( GameObject *child )
{
	assert( child );
	if ( child->parent() == this && _children.count(child))
	{
		child->_setParent(NULL);
		return _children.erase(child) > 0;
	}
	
	return false;
}
		
void GameObject::orphanAllChildren()
{
	foreach( GameObject *child, _children )
	{
		child->_setParent(NULL);
	}
	
	_children.clear();
}

void GameObject::destroyChildren()
{
	foreach( GameObject *child, _children )
	{
		child->destroyChildren();

		if ( child->level() ) 
		{
			child->level()->removeObject( child );
		}
		
		delete child;
	}
	
	_children.clear();
}

NotificationDispatcher *GameObject::notificationDispatcher() const 
{ 
	return (level() && level()->scenario()) ? level()->scenario()->notificationDispatcher() : NULL;
}

void GameObject::addComponent( Component *c )
{
	if ( c->owner() != this )
	{
		if ( c->owner() )
		{
			c->owner()->removeComponent(c);
		}	
		
		_components.insert(c);
		c->setOwner(this);
	}
}

void GameObject::removeComponent( Component *c )
{
	if ( c->owner() == this )
	{
		_components.erase( c );
		c->setOwner(NULL);
	}
}

void GameObject::_setLevel( Level *l )
{
	Level *previousLevel = _level;
	_level = l;
	_age = 0;

	if ( _level ) 
	{
		foreach( GameObject *child, _children )
		{
			_level->addObject( child );
		}

		addedToLevel( l );
	}
	else
	{
		if ( previousLevel )
		{
			foreach( GameObject *child, _children )
			{
				previousLevel->removeObject( child );
			}

			removedFromLevel( previousLevel );
		}
	}
}

void GameObject::dispatchUpdate( const time_state &time )
{
	_age += time.deltaT;

	//
	//	Update components, and then self.
	//

	foreach( Component *c, _components )
	{
		c->update( time );
	}

	update( time );
}

void GameObject::dispatchDraw( const render_state &state )
{
	//
	//	Draw self & components
	//

	draw( state );

	foreach( Component *c, _components )
	{
		c->draw( state );
	}
}

void GameObject::_rootGather( GameObjectSet &all )
{
	all.insert( this );
	foreach( GameObject *child, _children )
	{
		all.insert( child );
		child->_rootGather( all );
	}
}

void GameObject::_checkIdentifier()
{
	if ( _identifier.empty() )
	{
		_identifier = "[Unnamed-" + str( _instanceId );
	}
}

#pragma mark -

void cpBBDraw( cpBB bb, const ci::ColorA &color, real padding )
{
	bb.l -= padding;
	bb.r += padding;
	bb.t += padding;
	bb.b -= padding;
	
	gl::color( color );
	gl::drawLine( Vec2r( bb.l, bb.b ), Vec2r( bb.r, bb.b ) );
	gl::drawLine( Vec2r( bb.r, bb.b ), Vec2r( bb.r, bb.t ) );
	gl::drawLine( Vec2r( bb.r, bb.t ), Vec2r( bb.l, bb.t ) );
	gl::drawLine( Vec2r( bb.l, bb.t ), Vec2r( bb.l, bb.b ) );	
}

void cpBBDraw( const cpBB &bb, const render_state &state, const ci::ColorA &color, real padding )
{
	gl::pushModelView();	
	glLoadMatrixf( state.viewport.modelview() );
		
		cpBBDraw( bb, color, padding * state.viewport.reciprocalZoom() );

	gl::popModelView();
}



}