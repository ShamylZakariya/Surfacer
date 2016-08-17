//
//  Sensor.cpp
//  Surfacer
//
//  Created by Shamyl Zakariya on 5/8/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include "Sensor.h"

#include "GameConstants.h"
#include "Player.h"
#include "PowerCell.h"
#include "Terrain.h"

#include "GameNotifications.h"

#include <cinder/gl/gl.h>
#include <cinder/Text.h>
#include <cinder/Font.h>

using namespace ci;
using namespace core;

namespace game {

namespace {

	const ColorA SensorColorInactive(0,0.5,1, 0.25);
	const ColorA SensorColorActive( 1,0.5,0,0.5 );
	ci::Font SensorDebugDrawFont;

}

namespace SensorTargetType
{

	std::string toString( type t )
	{
		switch( t )
		{
			case NONE: return "NONE";
			case PLAYER: return "PLAYER";
			case TERRAIN: return "TERRAIN";
			case POWERCELL: return "POWERCELL";
		}
		
		return "NONE";
	}
	
	type fromString( const std::string &s )
	{
		std::string uc = util::stringlib::uppercase(s);
		if ( uc == "PLAYER" ) return PLAYER;
		if ( uc == "TERRAIN" ) return TERRAIN;
		if ( uc == "POWERCELL" ) return POWERCELL;
		return NONE;
	}


}

CLASSLOAD(Sensor);

/*
			init _initializer;
			int _triggerCount;
			cpBody *_body;
			cpShape *_shape;			
			std::map< std::size_t, int > _contacts;
			std::string _description;
*/

Sensor::Sensor():
	GameObject( GameObjectType::SENSOR ),
	_triggerCount(0),
	_body(NULL),
	_shape(NULL)
{
	setName( "Sensor" );
	setLayer(RenderLayer::SENSORS);
	setVisibilityDetermination( VisibilityDetermination::FRUSTUM_CULLING );
	setDebugColor(SensorColorInactive);
	addComponent( new SensorRenderer );
}

Sensor::~Sensor()
{
	aboutToDestroyPhysics(this);
	cpCleanupAndFree( _shape );
	cpCleanupAndFree( _body );
}

void Sensor::initialize( const init &i )
{
	GameObject::initialize(i);
	_initializer = i;
	
	switch( i.targetType )
	{
		case SensorTargetType::PLAYER:		
		case SensorTargetType::POWERCELL:
		case SensorTargetType::NONE:
			_description = "Sensor(" + eventName() + ")[" + SensorTargetType::toString(i.targetType) + "]";
			break;

		case SensorTargetType::TERRAIN:		
			if ( _initializer.voxelId > 0 )
			{
				_description = "Sensor(" + eventName() + ")[" + SensorTargetType::toString(i.targetType) + " id: " + str(_initializer.voxelId) + "]";
			}
			else 
			{
				_description = "Sensor(" + eventName() + ")[" + SensorTargetType::toString(i.targetType) + " id: ALL]";
			}
			break;
			
	}			
}

std::string Sensor::description() const
{
	return _description;
}

#pragma mark -

void Sensor::_trigger( bool onOrOff )
{		
	if (!this->eventName().empty())
	{
		const sensor_event 
			Event( this, onOrOff, this->eventName() );

		sensorTriggered( Event );
		notificationDispatcher()->broadcast(Notification(Notifications::SENSOR_TRIGGERED, Event ));


		if ( !onOrOff && _initializer.once )
		{
			setFinished(true);
		}
	}
}


// GameObject
void Sensor::addedToLevel( core::Level *level )
{
	GameObject::addedToLevel(level);

	_body = cpBodyNewStatic();

	_shape = cpBoxShapeNew( _body, _initializer.width, _initializer.height );
	cpSpaceAddShape( level->space(), _shape );
	cpShapeSetUserData( _shape, this );	
	cpShapeSetCollisionType( _shape, CollisionType::SENSOR );
	cpShapeSetLayers( _shape, CollisionLayerMask::ALL );
	cpShapeSetSensor( _shape, true );
	
	cpBodySetPos( _body, cpv(_initializer.position) );
	cpBodySetAngle( _body, _initializer.angle );
	cpSpaceReindexShapesForBody(level->space(), _body);
	
	setAabb( cpShapeGetBB( _shape ));
		
	CollisionDispatcher *dispatcher = level->collisionDispatcher();
	switch( _initializer.targetType )
	{
		case SensorTargetType::PLAYER:
		{
			CollisionDispatcher::signal_set &signals = dispatcher->dispatch( CollisionType::SENSOR, CollisionType::PLAYER, this );
			signals.contact.connect( this, &Sensor::_contactPlayer );
			signals.separate.connect( this, &Sensor::_separatePlayer );
			break;
		}

		case SensorTargetType::TERRAIN:
		{
			CollisionDispatcher::signal_set &signals  = dispatcher->dispatch( CollisionType::SENSOR, CollisionType::TERRAIN, this );
			signals.contact.connect( this, &Sensor::_contactTerrain );
			signals.separate.connect( this, &Sensor::_separateTerrain );
			break;
		}
		
		case SensorTargetType::POWERCELL:
		{
			CollisionDispatcher::signal_set &signals  = dispatcher->dispatch( CollisionType::SENSOR, CollisionType::DECORATION, this );
			signals.contact.connect( this, &Sensor::_contactPowerCell );
			signals.separate.connect( this, &Sensor::_separatePowerCell );
			break;
		}
		
		case SensorTargetType::NONE:
		{}
	}	
	
}

void Sensor::_contactPlayer( const core::collision_info &info, bool &discard )
{
	discard = true;
	_onContact( info.b );
}

void Sensor::_separatePlayer( const core::collision_info &info )
{
	_onSeparate( info.b );
}

void Sensor::_contactTerrain( const core::collision_info &info, bool &discard )
{
	terrain::Island *island = static_cast< terrain::Island* >( info.b );
	if ( _islandTriggers( island ))
	{
		_onContact( island );
	}
}

void Sensor::_separateTerrain( const core::collision_info &info )
{
	_onSeparate( info.b );
}

bool Sensor::_islandTriggers( terrain::Island *island )
{
	//
	//	An island which cannot trigger this sensor can never trigger this sensor, so this is our early out
	//
	if ( _ignorable.count( island->instanceId())) 
	{
		return false;
	}

	const unsigned int VoxelId = _initializer.voxelId;

	//
	//	When VoxelId == 0, all voxels pass, and that means that this island passes, since it by definition intersects the sensor
	//

	if ( VoxelId == 0 )
	{
		return true;
	}
	

	bool someVoxelsMatch = false;

	terrain::IslandGroup *group = island->group();
	const Mat4r mv(group->modelview());
	
	for( std::vector< terrain::Voxel* >::const_iterator voxel(island->voxels().begin()), end( island->voxels().end()); voxel != end; ++voxel )
	{
		terrain::Voxel *v = *voxel;
		if ( v->id == VoxelId )
		{
			someVoxelsMatch = true;
			if ( cpShapePointQuery( _shape, cpv( mv * v->centroidRelativePosition )))
			{
				return true;
			}
		}		
	}
	
	//
	//	The island did not have any voxels which match, so it can't ever trigger this sensor. 
	//

	if ( !someVoxelsMatch )
	{
		_ignorable.insert( island->instanceId() );
	}
	
	return false;
}

void Sensor::_contactPowerCell( const core::collision_info &info, bool &discard )
{
	if ( (info.b->type() == GameObjectType::POWERCELL) )
	{
		PowerCell *powerCell = static_cast<PowerCell*>(info.b);
		
		if ( powerCell->empty() ) 
		{
			return;
		}
				
		_onContact( powerCell );
	}
}

void Sensor::_separatePowerCell( const core::collision_info &info )
{
	_onSeparate( info.b );
}

void Sensor::_onContact( GameObject *obj )
{
	if ( _isFirstContact( obj ))
	{
		_triggerCount++;
		if ( _triggerCount == 1 )
		{
			_trigger( true );
		}
	}
}

void Sensor::_onSeparate( GameObject *obj )
{
	if ( _isFinalSeparation( obj ))
	{
		_triggerCount = std::max( _triggerCount-1, 0); 
		if ( _triggerCount == 0 )
		{
			_trigger( false );
		}
	}	
}


bool Sensor::_isFirstContact( GameObject *obj )
{
	const std::size_t ObjId = obj->instanceId();
	std::map< std::size_t, int >::iterator pos( _contacts.find(ObjId));

	if ( pos == _contacts.end() )
	{
		_contacts[ObjId] = 1;
		return true;
	}
	
	pos->second++;
	return pos->second == 1;
}

bool Sensor::_isFinalSeparation( GameObject *obj )
{
	const std::size_t ObjId = obj->instanceId();
	std::map< std::size_t, int >::iterator pos( _contacts.find(ObjId));

	if ( pos != _contacts.end() )
	{
		pos->second = std::max( pos->second - 1, 0 );
		return pos->second == 0;
	}
	
	_contacts[ObjId] = 0;
	return true;
}

#pragma mark - SensorRenderer

SensorRenderer::SensorRenderer()
{
	setName( "SensorRenderer" );
}

SensorRenderer::~SensorRenderer()
{}
		
void SensorRenderer::_drawGame( const core::render_state &state )
{}	

void SensorRenderer::_drawDevelopment( const core::render_state &state )
{
	_drawDebug( state );
}
		
void SensorRenderer::_drawDebug( const core::render_state &state )
{
	Sensor *sensor = this->sensor();
	const ci::ColorA SensorColor = sensor->triggered() ? SensorColorActive : SensorColorInactive;

	gl::enableAlphaBlending();
	_debugDrawChipmunkShape(state, sensor->shape(), SensorColor, SensorColor );
	cpBBDraw( sensor->aabb(), SensorColor);
		
	if ( !_descTex )
	{
		if ( !SensorDebugDrawFont )
		{
			SensorDebugDrawFont = Font( "Menlo", 10 );
		}
	
		TextLayout layout;
		layout.clear( ColorA( 0,0,0,0 ) );
		layout.setColor( Color::white() );
		layout.setFont( SensorDebugDrawFont );		
		layout.addLine( sensor->description() );
		
		_descTex = layout.render( true, false );
		//_descTex.setFlipped( true );
	}
	
	
	const Vec2r
		CenterWorld(v2r(cpBBCenter(sensor->aabb()))),
		CenterScreen( state.viewport.worldToScreen(CenterWorld)),
		Center( state.viewport.screenToWorld( Vec2r( std::floor( CenterScreen.x ), std::floor( CenterScreen.y))));

	const real
		ReciprocalZoom = state.viewport.reciprocalZoom();
		
	Vec2r		
		labelWorldHalfSize( _descTex.getWidth()/2 * ReciprocalZoom, _descTex.getHeight()/2 * ReciprocalZoom );

	const Rectf LabelScreenRect(
		Center.x - labelWorldHalfSize.x, 
		Center.y + labelWorldHalfSize.y, 
		Center.x + labelWorldHalfSize.x, 
		Center.y - labelWorldHalfSize.y );

	gl::color(Color::white());
	gl::draw( _descTex, LabelScreenRect );
}

#pragma mark - ViewportFitSensor

CLASSLOAD(ViewportFitSensor);

const std::string ViewportFitSensor::FitEventName("ViewportFitSensorEvent");

ViewportFitSensor::ViewportFitSensor()
{}

ViewportFitSensor::~ViewportFitSensor()
{}

void ViewportFitSensor::initialize( const init &i )
{
	_initializer = i;	
	_initializer.eventName = FitEventName;
	_initializer.targetType = SensorTargetType::PLAYER;
	
	Sensor::initialize(_initializer);
}

std::string ViewportFitSensor::description() const
{
	return "[ViewportFitSensor]";
}


}
