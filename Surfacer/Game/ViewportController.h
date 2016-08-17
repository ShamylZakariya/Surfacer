#pragma once

//
//  ViewportController.h
//  Surfacer
//
//  Created by Zakariya Shamyl on 10/18/11.
//  Copyright 2011 TomorrowPlusX. All rights reserved.
//

#include "GameObject.h"
#include "Viewport.h"

namespace game {

class ViewportFitSensor;

class ViewportController : public core::Behavior
{
	public:
	
		enum constraint {
			NoConstraint = 0,
			ConstrainPanning = 1,
			ConstrainZooming = 2	
		};

		enum control_method {			
			Zeno,
			PID		
		};
		
		/**
			Configurator structor for Zeno control method.
			There are two fields, panFactor and zoomFactor which are
			in the range of [0,1], representing the interpolation distance
			to take from the current viewport value to the controller's set point.
			
			A panFactor of 0.75 will, for example, cause the error between the controller
			set point and the current viewport value to be reduced by 75% each timestep.
			
		*/
		struct zeno_config {
			real panFactor, zoomFactor;
			
			zeno_config():
				panFactor(0.06),
				zoomFactor(0.06)
			{}
			
			zeno_config( real pf, real zf ):
				panFactor(pf),
				zoomFactor(zf)
			{}
			
		};

	public:
	
		ViewportController( core::Viewport &vp, control_method cm = Zeno );
		virtual ~ViewportController();

		///////////////////////////////////////////////////////////////////////////

		// NotificationListener
		virtual void notificationReceived( const core::Notification &note );
		
		// Behavior
		virtual void update( const core::time_state &time );
		virtual void ready();

		///////////////////////////////////////////////////////////////////////////
		

		core::Viewport &viewport() const { return _viewport; }		
		
		void setControlMethod( control_method m ) { _controlMethod = m; }
		control_method controlMethod() const { return _controlMethod; }
		
		void setZenoConfig( zeno_config zc ) { _zenoConfig = zc; }
		zeno_config &zenoConfig() { return _zenoConfig; }
		const zeno_config &zenoConfig() const { return _zenoConfig; }
		
		void setConstraintMask( unsigned int cmask );
		unsigned int constraintMask() const { return _constraintMask; }

		/**
			Set the zoom
		*/
		void setZoom( real z ) { _zoom = _constrainZoom(z); }
		
		/**
			set the zoom and pan such that @a about stays in the same spot on screen
		*/
		void setZoom( real z, const Vec2r &about );

		/**
			get the current zoom
		*/
		real zoom() const { return _zoom; }
		
		/**
			set the pan
		*/
		void setPan( const Vec2r &p ) { _pan = _constrainPan( p ); }

		/**
			get the current pan
		*/
		Vec2r pan() const { 
			return _pan;
		}
				
		/**
			cause the viewport to pan such that @a world is at location @a screen with @a zoom
			@a world Location of interest in world coordinates
			@a zoom desired level of zoom
			@a screen focal point in screen coordinates
			
			This is the function to use if, for example, you want the camera to pan to some
			object or character of interest.			
		*/
		void lookAt( const Vec2r &world, real zoom, const Vec2r &screen );

		/**
			Shortcut to lookAt( const Vec2r &world, real zoom, const Vec2r &screen )
		*/
		void lookAt( const Vec2r &world, real zoom ) { lookAt( world, zoom, _viewport.viewportCenter() ); }
		
		/**
			Shortcut to lookAt( const Vec2r &world, real zoom, const Vec2r &screen )
			Causes the camera to center @a world in the screen
		*/
		void lookAt( const Vec2r &world ) { lookAt( world, _zoom, _viewport.viewportCenter() ); }
		
		void setTrackingTarget( core::GameObject *tt );
		core::GameObject *trackingTarget() const { return _trackingTarget; }
		
		void setTrackingEnabled( bool te ) { _trackingEnabled = te; }
		bool trackingEnabled() const { return _trackingEnabled; }
		
		bool tracking() const { return trackingEnabled() && trackingTarget(); }

	protected:

		Vec2r _constrainPan( Vec2r p ) const;
		real _constrainZoom( real z ) const;
	
		virtual void _updateZeno( const core::time_state &time );
		virtual void _updatePID( const core::time_state &time );
		
		void _viewportInitiatedChange( const core::Viewport &vp );
		void _viewportBoundsChanged( const core::Viewport &vp, const ci::Area &bounds );
		
		void _releaseTrackingTarget( core::GameObject * );
		
	private:
	
		core::Viewport		&_viewport;
		unsigned int		_constraintMask;
		control_method		_controlMethod;
		real				_zoom;
		Vec2r				_pan;
		cpBB				_levelBounds;		
		zeno_config			_zenoConfig;		
		bool				_disregardViewportMotion, _trackingEnabled;
		core::GameObject	*_trackingTarget;	

		std::set< core::GameObject* > _activeViewportFitSensors;
		cpBB				_activeViewportFitSensorsCombinedAabb;

};


} // end namespace game