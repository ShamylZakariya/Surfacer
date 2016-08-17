//
//  ViewportController.cpp
//  Surfacer
//
//  Created by Zakariya Shamyl on 10/18/11.
//  Copyright (c) 2011 TomorrowPlusX. All rights reserved.
//

#include "ViewportController.h"

#include "GameLevel.h"
#include "GameNotifications.h"
#include "Level.h"
#include "Player.h"
#include "Sensor.h"

using namespace ci;
using namespace core;
namespace game {

/*
		core::Viewport	&_viewport;
		unsigned int	_constraintMask;
		control_method	_controlMethod;
		real			_zoom;
		Vec2r			_pan;
		cpBB			_levelBounds;		
		zeno_config		_zenoConfig;		
		bool			_disregardViewportMotion, _trackingEnabled;
		GameObject		*_trackingTarget;	

		std::set< core::GameObject* > _activeViewportFitSensors;
		cpBB				_activeViewportFitSensorsCombinedAabb;
*/

ViewportController::ViewportController( Viewport &vp, control_method cm ):
	_viewport(vp),
	_constraintMask(NoConstraint),
	_controlMethod(cm),
	_zoom(vp.zoom()),
	_pan(vp.pan()),
	_levelBounds( cpBBInfinity ),
	_disregardViewportMotion(false),
	_trackingEnabled(false),
	_trackingTarget(NULL),
	_activeViewportFitSensorsCombinedAabb(cpBBInvalid)
{
	setName( "ViewportController" );
	_viewport.motion.connect(this, &ViewportController::_viewportInitiatedChange );
	_viewport.boundsChanged.connect(this, &ViewportController::_viewportBoundsChanged );
}

ViewportController::~ViewportController()
{}

void ViewportController::notificationReceived( const core::Notification &note )
{
	switch( note.identifier() )
	{
		case game::Notifications::SENSOR_TRIGGERED:
		{
			//
			//	If it's a triggering event we need to push the sensor to the fit stack, if it's a removal we need to erase that sensor
			//

			const sensor_event &Event = boost::any_cast< sensor_event >( note.message() );
			if ( Event.eventName == ViewportFitSensor::FitEventName )
			{
				if ( Event.triggered )
				{				
					_activeViewportFitSensors.insert( Event.sensor );
				}
				else
				{
					_activeViewportFitSensors.erase( Event.sensor );
				}
				
				_activeViewportFitSensorsCombinedAabb = cpBBInvalid;
				foreach( GameObject *sensor, _activeViewportFitSensors )
				{
					_activeViewportFitSensorsCombinedAabb = cpBBMerge( _activeViewportFitSensorsCombinedAabb, sensor->aabb());
				}
			}
			
			break;
		}
	}
}

void ViewportController::update( const time_state &time )
{
	if ( _trackingEnabled )
	{
		GameObject *target = trackingTarget();
		GameLevel *level = static_cast<GameLevel*>(this->level());
		
		if ( target )
		{
			if ( !_activeViewportFitSensors.empty() )
			{
				//
				//	Compute a zoom level that will fit the combined aabb to the viewport, showing all
				//
			
				real Zoom = level->initializer().defaultZoom;
				
				const real
					ViewportWidth = _viewport.width(),
					ViewportHeight = _viewport.height(),
					TargetWidth = cpBBWidth(_activeViewportFitSensorsCombinedAabb),
					TargetHeight = cpBBHeight(_activeViewportFitSensorsCombinedAabb);
					
				Zoom = ViewportWidth / TargetWidth;
				if ( TargetHeight * Zoom > ViewportHeight )
				{
					Zoom = ViewportHeight / TargetHeight;
				}
				
				lookAt( v2r(cpBBCenter(_activeViewportFitSensorsCombinedAabb)), Zoom );				
			}
			else
			{
				lookAt( v2r(cpBBCenter( target->aabb())), level->initializer().defaultZoom );	
			}
		}
	}

	//
	//	Now smoothly apply the changes
	//

	switch( _controlMethod )
	{
		case Zeno:
			_updateZeno( time );
			break;
			
		case PID:
			_updatePID( time );
			break;
	}
}

void ViewportController::ready()
{
	_levelBounds = level()->bounds();
}

void ViewportController::setConstraintMask( unsigned int cmask )
{
	_constraintMask = cmask;
	if ( _constraintMask )
	{
		_pan = _constrainPan( _pan );
		_zoom = _constrainZoom( _zoom );
	}		
}

void ViewportController::setZoom( real z, const Vec2r &aboutScreen )
{
	Mat4r mv, imv;
	Viewport::modelviewFor( _pan, _zoom, mv );

	imv = mv.affineInverted();
	Vec2r aboutWorld = imv * aboutScreen;
	
	Viewport::modelviewFor( _pan, _constrainZoom(z), mv );

	Vec2r postZoomAboutScreen = mv * aboutWorld;

	_pan = _constrainPan( _pan + ( aboutScreen - postZoomAboutScreen ) );
	_zoom = _constrainZoom( z );
}

void ViewportController::lookAt( const Vec2r &world, real zoom, const Vec2r &screen )
{
	_zoom = _constrainZoom(zoom);

	Mat4r mv;
	Viewport::modelviewFor( _pan, _zoom, mv );

	_pan = _constrainPan( _pan + (screen - (mv*world)));
}

void ViewportController::setTrackingTarget( GameObject *tt ) 
{
	if ( tt == _trackingTarget ) return;

	if ( _trackingTarget )
	{
		_trackingTarget->aboutToBeDestroyed.disconnect( this );
	}

	_trackingTarget = tt; 
	
	if ( _trackingTarget )
	{
		_trackingTarget->aboutToBeDestroyed.connect( this, &ViewportController::_releaseTrackingTarget );
	}
}


#pragma mark -
#pragma mark Private

Vec2r ViewportController::_constrainPan( Vec2r pan ) const
{
	if ( _levelBounds == cpBBInfinity ) return pan;

	if ( _constraintMask & ConstrainPanning )
	{		
		pan.x = std::min( pan.x, _levelBounds.l * _zoom );
		pan.y = std::min( pan.y, _levelBounds.b * _zoom );
		
		real right = (_levelBounds.r * _zoom) - _viewport.width(),
			 top = (_levelBounds.t * _zoom) - _viewport.height();

		pan.x = std::max( pan.x, -right );
		pan.y = std::max( pan.y, -top );	
	}
	
	return pan;
}

real ViewportController::_constrainZoom( real zoom ) const
{
	if ( _levelBounds == cpBBInfinity ) return zoom;

	if ( _constraintMask & ConstrainZooming )
	{
		real minZoomWidth = _viewport.width() / (_levelBounds.r - _levelBounds.l),
			 minZoomHeight = _viewport.height() / (_levelBounds.t - _levelBounds.b );

		zoom = std::max( zoom, std::max( minZoomWidth, minZoomHeight ));
	}
	else
	{
		zoom = std::max( zoom, Epsilon );
	}
	
	return zoom;
}


void ViewportController::_updateZeno( const time_state &time )
{
	_disregardViewportMotion = true;

	Vec2r panError = _pan - _viewport.pan();
	real zoomError = _zoom - _viewport.zoom();
	
	const real
		Rate = static_cast<real>(1)/ time.deltaT;
	
	bool panNeedsCorrecting = panError.lengthSquared() > Epsilon,
	     zoomNeedsCorrecting = std::abs( zoomError ) > Epsilon;
	
	if ( panNeedsCorrecting && zoomNeedsCorrecting )
	{
		_viewport.setPanAndZoom( 
			_viewport.pan() + panError * std::pow( 1 - _zenoConfig.panFactor, Rate),
			_viewport.zoom() + zoomError * std::pow( 1 - _zenoConfig.zoomFactor, Rate ));
	}
	else if ( panNeedsCorrecting )
	{
		_viewport.setPan( _viewport.pan() + panError * std::pow( 1-_zenoConfig.panFactor, Rate ));
	}
	else if ( zoomNeedsCorrecting )
	{
		_viewport.setZoom( _viewport.zoom() + zoomError * std::pow( 1-_zenoConfig.zoomFactor, Rate));
	}

	_disregardViewportMotion = false;
}

void ViewportController::_updatePID( const time_state &time )
{
	_disregardViewportMotion = true;
	
	// unimplemented...

	_disregardViewportMotion = false;
}

void ViewportController::_viewportInitiatedChange( const Viewport &vp )
{
	if ( !_disregardViewportMotion )
	{
		_pan = vp.pan();
		_zoom = vp.zoom();
	}
}

void ViewportController::_viewportBoundsChanged( const Viewport &vp, const Area &bounds )
{
	_pan = _constrainPan( _pan );
	_zoom = _constrainZoom( _zoom );
}

void ViewportController::_releaseTrackingTarget( GameObject *obj )
{
	assert( obj == _trackingTarget );
	_trackingTarget = NULL;
}



} // end namespace game